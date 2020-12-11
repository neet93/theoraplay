#include <SDL.h>
#include <theoraplay.h>

#define LOG_ERROR SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", SDL_GetError());

static Uint32 baseticks = 0;

typedef struct AudioQueue {
    const THEORAPLAY_AudioPacket *audio;
    int offset;
    struct AudioQueue *next;
} AudioQueue;

static volatile AudioQueue *audio_queue = NULL;
static volatile AudioQueue *audio_queue_tail = NULL;

static void SDLCALL audio_callback(void *userdata, Uint8 *stream, int len) {
    Sint16 *dst = (Sint16 *)stream;

    while(audio_queue && (len > 0)) {
        volatile AudioQueue *item = audio_queue;
        AudioQueue *next = item->next;
        const int channels = item->audio->channels;

        const float *src = item->audio->samples + (item->offset * channels);
        int cpy = (item->audio->frames - item->offset) * channels;
        int i;

        if (cpy > (len / sizeof(Sint16)))
            cpy = len / sizeof(Sint16);

        for (i = 0; i < cpy; i++) {
            const float val = *(src++);
            if (val < -1.0f)
                *(dst++) = -32768;
            else if (val > 1.0f)
                *(dst++) = 32767;
            else
                *(dst++) = (Sint16)(val * 32767.0f);
        }

        item->offset += (cpy / channels);
        len -= cpy * sizeof(Sint16);

        if (item->offset >= item->audio->frames) {
            THEORAPLAY_freeAudio(item->audio);
            SDL_free((void *)item);
            audio_queue = next;
        }
    }

    if(!audio_queue) audio_queue_tail = NULL;

    if(len > 0) memset(dst, '\0', len);
}

static void queue_audio(const THEORAPLAY_AudioPacket *audio) {
    AudioQueue *item = (AudioQueue *)SDL_malloc(sizeof(AudioQueue));
    if (!item) {
        THEORAPLAY_freeAudio(audio);
        return;
    }

    item->audio = audio;
    item->offset = 0;
    item->next = NULL;

    SDL_LockAudio();
    if (audio_queue_tail)
        audio_queue_tail->next = item;
    else
        audio_queue = item;
    audio_queue_tail = item;
    SDL_UnlockAudio();
}

