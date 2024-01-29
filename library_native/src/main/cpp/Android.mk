LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := FsPipelineJNI
LOCAL_SRC_FILES += FsPipelineJNI.cpp
# 添加 C++ 标志和链接库
LOCAL_CPPFLAGS := -std=c++14
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lm -lz -llog -lstdc++ -L$(LOCAL_PATH)/lib -lfs_p2p
include $(BUILD_SHARED_LIBRARY)
