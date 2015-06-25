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

LOCAL_MODULE := libx264
LOCAL_SRC_FILES := libx264.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libavformat
LOCAL_SRC_FILES := libavformat.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libavcodec
LOCAL_SRC_FILES := libavcodec.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libavutil
LOCAL_SRC_FILES := libavutil.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := libswscale
LOCAL_SRC_FILES := libswscale.a

include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_C_INCLUDES :=/home/kiem/wsancode/ffmpeg-0.10.4
LOCAL_LDLIBS := -lz -llog
LOCAL_STATIC_LIBRARIES := libavformat libavcodec libswscale libavutil libx264

LOCAL_MODULE    := ffmpeg-jni
LOCAL_SRC_FILES := ffmpeg-jni.c

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE    := avjni
LOCAL_SRC_FILES := avjni.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS :=-L$(LOCAL_PATH)/include -lavformat -lavcodec -lavcore -lavdevice -lavfilter -lavutil -lswscale
LOCAL_LDLIBS += -llog -ljnigraphics -lz -ldl -lgcc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    :=serial_port
LOCAL_SRC_FILES :=SerialPort.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS := -L$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib -L$(LOCAL_PATH) -lavformat -lavcodec -lavdevice -lavfilter -lavcore -lavutil -lswscale -llog -ljnigraphics -lz -ldl -lgcc
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
include /home/kiem/wsancode/FFMpegAV_ban2/jni/OpenCV-2.3.1/share/OpenCV/OpenCV.mk

LOCAL_MODULE    :=opencv
LOCAL_SRC_FILES := jni_part.cpp
LOCAL_LDLIBS := -L$(NDK_PLATFORMS_ROOT)/$(TARGET_PLATFORM)/arch-arm/usr/lib -L$(LOCAL_PATH)
LOCAL_LDLIBS +=  -llog -ldl -ljnigraphics -lcurl

LOCAL_LDLIBS +=-lcurl
include $(BUILD_SHARED_LIBRARY)





