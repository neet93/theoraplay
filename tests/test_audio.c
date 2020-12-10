/* ** What's the purpose? **
 * In order to understand how Theoraplay works we should start here with just simeple audio playback.
 * 
 * ** How Audio Works **
 * Audio is stored on files(in this case a WAV file). When that file is loaded, information about that file is used as "userdata" for a callback.
 * A device is opened, and audio is paused by default. When audio is set to play, the callback is usually processed in a seperate thread, it
 * "streams" data of a fixed length to the device. How this data gets streamed is left up to you in the callback. Using this flexible callback
 * method, you can not only playback audio, but you can mix audio effects, & do other things. Audio data is represented as a queue, and processed
 * one chunk(sample) at a time.
*/

#include <SDL.h>

#define LOG_ERROR SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());

static SDL_bool is_running = SDL_TRUE;

/*Represents properties needed for wave audio data playback.*/
typedef struct _WaveObj{
    Uint8 *data;     /*Audio data.*/
    Uint32 length;   /*Size of the audio data.*/
    int pos;         /*Current play position.*/
}WaveObj;

/*Simple playback method.*/
void PlaybackWaveBlock(void *data, Uint8 *stream, int length) {
    WaveObj *wav = (WaveObj *)data;
    Uint8 *waveptr = NULL;
    int waveleft = 0;

    /*Updates the current sample by offsetting the WAV data's position in the queue.*/
    waveptr = wav->data + wav->pos;
    /*Updates the amount of audio left to playback by subtracting the length of the WAV data by the current position in the queue.*/
    waveleft = wav->length - wav->pos;

    /*If the amount of WAV data left to play is less than the amount of wav data we can process, then we must be done!*/
    if(waveleft < length) {
        is_running = SDL_FALSE;
        return;
    }

    /*Copy the current chunk of data into the audio "stream", of a fixed length.*/
    SDL_memcpy(stream, waveptr, length);
    /*Update the current position, by offsetting it by the fixed length needed to copy the data for playback.*/
    wav->pos += length;
}

int main(int argc, char *argv[]) {
    const char *filename = "audio_test.wav";
    WaveObj wav = {0};
    SDL_AudioSpec spec = {0};
    SDL_AudioDeviceID dev = {0};

    if(SDL_Init(SDL_INIT_AUDIO) != 0) {
        LOG_ERROR
        return EXIT_FAILURE;
    }

    /*Loads the file, and sets the data needed for playback into their respective buffers.*/
    if(!SDL_LoadWAV(filename, &spec, &wav.data, &wav.length)) {
        LOG_ERROR
        goto cleanup;
    }

    spec.callback = PlaybackWaveBlock;
    spec.userdata = &wav;

    /*Opens a device for playback only!*/
    if(!(dev = SDL_OpenAudioDevice(NULL, SDL_FALSE, &spec, NULL, 0))) {
        LOG_ERROR
        goto cleanup;
    }

    /*Unpause the audio, so that out callback can play some music!*/
    SDL_PauseAudioDevice(dev, SDL_FALSE);

    while(is_running) SDL_Delay(100); /*Arbitrary delay, the loop has to be there until the audio playback is done.*/

cleanup:
    SDL_CloseAudioDevice(dev);
    SDL_FreeWAV(wav.data);
    SDL_Quit();

    return EXIT_SUCCESS;
}
