/*
 * Copyright 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <logging_macros.h>
#include <climits>
#include <assert.h>
#include <audio_common.h>
#include "StreamEngine.h"
#include <thread>


using namespace std;

///////////////////////////////// Method From Java ////////////////////////////////////

void StreamEngine::stop() {
    closePlaybackStream();
}
void StreamEngine::start() {

    openPlaybackStream();

    thread t1(&StreamEngine::writeBufferToStreamer,this);
    t1.detach();
}
void StreamEngine::writeBuffer(short *buffer) {
    //AAudioStream_write(recordingStream_, buffer, 5000, 0);
    buffer_ = buffer;
}

///////////////////////////////// LOCAL METHOD ////////////////////////////////////

/**
 * If there is an error with a stream this function will be called. A common example of an error
 * is when an audio device (such as headphones) is disconnected. In this case you should not
 * restart the stream within the callback, instead use a separate thread to perform the stream
 * recreation and restart.
 *
 * @param stream the stream with the error
 * @param userData the context in which the function is being called, in this case it will be the
 * EchoAudioEngine instance
 * @param error the error which occured, a human readable string can be obtained using
 * AAudio_convertResultToText(error);
 *
 * @see EchoAudioEngine#errorCallback
 */
void errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error) {
    assert(userData);
    StreamEngine *audioEngine = reinterpret_cast<StreamEngine *>(userData);
    audioEngine->errorCallback(stream, error);
}

///////////////////////////////// Callback ////////////////////////////////////

/**
 * See the C method errorCallback at the top of this file
 */
void StreamEngine::errorCallback(AAudioStream *stream, aaudio_result_t error) {

    LOGI("errorCallback has result: %s", AAudio_convertResultToText(error));
    aaudio_stream_state_t streamState = AAudioStream_getState(stream);
    if (streamState == AAUDIO_STREAM_STATE_DISCONNECTED) {

        // Handle stream restart on a separate thread
        std::function<void(void)> restartStreams = std::bind(&StreamEngine::restartStreams,
                                                             this);
        std::thread streamRestartThread(restartStreams);
        streamRestartThread.detach();
    }
}

///////////////////////////////// Method ////////////////////////////////////

/**
 * Close the stream. After the stream is closed it is deleted and subesequent AAudioStream_* calls
 * will return an error. AAudioStream_close() also checks and waits for any outstanding dataCallback
 * calls to complete before closing the stream. This means the application does not need to add
 * synchronization between the dataCallback function and the thread calling AAudioStream_close()
 * [the closing thread is the UI thread in this sample].
 * @param stream the stream to close
 */
void StreamEngine::closeStream(AAudioStream *stream) {

    if (stream != nullptr) {
        aaudio_result_t result = AAudioStream_close(stream);
        if (result != AAUDIO_OK) {
            LOGE("Error closing stream. %s", AAudio_convertResultToText(result));
        }
    }
}
void StreamEngine::startStream(AAudioStream *stream) {

    /// per https://developer.android.com/ndk/guides/audio/aaudio/aaudio

    aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STARTING;
    aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
    int64_t timeoutNanos = 100 * AAUDIO_NANOS_PER_MILLISECOND;

    aaudio_result_t result = AAudioStream_requestStart(stream);
    result = AAudioStream_waitForStateChange(stream, inputState, &nextState, timeoutNanos);

    if (result != AAUDIO_OK) {
        LOGE("Error starting stream. %s", AAudio_convertResultToText(result));
    }
}
void StreamEngine::stopStream(AAudioStream *stream) {

    if (stream != nullptr) {

        aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
        aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
        int64_t timeoutNanos = 100 * AAUDIO_NANOS_PER_MILLISECOND;

        aaudio_result_t result = AAudioStream_requestStop(stream);

        result = AAudioStream_waitForStateChange(stream, inputState, &nextState, timeoutNanos);

        if (result != AAUDIO_OK) {
            LOGE("Error stopping stream. %s", AAudio_convertResultToText(result));
        }
    }
}


void StreamEngine::openPlaybackStream() {

    // Note: The order of stream creation is important. We create the playback stream first,
    // then use properties from the playback stream (e.g. sample rate) to create the
    // recording stream. By matching the properties we should get the lowest latency path
    createPlaybackStream();

    // Now start the stream so we can read from it during playback
    // stream's dataCallback
    if (playStream_ != nullptr) {
        startStream(playStream_);
    } else {
        LOGE("Failed to create recording and/or playback stream");
    }
}
/**
 * Stops and closes the playback and recording streams.
 */
void StreamEngine::closePlaybackStream() {

    if (playStream_ != nullptr) {
        aaudio_stream_state_t inputState = AAUDIO_STREAM_STATE_STOPPING;
        aaudio_stream_state_t nextState = AAUDIO_STREAM_STATE_UNINITIALIZED;
        int64_t timeoutNanos = 100 * AAUDIO_NANOS_PER_MILLISECOND;

        aaudio_result_t result = AAudioStream_requestStop(playStream_);

        result = AAudioStream_waitForStateChange(playStream_, inputState, &nextState, timeoutNanos);

        //aaudio_result_t result = AAudioStream_close(playStream);
        if (result != AAUDIO_OK) {
            LOGE("Error closing stream. %s", AAudio_convertResultToText(result));
        }

        playStream_ = nullptr;
    }

}

