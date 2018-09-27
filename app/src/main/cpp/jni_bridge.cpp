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


#include <jni.h>
#include <logging_macros.h>
#include "StreamEngine.h"

static StreamEngine *engine = nullptr;

extern "C" {

JNIEXPORT bool JNICALL
Java_com_example_aliassadi_aaudio_AAudioApi_init(JNIEnv *env,
                                                       jclass) {
  if (engine == nullptr) {
    engine = new StreamEngine();
  }

  return (engine != nullptr);
}

JNIEXPORT void JNICALL
Java_com_example_aliassadi_aaudio_AAudioApi_writeBuffer(JNIEnv *env,
                                                              jclass, jshortArray arr) {
  if (engine == nullptr) {
    LOGE("Engine is null, you must call createEngine before calling this method");
    return;
  }

  jshort *body = (env)->GetShortArrayElements(arr,0);

    engine->writeBuffer(body);
}


JNIEXPORT void JNICALL
Java_com_example_aliassadi_aaudio_AAudioApi_start(JNIEnv *env,
                                                                   jclass, jboolean isEchoOn) {
  if (engine == nullptr) {
    LOGE("Engine is null, you must call createEngine before calling this method");
    return;
  }

  engine->start();
}

JNIEXPORT void JNICALL
Java_com_example_aliassadi_aaudio_AAudioApi_stop(JNIEnv *env,
                                                        jclass, jboolean isEchoOn) {
    if (engine == nullptr) {
        LOGE("Engine is null, you must call createEngine before calling this method");
        return;
    }

    engine->stop();
}


JNIEXPORT void JNICALL
Java_com_example_aliassadi_aaudio_AAudioApi_delete(JNIEnv *env,
                                                         jclass) {
  delete engine;
  engine = nullptr;
}

}
