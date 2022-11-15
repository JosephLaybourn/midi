// MidiInTest.cpp
// -- simple test of MidiIn framework
// cs245 1/18
//
// usage:
//   MidInTest [<in> <out>]
// where:
//   <in>  -- device number of a loopback MIDI port(*)
//   <out> -- device number of a loopback MIDI port(*)
// notes:
//   --  If no arguments are given, a list of the input and
//       output ports is printed.
//   (*) It is assumed that the program 'loopMIDI' is running,
//       and that <in> and <out> are the device numbers of
//       virtual MIDI ports (possibly the same port) created by
//       'loopMIDI'.

#include <iostream>
#include <cstdlib>
#include <exception>
#include "MidiIn.h"
using namespace std;


struct MyMidiIn : MidiIn {
  MyMidiIn(int n) : MidiIn(n) { }
  void onNoteOn(int channel, int note, int velocity) override {
    cout <<  "note on: " << channel << ' '
         << note << ' ' << velocity << endl;
  }
  void onNoteOff(int channel, int note) override {
    cout << "note off: " << channel << ' ' << note << endl;
  }
  void onPitchWheelChange(int channel, float value) override {
    cout << "pitch wheel: " << channel << ' ' << value << endl;
  }
  void onVolumeChange(int channel, int level) override {
    cout << "volume: " << channel << ' ' << level << endl;
  }
  void onModulationWheelChange(int channel, int value) override {
    cout << "mod wheel: " << channel << ' ' << value << endl;
  }
  void onControlChange(int channel, int number, int value) override {
    cout << "control: " << channel << ' '
         << number << ' ' << value << endl;
  }
  void onPatchChange(int channel, int value) override {
    cout << "patch: " << channel << ' ' << value << endl;
  }
};


int main(int argc, char *argv[]) {

  if (argc == 1) {
    cout << "*** Available Devices ***" << endl;
    cout << MidiIn::getDeviceInfo() << endl;
    return 0;
  }

  if (argc != 3)
    return 0;

  int num_in = atoi(argv[1]),
      num_out = atoi(argv[2]);

  MidiIn *input;
  try {
    input = new MyMidiIn(num_in);
  }
  catch (exception &e) {
    cout << e.what() << endl;
    return 0;
  }

  // Note that PortMidi is already initialized as a result
  //   of instantiating 'MyMidiIn'
  PmStream *output_stream;
  PmError value = Pm_OpenOutput(&output_stream,num_out,0,4,0,0,0);
  if (value != pmNoError) {
    delete input;
    cout << "failed to open output device" << endl;
    return -1;
  }

  input->start();
  long msgs[] = { 0x00633291,
                  0x00003291,
                  0x00001A82,
                  0x00140EE3,
                  0x006407B4,
                  0x005301B5,
                  0x002A02B6,
                  0x00003DCB };
  for (int i=0; i < sizeof(msgs)/sizeof(long); ++i)
    Pm_WriteShort(output_stream,0,msgs[i]);

  cin.get();
  input->stop();

  Pm_Close(output_stream);
  delete input;
  return 0;
}
