#include "MidiIn.h"
#include <stdexcept>
#include <iostream>

std::string MidiIn::getDeviceInfo(void)
{
  // change this into correct formating
  Pm_Initialize();
  std::string infoString;
  int n = Pm_CountDevices();

  for (int i = 0; i < n; ++i) 
  {
    const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
    infoString += std::to_string(i) + ": " + info->name + "\n";
  }

  Pm_Terminate();
  return infoString;
}

MidiIn::MidiIn(int devno)
{
  // (1) initialize PortMidi
  Pm_Initialize();
  
  // (2) open a PortMidi stream for MIDI input 
  PmError result = Pm_OpenInput(&input_stream, devno, 0, 64, 0, 0);
  if (result != pmNoError) 
  {
    throw std::runtime_error("Device cannot be opened\n");
  }

  // (3) create a thread to poll for MIDI messages
  event_thread = new std::thread(eventLoop, this);
}

void MidiIn::start()
{
  thread_running = true;
}

void MidiIn::stop()
{
  thread_running = false;
}

MidiIn::~MidiIn(void)
{
  // (1) stop the thread and wait for it to ﬁnish
  stop();
  event_thread->join();
  delete event_thread;
  
  // (2) close the PortMidi stream
  Pm_Close(input_stream);
  
  // (3) terminate PortMidi.
  Pm_Terminate();
}

void MidiIn::eventLoop(MidiIn * midiin_ptr)
{
  // fetch and process messages until kill flag is set
  while (midiin_ptr->thread_running) 
  {
    
    // is there a message?
    if (Pm_Poll(midiin_ptr->input_stream)) 
    {
      // fetch and process message
      union 
      {
        long message;
        unsigned char byte[4];
      } msg;
      PmEvent event;
      Pm_Read(midiin_ptr->input_stream, &event, 1);
      msg.message = event.message;
      

      //debug code
      //std::cout << std::hex << int(msg.byte[0]) << 'h' << std::dec << ' ' << int(msg.byte[1]) << ' ' << int(msg.byte[2]) << std::endl;

      switch (msg.byte[0] & 0xF0)
      {
      case 0x90:
      {
        midiin_ptr->onNoteOn(int(msg.byte[0] & 0x0F), int(msg.byte[1]), int(msg.byte[2]));
        break;
      }

      case 0x80:
      {
        midiin_ptr->onNoteOff(int(msg.byte[0] & 0x0F), int(msg.byte[1]));
        break;
      }

      case 0xE0:
      {
        short rawData = (msg.byte[2] * 0x80) + msg.byte[1];
        float data = -(float(rawData) - 0x2000) * (1 / 0x4000 - 1);

        midiin_ptr->onPitchWheelChange(int(msg.byte[0] & 0x0F), (data / 0x2000));
        break;
      }

      case 0xC0:
      {
        midiin_ptr->onPatchChange(int(msg.byte[0] & 0x0F), int(msg.byte[1]));
        break;
      }

      case 0xB0:
      {
        midiin_ptr->onControlChange(int(msg.byte[0] & 0x0F), int(msg.byte[1]), int(msg.byte[2]));

        switch (msg.byte[1])
        {
        case 0x1:
        {
          midiin_ptr->onModulationWheelChange(int(msg.byte[0] & 0x0F), msg.byte[2]);
          break;
        }
        case 0x7:
        {
          midiin_ptr->onVolumeChange(int(msg.byte[0] & 0x0F), msg.byte[2]);
          break;
        }
        }
        break;
      }
      }
    }
  }
  while (!midiin_ptr->thread_running);
}
