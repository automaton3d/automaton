/*
 * sound.cpp
 *
 * Encapsulates sound capabilities.
 */

#include <windows.h>
#include <mmsystem.h>

namespace framework
{
	void sound()
	{
		PlaySoundA("close2u.wav", NULL, SND_FILENAME | SND_LOOP);
		PlaySoundA(NULL, NULL, SND_PURGE);
	}
}
