LOCAL_PATH := $(call my-dir)  

include $(CLEAR_VARS)

LOCAL_CFLAGS += -Wall -g -mllvm -sub -mllvm -fla -mllvm -bcf 
LOCAL_MODULE := inotify_test

LOCAL_SRC_FILES := \
event_queue.h \
event_queue.c \
inotify_utils.h \
inotify_utils.c \
inotify_test.c

include $(BUILD_EXECUTABLE)

