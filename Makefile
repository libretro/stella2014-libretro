DEBUG = 0

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
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
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
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
   fpic := -fPIC
   SHARED := -dynamiclib

ifeq ($(arch),ppc)
	FLAGS += -DMSB_FIRST
	OLD_GCC = 1
endif
OSXVER = `sw_vers -productVersion | cut -c 4`
ifneq ($(OSXVER),9)
   fpic += -mmacosx-version-min=10.5
endif
else ifeq ($(platform), ios)
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib

   CC = clang -arch armv7 -isysroot $(IOSSDK)
   CXX = clang++ -arch armv7 -isysroot $(IOSSDK)
OSXVER = `sw_vers -productVersion | cut -c 4`
ifneq ($(OSXVER),9)
   SHARED += -miphoneos-version-min=5.0
   CC +=  -miphoneos-version-min=5.0
   CXX +=  -miphoneos-version-min=5.0
endif
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

STELLA_SOURCES := $(STELLA_DIR)/src/common/Base.cxx \
	$(STELLA_DIR)/src/common/SoundSDL.cxx \
	$(STELLA_DIR)/src/emucore/AtariVox.cxx \
	$(STELLA_DIR)/src/emucore/Booster.cxx \
	$(STELLA_DIR)/src/emucore/Cart.cxx \
	$(STELLA_DIR)/src/emucore/Cart0840.cxx \
	$(STELLA_DIR)/src/emucore/Cart2K.cxx \
	$(STELLA_DIR)/src/emucore/Cart3E.cxx \
	$(STELLA_DIR)/src/emucore/Cart3F.cxx \
	$(STELLA_DIR)/src/emucore/Cart4A50.cxx \
	$(STELLA_DIR)/src/emucore/Cart4K.cxx \
	$(STELLA_DIR)/src/emucore/Cart4KSC.cxx \
	$(STELLA_DIR)/src/emucore/CartAR.cxx \
	$(STELLA_DIR)/src/emucore/CartBF.cxx \
	$(STELLA_DIR)/src/emucore/CartBFSC.cxx \
	$(STELLA_DIR)/src/emucore/CartCM.cxx \
	$(STELLA_DIR)/src/emucore/CartCTY.cxx \
	$(STELLA_DIR)/src/emucore/CartCV.cxx \
	$(STELLA_DIR)/src/emucore/CartDF.cxx \
	$(STELLA_DIR)/src/emucore/CartDFSC.cxx \
	$(STELLA_DIR)/src/emucore/CartDPC.cxx \
	$(STELLA_DIR)/src/emucore/CartDPCPlus.cxx \
	$(STELLA_DIR)/src/emucore/CartE0.cxx \
	$(STELLA_DIR)/src/emucore/CartE7.cxx \
	$(STELLA_DIR)/src/emucore/CartEF.cxx \
	$(STELLA_DIR)/src/emucore/CartEFSC.cxx \
	$(STELLA_DIR)/src/emucore/CartF0.cxx \
	$(STELLA_DIR)/src/emucore/CartF4.cxx \
	$(STELLA_DIR)/src/emucore/CartF4SC.cxx \
	$(STELLA_DIR)/src/emucore/CartF6.cxx \
	$(STELLA_DIR)/src/emucore/CartF6SC.cxx \
	$(STELLA_DIR)/src/emucore/CartF8.cxx \
	$(STELLA_DIR)/src/emucore/CartF8SC.cxx \
	$(STELLA_DIR)/src/emucore/CartFA.cxx \
	$(STELLA_DIR)/src/emucore/CartFA2.cxx \
	$(STELLA_DIR)/src/emucore/CartFE.cxx \
	$(STELLA_DIR)/src/emucore/CartMC.cxx \
	$(STELLA_DIR)/src/emucore/CartSB.cxx \
	$(STELLA_DIR)/src/emucore/CartUA.cxx \
	$(STELLA_DIR)/src/emucore/CartX07.cxx \
	$(STELLA_DIR)/src/emucore/CompuMate.cxx \
	$(STELLA_DIR)/src/emucore/Console.cxx \
	$(STELLA_DIR)/src/emucore/Control.cxx \
	$(STELLA_DIR)/src/emucore/Driving.cxx \
	$(STELLA_DIR)/src/emucore/Genesis.cxx \
	$(STELLA_DIR)/src/emucore/Joystick.cxx \
	$(STELLA_DIR)/src/emucore/Keyboard.cxx \
	$(STELLA_DIR)/src/emucore/KidVid.cxx \
	$(STELLA_DIR)/src/emucore/M6502.cxx \
	$(STELLA_DIR)/src/emucore/M6532.cxx \
	$(STELLA_DIR)/src/emucore/MD5.cxx \
	$(STELLA_DIR)/src/emucore/MindLink.cxx \
	$(STELLA_DIR)/src/emucore/MT24LC256.cxx \
	$(STELLA_DIR)/src/emucore/NullDev.cxx \
	$(STELLA_DIR)/src/emucore/Paddles.cxx \
	$(STELLA_DIR)/src/emucore/Props.cxx \
	$(STELLA_DIR)/src/emucore/PropsSet.cxx \
	$(STELLA_DIR)/src/emucore/Random.cxx \
	$(STELLA_DIR)/src/emucore/SaveKey.cxx \
	$(STELLA_DIR)/src/emucore/Serializer.cxx \
	$(STELLA_DIR)/src/emucore/Settings.cxx \
	$(STELLA_DIR)/src/emucore/StateManager.cxx \
	$(STELLA_DIR)/src/emucore/Switches.cxx \
	$(STELLA_DIR)/src/emucore/System.cxx \
	$(STELLA_DIR)/src/emucore/Thumbulator.cxx \
	$(STELLA_DIR)/src/emucore/TIA.cxx \
	$(STELLA_DIR)/src/emucore/TIASnd.cxx \
	$(STELLA_DIR)/src/emucore/TIATables.cxx \
	$(STELLA_DIR)/src/emucore/TrackBall.cxx

LIBRETRO_SOURCES := libretro.cxx

SOURCES := $(LIBRETRO_SOURCES) $(STELLA_SOURCES)
OBJECTS := $(SOURCES:.cxx=.o) $(SOURCES_C:.c=.o)

all: $(TARGET)

ifeq ($(DEBUG),1)
FLAGS += -O0 -g
else
FLAGS += -O3 -ffast-math
endif

LDFLAGS += $(fpic) -lz $(SHARED)
FLAGS += $(fpic) 
FLAGS += -I. -Istella -Istella/src -Istella/stubs -Istella/src/emucore -Istella/src/common -Istella/src/common/tv_filters -Istella/src/gui

ifeq ($(OLD_GCC), 1)
WARNINGS := -Wall
else ifeq ($(NO_GCC), 1)
WARNINGS :=
else
WARNINGS := -Wall \
	-Wno-narrowing \
	-Wno-sign-compare \
	-Wno-unused-variable \
	-Wno-unused-function \
	-Wno-uninitialized \
	-Wno-unused-result \
	-Wno-strict-aliasing \
	-Wno-overflow \
	-fno-strict-overflow
endif

FLAGS += -D__LIBRETRO__ $(WARNINGS)

CXXFLAGS += $(FLAGS) -DHAVE_INTTYPES -DHAVE_GETTIMEOFDAY -DTHUMB_SUPPORT -DSOUND_SUPPORT -DBSPF_UNIX
CFLAGS += $(FLAGS) -std=gnu99

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(CXX) -o $@ $^ $(LDFLAGS)
endif

%.o: %.cxx
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.cpp
	$(CXX) -c -o $@ $< $(CXXFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
