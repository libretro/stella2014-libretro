
DEBUG = 0

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
endif
endif

TARGET_NAME := stella

ifeq ($(platform), unix)
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC -mmacosx-version-min=10.6
   SHARED := -dynamiclib
else ifeq ($(platform), ios)
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib -miphoneos-version-min=5.0

   CC = clang -arch armv7 -isysroot $(IOSSDK) -miphoneos-version-min=5.0
   CXX = clang++ -arch armv7 -isysroot $(IOSSDK) -miphoneos-version-min=5.0
else ifeq ($(platform), qnx)
   TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	CC = qcc -Vgcc_ntoarmv7le
	CXX = QCC -Vgcc_ntoarmv7le_cpp
else
   TARGET := $(TARGET_NAME)_libretro.dll
   CC = gcc
   CXX = g++
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
   LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm
endif
STELLA_DIR := stella

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

LIBRETRO_SOURCES := libretro.cpp

SOURCES := $(LIBRETRO_SOURCES) $(STELLA_SOURCES)
OBJECTS := $(SOURCES:.cpp=.o) $(SOURCES_C:.c=.o)

all: $(TARGET)

ifeq ($(DEBUG),0)
   FLAGS += -O3 -ffast-math -funroll-loops 
else
   FLAGS += -O0 -g
endif

LDFLAGS += $(fpic) -lz $(SHARED)
FLAGS += -Wall $(fpic) -fno-strict-overflow
FLAGS += -I. -Istella -Istella/cart -Istella/input -Istella/system -Istella/utility -Istella/properties

WARNINGS := -Wall \
	-Wno-narrowing \
	-Wno-unused-but-set-variable \
	-Wno-sign-compare \
	-Wno-unused-variable \
	-Wno-unused-function \
	-Wno-uninitialized \
	-Wno-unused-result \
	-Wno-strict-aliasing \
	-Wno-overflow

FLAGS += $(WARNINGS)

CXXFLAGS += $(FLAGS) -DHAVE_INTTYPES
CFLAGS += $(FLAGS) -std=gnu99

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
