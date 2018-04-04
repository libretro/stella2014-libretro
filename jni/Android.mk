LOCAL_PATH := $(call my-dir)

CORE_DIR := ../stella
include $(CLEAR_VARS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	LOCAL_CXXFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM
LOCAL_ARM_MODE := arm
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CFLAGS += -DANDROID_MIPS
endif

LOCAL_MODULE    := libretro

CORE_DIR     := ../stella
LIBRETRO_DIR := ..

include ../Makefile.common

LOCAL_SRC_FILES = $(SOURCES_CXX) $(SOURCES_C)
LOCAL_CFLAGS = -DINLINE=inline -DHAVE_STDINT_H -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 -fexceptions -DSOUND_SUPPORT $(INCFLAGS)

LOCAL_CPP_EXTENSION := .cxx .cpp

LOCAL_C_INCLUDES = $(INCFLAGS)

LOCAL_LDLIBS := -latomic

include $(BUILD_SHARED_LIBRARY)
