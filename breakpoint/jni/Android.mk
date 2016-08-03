LOCAL_PATH:=$(call my-dir)

include $(CLEAR_VARS)
#LOCAL_SHARED_LIBRARIES := demo
LOCAL_LDLIBS := -llog
#LOCAL_CFLAGS += -mllvm -sub -mllvm -fla -mllvm -bcf 
LOCAL_MODULE := check_breakpoint 

LOCAL_SRC_FILES := \
check_breakpoint.c

#include $(BUILD_SHARED_LIBRARY)
include $(BUILD_EXECUTABLE)