/**
 * Restart the streams. During the restart operation subsequent calls to this method will output
 * a warning.
 */
void StreamEngine::restartStreams() {

    LOGI("Restarting streams");

    if (restartingLock_.try_lock()) {
        closePlaybackStream();
        openPlaybackStream();
        restartingLock_.unlock();
    } else {
        LOGW("Restart stream operation already in progress - ignoring this request");
        // We were unable to obtain the restarting lock which means the restart operation is currently
        // active. This is probably because we received successive "stream disconnected" events.
        // Internal issue b/63087953
    }
}

/**
 * Creates an audio stream for playback. The audio device used will depend on playbackDeviceId_.
 * If the value is set to AAUDIO_UNSPECIFIED then the default playback device will be used.
 */
void StreamEngine::createPlaybackStream() {
/// step 1 of https://developer.android.com/ndk/guides/audio/aaudio/aaudio
    AAudioStreamBuilder *builder = createStreamBuilder();

    if (builder != nullptr) {
/// step 2 of https://developer.android.com/ndk/guides/audio/aaudio/aaudio
        setupPlaybackStreamParameters(builder);
/// step 3 of https://developer.android.com/ndk/guides/audio/aaudio/aaudio
        aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &playStream_);
        if (result == AAUDIO_OK && playStream_ != nullptr) {

            sampleRate_ = AAudioStream_getSampleRate(playStream_);
            framesPerBurst_ = AAudioStream_getFramesPerBurst(playStream_);

            warnIfNotLowLatency(playStream_);

            // Set the buffer size to the burst size - this will give us the minimum possible latency
            AAudioStream_setBufferSizeInFrames(playStream_, framesPerBurst_);
            PrintAudioStreamInfo(playStream_);

        } else {
            LOGE("Failed to create playback stream. Error: %s", AAudio_convertResultToText(result));
        }
        AAudioStreamBuilder_delete(builder);
    } else {
        LOGE("Unable to obtain an AAudioStreamBuilder object");
    }
}

/**
 * Sets the stream parameters which are specific to playback, including device id and the
 * dataCallback function, which must be set for low latency playback.
 * @param builder The playback stream builder
 */
void StreamEngine::setupPlaybackStreamParameters(AAudioStreamBuilder *builder) {

//    AAudioStreamBuilder_setDeviceId(builder, playbackDeviceId_);  // occurs by default
//    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);  // output by default
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 2); // stereo
    AAudioStreamBuilder_setSampleRate(builder, 48000); // 48KHz

    AAudioStreamBuilder_setErrorCallback(builder, ::errorCallback, this);

    // The :: here indicates that the function is in the global namespace
    // i.e. *not* EchoAudioEngine::dataCallback, but dataCallback defined at the top of this class
    //AAudioStreamBuilder_setDataCallback(builder, ::dataCallback, this);

    // We request EXCLUSIVE mode since this will give us the lowest possible latency.
    // If EXCLUSIVE mode isn't available the builder will fall back to SHARED mode.
//    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_EXCLUSIVE);
//    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);

}

/**
 * Creates a stream builder which can be used to construct streams
 * @return a new stream builder object
 */
AAudioStreamBuilder *StreamEngine::createStreamBuilder() {
    AAudioStreamBuilder *builder = nullptr;
    aaudio_result_t result = AAudio_createStreamBuilder(&builder);
    if (result != AAUDIO_OK) {
        LOGE("Error creating stream builder: %s", AAudio_convertResultToText(result));
    }
    return builder;
}


void StreamEngine::warnIfNotLowLatency(AAudioStream *stream) {
    aaudio_performance_mode_t performance_mode = AAudioStream_getPerformanceMode(stream);
    switch (performance_mode) {
        case AAUDIO_PERFORMANCE_MODE_LOW_LATENCY:
            LOGW("Stream is low latency. Good!");
            break;
        case AAUDIO_PERFORMANCE_MODE_POWER_SAVING:
            LOGW("Stream is NOT low latency. It is Power Saving. Check your requested format, sample rate and channel count");
            break;
        default:
            LOGW("Stream does not offer particular performance needs. Default.");
    }
}

//Delete Object
StreamEngine::~StreamEngine() {
    stopStream(playStream_);
    closeStream(playStream_);
}

void StreamEngine::writeBufferToStreamer() {

//            int idx = 0;
//            for (int j = 0; j < 1000; ++j) {
//                int16_t buffer[192];
//                for (int i = 0; i < 192; ++i) {
//                    buffer[i] = buffer_[idx++]; // \2 OR *2 volume
//                }
//                aaudio_result_t result = AAudioStream_write(playStream_, buffer, 192,1);
//                LOGD("TASK WRITE DONE: %s",AAudio_convertResultToText(result));
//
//            }

    aaudio_result_t result = AAudioStream_write(playStream_, buffer_, 800000, 200);

}

/////////////////////////////////////////////////////////////////////////////////