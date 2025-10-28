LOCAL_PATH := $(call my-dir)

# include $(CLEAR_VARS)
# LOCAL_MODULE    := FsPipelineJNI
# LOCAL_SRC_FILES += FsPipelineJNI.cpp
# # 添加 C++ 标志和链接库
# LOCAL_CPPFLAGS := -std=c++14
# LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
# LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lm -lz -llog -lstdc++ -lfs_p2p -lmosquitto
# ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
# LOCAL_LDLIBS += -L$(LOCAL_PATH)/arm64-v8a
# else
# LOCAL_LDLIBS += -L$(LOCAL_PATH)/armeabi-v7a
# endif
# include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)
LOCAL_MODULE    := BaseXLink
LOCAL_SRC_FILES += BaseXLink.cpp \
                   JavaIotCallback.cpp \
                   BaseDataConverter.cpp \
                   Timer.cpp \
                   RequestManager.cpp \
                   Logger.cpp
# 添加 C++ 标志和链接库
LOCAL_CPPFLAGS := -std=c++14
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog -lm -lz -llog -lstdc++ -lfs_p2p -lmosquitto
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
LOCAL_LDLIBS += -L$(LOCAL_PATH)/arm64-v8a
else
LOCAL_LDLIBS += -L$(LOCAL_PATH)/armeabi-v7a
endif
include $(BUILD_SHARED_LIBRARY)
