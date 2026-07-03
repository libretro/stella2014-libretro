#include <ctime>
#include "OSystem.hxx"

OSystem::OSystem()
{
    myNVRamDir     = ".";
    mySettings     = 0;
    mySound        = new Sound(this);
    mySerialPort   = new SerialPort();
    myEventHandler = new EventHandler(this);
    myPropSet      = new PropertiesSet(this);
    Paddles::setDigitalSensitivity(50);
    Paddles::setMouseSensitivity(5);
}

OSystem::~OSystem()
{
    delete mySound;
    delete mySerialPort;
    delete myEventHandler;
    delete myPropSet;
}

bool OSystem::create() { return 1; }
void OSystem::stateChanged(EventHandler::State state) { }

uInt64 OSystem::getTicks() const
{
    // DETERMINISM INVARIANT: this must return *emulated* time, never
    // wall-clock time. Random::initSeed() seeds the RNG from getTicks(),
    // and that RNG initializes cartridge RAM contents (CartXX ctors) and
    // undriven TIA pins. Upstream Stella's getTicks() is wall-clock;
    // if a future sync ever restores that, RAM-init randomness stops
    // being reproducible and netplay/run-ahead/replay determinism
    // silently breaks. TIA::getMilliSeconds() is derived purely from
    // emulated frame counters, which is what keeps this deterministic.
    return myConsole->tia().getMilliSeconds();
}

EventHandler::EventHandler(OSystem*) { }
EventHandler::~EventHandler() { }
