# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#


LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#LOCAL_MODULE := libhookhelper
#LOCAL_SRC_FILES := libhookhelper.so
#include $(PREBUILT_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE := libmedia
LOCAL_SRC_FILES := libmedia.so
include $(PREBUILT_SHARED_LIBRARY)

 

include $(CLEAR_VARS)

LOCAL_C_INCLUDES :=              	   \
    $(LOCAL_PATH)/libhook \
    $(LOCAL_PATH)/libhook/utils/include \
    $(LOCAL_PATH)/libhook/android_jni \
    $(LOCAL_PATH)/libhook/resample   \
    $(LOCAL_PATH)/android-headers-jbmr2/frameworks/av/include \
    $(LOCAL_PATH)/android-headers-jbmr2/frameworks/native/include \
    $(LOCAL_PATH)/android-headers-jbmr2/system/core/include \
    $(LOCAL_PATH)/android-headers-jbmr2/hardware/libhardware/include


LOCAL_MODULE    := internal_audio_record
LOCAL_SRC_FILES +=  \
    InternalRecord.cpp                \
    ./libhook/hook.cpp           \
    ./libhook/report.cpp         \
    ./libhook/hooks/io.cpp       \
    ./libhook/hvjava.c     	     \
    ./libhook/tool.cpp           \
    ./libhook/AssetManager.cpp   \
    ./libhook/android_jni/MediaExtractor_jni.cpp    \
    ./libhook/android_jni/AudioTrack_jni.cpp    \
    ./libhook/android_jni/AudioDecode_jn.c      \
    ./libhook/android_jni/AudioFormat_jni.cpp   \
    ./libhook/android_jni/AudioManager_jni.cpp  \
    ./libhook/android_jni/jni_helper.c        \
    ./libhook/utils/src/timer.c                 \
    ./libhook/utils/src/os.c                    \
    ./libhook/utils/src/List.c                  \
    ./libhook/utils/src/LoopBuf.c               \
    ./libhook/resample/resample.c               \
    ./libhook/resample/resample2.c              \
    ./libhook/resample/mem.c

#include $(call all-subdir-makefiles)

LOCAL_SHARED_LIBRARIES :=  libmedia

LOCAL_LDLIBS := -llog -ljnigraphics -lz -landroid -lEGL

include $(BUILD_SHARED_LIBRARY)
