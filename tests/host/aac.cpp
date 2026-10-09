#include <Arduino.h>
#include "AudioFileSourceSTDIO.h"
#include "AudioOutputSTDIO.h"
#include "AudioGeneratorAAC.h"

#define AAC "../../examples/PlayAACFromPROGMEM/homer.aac"

int main(int argc, char **argv)
{
    const char *infile = (argc > 1) ? argv[1] : AAC;
    const char *outfile = (argc > 2) ? argv[2] : "out.aac.wav";
    AudioFileSourceSTDIO *in = new AudioFileSourceSTDIO(infile);
    AudioOutputSTDIO *out = new AudioOutputSTDIO();
    out->SetFilename(outfile);
    void *space = malloc(200000);
    AudioGeneratorAAC *aac = new AudioGeneratorAAC(space, 200000);

    printf("Opening infile=%s, outfile=%s\n", infile, outfile);
    if (!aac->begin(in, out)) {
        printf("aac->begin failed!\n");
    }
    int count = 0;
    while (aac->loop()) { count++; }
    printf("aac->loop finished, count=%d\n", count);
    aac->stop();

    delete aac;
    delete out;
    delete in;

    free(space);
}
