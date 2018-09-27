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

#ifndef AAUDIO_ECHOAUDIOENGINE_H
#define AAUDIO_ECHOAUDIOENGINE_H

#include <thread>
#include "audio_common.h"

class StreamEngine {

public:

    short *buffer_;
    ~StreamEngine();
    void errorCallback(AAudioStream *stream, aaudio_result_t  __unused error);

    void writeBuffer(short *buffer);
    void stop();
    void start();

private:

    int32_t playbackDeviceId_ = AAUDIO_UNSPECIFIED;
    aaudio_format_t format_ = AAUDIO_FORMAT_PCM_I16;
    int32_t sampleRate_;
    int32_t inputChannelCount_ = kMonoChannelCount; //1
    int32_t outputChannelCount_ = kStereoChannelCount; // 2

    int32_t framesPerBurst_;
    std::mutex restartingLock_;

    AAudioStream *playStream_ = nullptr;

    void createPlaybackStream();

    void startStream(AAudioStream *stream);
    void stopStream(AAudioStream *stream);
    void closeStream(AAudioStream *stream);
    void openPlaybackStream();
    void closePlaybackStream();
    void restartStreams();

    AAudioStreamBuilder *createStreamBuilder();
    void setupPlaybackStreamParameters(AAudioStreamBuilder *builder);

    void warnIfNotLowLatency(AAudioStream *stream);

    void writeBufferToStreamer();

};

#endif //AAUDIO_ECHOAUDIOENGINE_H
