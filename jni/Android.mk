LOCAL_PATH := $(call my-dir)

STELLA_DIR := ../stella
include $(CLEAR_VARS)

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

STELLA_SOURCES := $(STELLA_DIR)/Console.cpp \
	$(STELLA_DIR)/Settings.cpp \
	$(STELLA_DIR)/SoundSDL.cpp \
	$(STELLA_DIR)/cart/Cart0840.cpp \
	$(STELLA_DIR)/cart/Cart2K.cpp \
	$(STELLA_DIR)/cart/Cart3E.cpp \
	$(STELLA_DIR)/cart/Cart3F.cpp \
	$(STELLA_DIR)/cart/Cart4A50.cpp \
	$(STELLA_DIR)/cart/Cart4K.cpp \
	$(STELLA_DIR)/cart/CartAR.cpp \
	$(STELLA_DIR)/cart/Cart.cpp	 \
	$(STELLA_DIR)/cart/CartCV.cpp \
	$(STELLA_DIR)/cart/CartDPC.cpp \
	$(STELLA_DIR)/cart/CartDPCPlus.cpp \
	$(STELLA_DIR)/cart/CartE0.cpp \
	$(STELLA_DIR)/cart/CartE7.cpp \
	$(STELLA_DIR)/cart/CartEF.cpp \
	$(STELLA_DIR)/cart/CartEFSC.cpp \
	$(STELLA_DIR)/cart/CartF0.cpp \
	$(STELLA_DIR)/cart/CartF4.cpp \
	$(STELLA_DIR)/cart/CartF4SC.cpp \
	$(STELLA_DIR)/cart/CartF6.cpp \
	$(STELLA_DIR)/cart/CartF6SC.cpp \
	$(STELLA_DIR)/cart/CartF8.cpp \
	$(STELLA_DIR)/cart/CartF8SC.cpp \
	$(STELLA_DIR)/cart/CartFA.cpp \
	$(STELLA_DIR)/cart/CartFE.cpp \
	$(STELLA_DIR)/cart/CartMC.cpp \
	$(STELLA_DIR)/cart/CartSB.cpp \
	$(STELLA_DIR)/cart/CartUA.cpp \
	$(STELLA_DIR)/cart/CartX07.cpp \
	$(STELLA_DIR)/cart/Thumbulator.cpp \
	$(STELLA_DIR)/input/AtariVox.cpp \
	$(STELLA_DIR)/input/Booster.cpp \
	$(STELLA_DIR)/input/Control.cpp \
	$(STELLA_DIR)/input/Driving.cpp \
	$(STELLA_DIR)/input/Genesis.cpp \
	$(STELLA_DIR)/input/Joystick.cpp \
	$(STELLA_DIR)/input/Keyboard.cpp \
	$(STELLA_DIR)/input/KidVid.cpp \
	$(STELLA_DIR)/input/MT24LC256.cpp \
	$(STELLA_DIR)/input/Paddles.cpp \
	$(STELLA_DIR)/input/SaveKey.cpp \
	$(STELLA_DIR)/input/Switches.cpp \
	$(STELLA_DIR)/input/TrackBall.cpp \
	$(STELLA_DIR)/system/M6502.cpp \
	$(STELLA_DIR)/system/M6532.cpp \
	$(STELLA_DIR)/system/NullDev.cpp \
	$(STELLA_DIR)/system/System.cpp \
	$(STELLA_DIR)/system/TIA.cpp \
	$(STELLA_DIR)/system/TIASnd.cpp \
	$(STELLA_DIR)/system/TIATables.cpp \
	$(STELLA_DIR)/utility/MD5.cpp \
	$(STELLA_DIR)/utility/Random.cpp \
	$(STELLA_DIR)/utility/Serializer.cpp \
	$(STELLA_DIR)/properties/Props.cpp \
	$(STELLA_DIR)/properties/PropsSet.cpp

LIBRETRO_SOURCES := ../libretro.cpp

LOCAL_SRC_FILES = $(STELLA_SOURCES) $(LIBRETRO_SOURCES)
LOCAL_CFLAGS = -DINLINE=inline -DHAVE_STDINT_H -DHAVE_INTTYPES -DLSB_FIRST -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 -fexceptions
LOCAL_C_INCLUDES = ../stella ../stella/cart ../stella/system ../stella/utility ../stella/input ../stella/properties

include $(BUILD_SHARED_LIBRARY)