SDL_bool PlayMovie(const char *path, SDL_Window *window, SDL_Renderer *renderer) {
    THEORAPLAY_Decoder *decoder = NULL;
    const THEORAPLAY_AudioPacket *audio = NULL;
    const THEORAPLAY_VideoFrame *video = NULL;
    SDL_Texture *texture = NULL;
    SDL_AudioSpec spec;
    SDL_Event event;
    Uint32 framems = 0;
    int initfailed = 0;
    int quit = 0;
    int audio_ret = 0;
    SDL_bool ret = SDL_TRUE;

    /*Initializes the decoder state used to playback both audio, & video.
     *The state initialized here is needed for every other THEORAPLAY function.*/
    if(!(decoder = THEORAPLAY_startDecodeFile(path, 30, THEORAPLAY_VIDFMT_IYUV))) {
        SDL_SetError("Failed to decode file");
        return SDL_FALSE;
    }

    /*Get the pointer to the head of the audio, & video linked lists.*/
    while(!audio || !video) {
        if(!audio) audio = THEORAPLAY_getAudio(decoder);
        if(!video) video = THEORAPLAY_getVideo(decoder);
        SDL_Delay(10);
    }

    /*Get playback start time in milliseconds.*/
    framems = (video->fps == 0.0) ? 0 : ((Uint32)(1000.0 / video->fps));

    /*Gets a Texture of the proper pixel format, and set for streaming, to play video frames.*/
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, video->width, video->height);

    /*Get initialization status to make sure we have a window, & texture to draw to!*/
    initfailed = quit = (!window || !texture);

    /*Opens an audio device driver to play out video's audio, with the desired specs. If the desired specs can't be met, the specs are modified.*/
    memset(&spec, '\0', sizeof(SDL_AudioSpec));
    spec.freq = audio->freq;
    spec.format = AUDIO_S16SYS;
    spec.channels = audio->channels;
    spec.samples = 2048;
    spec.callback = audio_callback;

    if((audio_ret = SDL_OpenAudio(&spec, NULL)) != 0) return SDL_FALSE;

    /*Get initialization status to make sure we have an audio device to play to!*/
    initfailed = quit = (initfailed || (audio_ret != 0));

    /*Smallest unit of a video frame.*/
    void *pixels = NULL;
    /*The length of one row of pixels in bytes.*/
    int pitch = 0;

    /*Fills the audio queue.*/
    while(audio) {
        /*Add it to the queue for playback in the callback!(Makes more sense if read the next comment...)*/
        queue_audio(audio);
        /*Gets the next audio packet in line.*/
        audio = THEORAPLAY_getAudio(decoder);
    }

    baseticks = SDL_GetTicks();

    /*If everything up there was properly initialized, then we're ready to start play audio!*/
    if(!quit) SDL_PauseAudio(0);

    while(!quit && THEORAPLAY_isDecoding(decoder)) {
        /*Get elapsed time.*/
        const Uint32 now = SDL_GetTicks() - baseticks;

        /*Get the next frame from the list.*/
        if(!video) video = THEORAPLAY_getVideo(decoder);

        /*If there is a fame, & there are frames left to play perform operations below.*/
        if(video && (video->playms <= now)) {

            /*???*/
            if(framems && ((now - video->playms) >= framems)) {
                const THEORAPLAY_VideoFrame *last = video;
                while ((video = THEORAPLAY_getVideo(decoder)) != NULL) {
                    THEORAPLAY_freeVideo(last);
                    last = video;
                    if((now - video->playms) < framems) break;
                }

                if(!video) video = last;
            }

            /*Copies frame data into memory for streaming, conforming to YUV color space.*/
            SDL_LockTexture(texture, NULL, &pixels, &pitch);
            const int w = video->width;
            const int h = video->height;
            /*Get luminance from the current frames pixels(its grays).*/
            const Uint8 *y = (const Uint8 *)video->pixels;
            /*Gets the chrominance for the frame by using the light value(y/luminance).*/
            const Uint8 *u = y + (w * h);
            const Uint8 *v = u + ((w / 2) * (h / 2));
            /*Set a buffer to copy into.*/
            Uint8 *dst = (Uint8*)pixels;

            for(int i = 0; i < h; i++, y += w, dst += pitch) memcpy(dst, y, w);

            for(int i = 0; i < h / 2; i++, u += w / 2, dst += pitch / 2) memcpy(dst, u, w / 2);

            for(int i = 0; i < h / 2; i++, v += w / 2, dst += pitch / 2) memcpy(dst, v, w / 2);

            SDL_UnlockTexture(texture);

            THEORAPLAY_freeVideo(video);
            video = NULL;
        }
        else {
            SDL_Delay(10);
        }

        /*Get more audio packets.*/
        while((audio = THEORAPLAY_getAudio(decoder)) != NULL) queue_audio(audio);

        while(window && SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                quit = 1;
                break;
            case SDL_KEYDOWN:
                quit = 1;
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                quit = 1;
                break;
            }
        }

        /*Draw the texture created above!*/
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    while (!quit) {
        SDL_LockAudio();
        quit = (audio_queue == NULL);
        SDL_UnlockAudio();
        if (!quit)
            SDL_Delay(100);
    }


    if(initfailed){
        SDL_SetError("Theora initialization failed!\n");
        ret = SDL_FALSE;
    }
    else if(THEORAPLAY_decodingError(decoder)){
        SDL_SetError("There was an error decoding this file!\n");
        ret = SDL_FALSE;
    }


    if(texture) SDL_DestroyTexture(texture);
    if(video) THEORAPLAY_freeVideo(video);
    if(audio) THEORAPLAY_freeAudio(audio);
    if(decoder) THEORAPLAY_stopDecode(decoder);

    return ret;
}

int main(int argc, char *argv[]) {
    const char *path = "video_test.ogv";
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS) != 0) {
        LOG_ERROR
        return EXIT_FAILURE;
    }

    if(SDL_CreateWindowAndRenderer(640, 480, SDL_WINDOW_SHOWN, &window, &renderer) != 0) {
        LOG_ERROR
        goto cleanup;
    }

    PlayMovie(path, window, renderer);

cleanup:
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}
