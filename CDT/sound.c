#include <windows.h>
#include <mmsystem.h>

void sound()
{
    PlaySoundA("trout.wav", NULL, SND_FILENAME | SND_LOOP);
    PlaySoundA(NULL, NULL, SND_PURGE);
}
