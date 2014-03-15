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

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
EXE_EXT = .exe
   system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   system_platform = osx
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   system_platform = win
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
else ifeq ($(platform), ps3)
   TARGET := $(TARGET_NAME)_libretro_ps3.a
   CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
   CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
   AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
   STATIC_LINKING = 1
	FLAGS += -DMSB_FIRST
	OLD_GCC = 1
else ifeq ($(platform), sncps3)
   TARGET := $(TARGET_NAME)_libretro_ps3.a
   CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
   CXX = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
   AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
   STATIC_LINKING = 1
	FLAGS += -DMSB_FIRST
	NO_GCC = 1
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_psp1.a
	CC = psp-gcc$(EXE_EXT)
	CXX = psp-g++$(EXE_EXT)
	AR = psp-ar$(EXE_EXT)
   STATIC_LINKING = 1
	FLAGS += -G0 -DLSB_FIRST
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

ifeq ($(DEBUG),1)
FLAGS += -O0 -g
else
FLAGS += -O2 -ffast-math
endif

LDFLAGS += $(fpic) -lz $(SHARED)
FLAGS += $(fpic) 
FLAGS += -I. -Istella -Istella/cart -Istella/input -Istella/system -Istella/utility -Istella/properties

ifeq ($(OLD_GCC), 1)
WARNINGS := -Wall
else ifeq ($(NO_GCC), 1)
WARNINGS :=
else
WARNINGS := -Wall \
	-Wno-narrowing \
	-Wno-unused-but-set-variable \
	-Wno-sign-compare \
	-Wno-unused-variable \
	-Wno-unused-function \
	-Wno-uninitialized \
	-Wno-unused-result \
	-Wno-strict-aliasing \
	-Wno-overflow \
	-fno-strict-overflow
endif

FLAGS += $(WARNINGS)

CXXFLAGS += $(FLAGS) -DHAVE_INTTYPES
CFLAGS += $(FLAGS) -std=gnu99

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) -o $@ $^ $(LDFLAGS)
endif

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
