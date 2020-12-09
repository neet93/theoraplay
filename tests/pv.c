/*Play Video*/
#include <stdio.h>
#include <stdlib.h>
#include <theoraplay.h>

int main(int argc, char *argv[]) {
    THEORAPLAY_Decoder *dec = NULL;

    dec = THEORAPLAY_startDecodeFile(argv[1], 30, THEORAPLAY_VIDFMT_YV12);
    printf("Decoding: %s\n", argv[1]);

    return EXIT_SUCCESS;
}
