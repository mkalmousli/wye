/* PipeWire/PulseAudio give new USB sound cards 40-50% software volume; the bridge needs 100%. */
#include "plat_sys.h"
#include "proc.h"

void plat_audio_full_volume(const char *id, bool capture)
{
    const char *vol[] = {"pactl", capture ? "set-source-volume" : "set-sink-volume", id, "100%", NULL};
    const char *mute[] = {"pactl", capture ? "set-source-mute" : "set-sink-mute", id, "0", NULL};
    posix_run(vol);
    posix_run(mute);
}
