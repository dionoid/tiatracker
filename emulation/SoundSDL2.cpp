//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SoundSDL2.cxx 3205 2015-09-14 21:33:50Z stephena $
//============================================================================

#include <sstream>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <SDL.h>

#include "TIASnd.h"
#include "SoundSDL2.h"

namespace Emulation {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::SoundSDL2(TIASound *tiasound)
  : myTIASound(tiasound),
    myIsEnabled(false),
    myIsInitializedFlag(false),
    myNumChannels(0),
    myIsMuted(true),
    myVolume(100)
{
  // The sound system is opened only once per program run, to eliminate
  // issues with opening and closing it multiple times
  // This fixes a bug most prevalent with ATI video cards in Windows,
  // whereby sound stopped working after the first video change
  SDL_AudioSpec desired{};
  desired.freq   = 44100;
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples  = 1024;
  desired.callback = callback;
  desired.userdata = static_cast<void*>(this);

  // The callback generates signed 16-bit mono/stereo samples. Passing an
  // obtained spec would allow SDL to change that format (e.g. to WASAPI's
  // 32-bit float format). Let SDL convert to the device format instead.
  if(SDL_OpenAudio(&desired, nullptr) < 0)
  {
    std::cerr << "WARNING: Couldn't open SDL audio system!\n"
        << "         " << SDL_GetError() << "\n";
    return;
  }

  // With no obtained spec, SDL updates desired with the callback buffer size.
  myHardwareSpec = desired;

  // Make sure the sample buffer isn't to big (if it is the sound code
  // will not work so we'll need to disable the audio support)
  if((float(myHardwareSpec.samples) / float(myHardwareSpec.freq)) >= 0.25)
  {
    std::cerr << "WARNING: Sound device doesn't support realtime audio! Make "
        << "sure a sound\n"
        << "         server isn't running.  Audio is disabled.\n";
    SDL_CloseAudio();
    return;
  }

  myIsInitializedFlag = true;
  SDL_PauseAudio(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::~SoundSDL2()
{
  // Close the SDL audio system if it's initialized
  if(myIsInitializedFlag)
  {
    SDL_CloseAudio();
    myIsEnabled = myIsInitializedFlag = false;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setEnabled(bool)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::open()
{
  mute(true);
  myIsEnabled = false;
  if(!myIsInitializedFlag)
  {
    return;
  }

  // Now initialize the TIASound object which will actually generate sound
  myTIASound->outputFrequency(myHardwareSpec.freq);
  const string& chanResult =
      myTIASound->channels(myHardwareSpec.channels, myNumChannels == 2);

  // Adjust volume to that defined in settings
  myVolume = 100;
  setVolume(myVolume);

  // And start the SDL sound subsystem ...
  myIsEnabled = true;
  mute(false);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::close()
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(1);
    myIsEnabled = false;
    myRenderedSamples = 0;
    myNextFrameSample = 0.0;
    myScheduleStarted = false;
    myTIASound->reset();
    myRegWriteQueue.clear();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::mute(bool state)
{
  if(myIsInitializedFlag)
  {
    myIsMuted = state;
    SDL_PauseAudio(myIsMuted ? 1 : 0);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::reset()
{
  if(myIsInitializedFlag)
  {
    SDL_PauseAudio(1);
    myRenderedSamples = 0;
    myNextFrameSample = 0.0;
    myScheduleStarted = false;
    myTIASound->reset();
    myRegWriteQueue.clear();
    mute(myIsMuted);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setVolume(Int32 percent)
{
  if(myIsInitializedFlag && (percent >= 0) && (percent <= 100))
  {
    SDL_LockAudio();
    myVolume = percent;
    myTIASound->volume(percent);
    SDL_UnlockAudio();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::adjustVolume(Int8 direction)
{
  string message;

  Int32 percent = myVolume;

  if(direction == -1)
    percent -= 2;
  else if(direction == 1)
    percent += 2;

  if((percent < 0) || (percent > 100))
    return;

  setVolume(percent);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setChannels(uInt32 channels)
{
  if(channels == 1 || channels == 2)
    myNumChannels = channels;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::setFrameRate(float framerate)
{
  assert(framerate > 0);
  myFrameRate = framerate;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::set(uInt16 addr, uInt8 value)
{
  assert(addr >= AUDC0 && addr <= AUDV1);
  myRegisters[addr - AUDC0] = value;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::endFrame()
{
  if(!myIsInitializedFlag)
    return;

  SDL_LockAudio();
  // Keep two callback buffers of lead to absorb normal scheduler jitter.
  // After an underrun, restart in the future instead of collapsing late
  // frames onto the same sample. Subsequent catch-up frames retain spacing.
  if(!myScheduleStarted || myNextFrameSample < myRenderedSamples)
  {
    myNextFrameSample = myRenderedSamples + 2 * myHardwareSpec.samples;
    myScheduleStarted = true;
  }

  const uInt64 sample = static_cast<uInt64>(std::llround(myNextFrameSample));
  for(uInt16 addr = AUDC0; addr <= AUDV1; ++addr)
    myRegWriteQueue.enqueue({addr, myRegisters[addr - AUDC0], sample});
  myNextFrameSample += myHardwareSpec.freq / myFrameRate;
  SDL_UnlockAudio();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::processFragment(Int16* stream, uInt32 length)
{
  const uInt32 channels = myHardwareSpec.channels;
  uInt32 remaining = length / channels;
  while(remaining > 0)
  {
    // Apply all writes at this boundary before generating its first sample.
    while(myRegWriteQueue.size() &&
          myRegWriteQueue.front().sample <= myRenderedSamples)
    {
      const RegWrite& info = myRegWriteQueue.front();
      myTIASound->set(info.addr, info.value);
      myRegWriteQueue.dequeue();
    }

    uInt32 count = remaining;
    if(myRegWriteQueue.size())
      count = static_cast<uInt32>(std::min<uInt64>(
          remaining, myRegWriteQueue.front().sample - myRenderedSamples));
    myTIASound->process(stream, count);
    stream += count * channels;
    remaining -= count;
    // The audio timeline keeps advancing even when the producer is late.
    myRenderedSamples += count;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::callback(void* udata, uInt8* stream, int len)
{
  SoundSDL2* sound = static_cast<SoundSDL2*>(udata);
  if(sound->myIsEnabled)
  {
    // SDL supplies a byte count for our signed 16-bit sample buffer.
    sound->processFragment(reinterpret_cast<Int16*>(stream), uInt32(len) >> 1);
  }
  else
    SDL_memset(stream, 0, len);  // Write 'silence'
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::RegWriteQueue::RegWriteQueue(uInt32 capacity)
  : myCapacity(capacity),
    myBuffer(0),
    mySize(0),
    myHead(0),
    myTail(0)
{
    myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
    grow();

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundSDL2::RegWrite& SoundSDL2::RegWriteQueue::front() const
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundSDL2::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundSDL2::RegWriteQueue::grow()
{
  RegWrite *buffer = new RegWrite[myCapacity*2];
  for(uInt32 i = 0; i < mySize; ++i) {
      buffer[i] = myBuffer[(myHead + i) % myCapacity];
  }

  myHead = 0;
  myTail = mySize;
  myCapacity *= 2;
  delete[] myBuffer;
  myBuffer = buffer;
}

}
