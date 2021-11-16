#include <ctime>
#include "OSystem.hxx"

OSystem::OSystem()
{
    myNVRamDir     = ".";
    mySettings     = 0;
    myFrameBuffer  = new FrameBuffer();
    mySound        = new Sound(this);
    mySerialPort   = new SerialPort();
    myEventHandler = new EventHandler(this);
    myPropSet      = new PropertiesSet(this);
    Paddles::setDigitalSensitivity(50);
    Paddles::setMouseSensitivity(5);
}

OSystem::~OSystem()
{
    delete myFrameBuffer;
    delete mySound;
    delete mySerialPort;
    delete myEventHandler;
    delete myPropSet;
}

bool OSystem::create() { return 1; }

void OSystem::mainLoop() { }

void OSystem::pollEvent() { }

bool OSystem::queryVideoHardware() { return 1; }

void OSystem::stateChanged(EventHandler::State state) { }

void OSystem::setDefaultJoymap(Event::Type event, EventMode mode) { }

void OSystem::setFramerate(float framerate) { }

uInt64 OSystem::getTicks() const
{
    return myConsole->tia().getMilliSeconds();
}

EventHandler::EventHandler(OSystem*)
{
    
}

EventHandler::~EventHandler()
{
    
}

FrameBuffer::FrameBuffer()
{

}

FrameBuffer::~FrameBuffer()
{

}

FBInitStatus FrameBuffer::initialize(const string& title, uInt32 width, uInt32 height)
{
    return kSuccess;
}
