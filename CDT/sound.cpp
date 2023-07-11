#include <windows.h>
#include <mmsystem.h>

void sound()
{
    PlaySoundA("close2u.wav", NULL, SND_FILENAME | SND_LOOP);
    PlaySoundA(NULL, NULL, SND_PURGE);
}
