#include <ctime>
#include <sys/time.h>
#include "OSystem.hxx"

#if defined(_WIN32)
#include <windows.h>
#endif
#if defined(__CELLOS_LV2__)
#include <sys/sys_time.h>
#include <sys/time_util.h>
#endif

OSystem::OSystem()
{
    myNVRamDir = ".";
    mySettings = 0;
    myFrameBuffer = new FrameBuffer();
    vcsSound = new SoundSDL(this);
    mySound = vcsSound;
    mySerialPort = new SerialPort();
    myEventHandler = new EventHandler(this);
    myPropSet = new PropertiesSet(this);
    Paddles::setDigitalSensitivity(5);
    Paddles::setMouseSensitivity(5);
}

OSystem::~OSystem()
{
    
}

bool OSystem::create() { return 1; }

void OSystem::mainLoop() { }

void OSystem::pollEvent() { }

bool OSystem::queryVideoHardware() { return 1; }

void OSystem::stateChanged(EventHandler::State state) { }

void OSystem::setDefaultJoymap(Event::Type event, EventMode mode) { }

void OSystem::setFramerate(float framerate) { }
//void OSystem::setFramerate(float framerate)
//{
//    if(framerate > 0.0)
//    {
//        myDisplayFrameRate = framerate;
//        myTimePerFrame = (uInt32)(1000000.0 / myDisplayFrameRate);
//    }
//}

uInt64 OSystem::getTicks() const
{
#if defined(__CELLOS_LV2__)
	static uint64_t ulScale=0;
	uint64_t ulTime;

	if (ulScale==0)
		ulScale = sys_time_get_timebase_frequency() / 1000;

#ifdef __SNC__
	ulTime=__builtin_mftb();
#else
	asm __volatile__ ("mftb %0" : "=r" (ulTime) : : "memory");
#endif

	return ulTime/ulScale;
#elif defined(_WIN32)
	LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    unsigned long long ticksPerMillisecond = freq.QuadPart/1000;

	LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (unsigned long)(counter.QuadPart/ticksPerMillisecond);
#else
   timeval tim;
   gettimeofday(&tim, 0);
   return (tim.tv_sec*1000u)+(long)(tim.tv_usec/1000.0);
#endif
}

EventHandler::EventHandler(OSystem*)
{
    
}

EventHandler::~EventHandler()
{
    
}

FrameBuffer::FrameBuffer() { }

FBInitStatus FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
    //logMsg("called FrameBuffer::initialize, %d,%d", width, height);
    return kSuccess;
}

void FrameBuffer::refresh() { }

void FrameBuffer::showFrameStats(bool enable) { }

// 0 to <counts> - 1, i_s caches the value of counts
//#define iterateTimes(counts, i) for(unsigned int i = 0, i ## _s = counts; i < (i ## _s); i++)
//void FrameBuffer::setTIAPalette(const uInt32* palette)
//{
//}
