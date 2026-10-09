#include <Arduino.h>
#include "AudioFileSourceSTDIO.h"
#include "AudioOutputSTDIO.h"
#include "AudioGeneratorAAC.h"

#define AAC_DEFAULT "../../examples/PlayAACFromPROGMEM/homer.aac"

static bool TestDecodeFile(const char *infile, const char *outfile) {
    AudioFileSourceSTDIO *in = new AudioFileSourceSTDIO(infile);
    AudioOutputSTDIO *out = new AudioOutputSTDIO();
    out->SetFilename(outfile);
    void *space = malloc(200000);
    AudioGeneratorAAC *aac = new AudioGeneratorAAC(space, 200000);

    printf("Opening infile=%s, outfile=%s\n", infile, outfile);
    if (!aac->begin(in, out)) {
        printf("aac->begin failed for %s!\n", infile);
        delete aac;
        delete out;
        delete in;
        free(space);
        return false;
    }
    int count = 0;
    while (aac->loop()) { count++; }
    printf("aac->loop finished for %s, count=%d\n", infile, count);
    aac->stop();

    delete aac;
    delete out;
    delete in;
    free(space);
    return (count > 0);
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        const char *infile = argv[1];
        const char *outfile = (argc > 2) ? argv[2] : "out.aac.wav";
        return TestDecodeFile(infile, outfile) ? 0 : 1;
    }

    /* Test default AAC-LC file */
    bool ok = TestDecodeFile(AAC_DEFAULT, "out_lc.wav");

    /* Test HE-AAC v1 (SBR) if present */
    AudioFileSourceSTDIO test_v1("homer_he_v1.aac");
    if (test_v1.isOpen()) {
        ok = ok && TestDecodeFile("homer_he_v1.aac", "out_he_v1.wav");
    }

    /* Test HE-AAC v2 (PS) if present */
    AudioFileSourceSTDIO test_v2("homer_he_v2.aac");
    if (test_v2.isOpen()) {
        ok = ok && TestDecodeFile("homer_he_v2.aac", "out_he_v2.wav");
    }

    return ok ? 0 : 1;
}
