/*
 * sound.cpp
 *
 * Encapsulates sound capabilities.
 */

#include <thread>
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <iostream>

namespace framework
{
  void sound(bool loop)
  {
    if (loop)
    {
        PlaySoundA("close2u.wav", NULL, SND_FILENAME | SND_LOOP);
    }
    else
    {/*
        // Launch in a separate thread for non-looping sound
        std::thread([]()
        {
            PlaySoundA("close2u.wav", NULL, SND_FILENAME);
            PlaySoundA(NULL, NULL, SND_PURGE); // Ensure resources are released
        }).detach(); // Detach thread to make it temporary
        */
        PlaySoundA("close2u.wav", NULL, SND_FILENAME);
    }
  }
}
