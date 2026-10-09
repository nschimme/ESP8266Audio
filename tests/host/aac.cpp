#include <Arduino.h>
#include "AudioFileSourceSTDIO.h"
#include "AudioOutputSTDIO.h"
#include "AudioGeneratorAAC.h"

struct TestCase {
    const char *name;
    const char *infile;
    const char *outfile;
};

static bool RunDecodeTest(const TestCase &tc) {
    AudioFileSourceSTDIO in(tc.infile);
    if (!in.isOpen()) {
        printf("ERROR: Could not open test sample file: %s\n", tc.infile);
        return false;
    }

    AudioOutputSTDIO out;
    out.SetFilename(tc.outfile);

    void *heap_space = malloc(200000);
    if (!heap_space) {
        printf("Failed to allocate decoder memory for test: %s\n", tc.name);
        return false;
    }

    AudioGeneratorAAC aac(heap_space, 200000);

    printf("=== Running Test: %s [%s -> %s] ===\n", tc.name, tc.infile, tc.outfile);
    if (!aac.begin(&in, &out)) {
        printf("ERROR: aac.begin failed for %s!\n", tc.infile);
        free(heap_space);
        return false;
    }

    int frame_count = 0;
    while (aac.loop()) {
        frame_count++;
    }
    aac.stop();

    free(heap_space);

    printf("SUCCESS: %s decoded %d frames successfully.\n\n", tc.name, frame_count);
    return (frame_count > 0);
}

int main(int argc, char **argv)
{
    if (argc > 1) {
        TestCase custom_tc = {
            "Custom Input",
            argv[1],
            (argc > 2) ? argv[2] : "out.aac.wav"
        };
        return RunDecodeTest(custom_tc) ? 0 : 1;
    }

    const TestCase test_suite[] = {
        {"AAC-LC Default Sample", "../../examples/PlayAACFromPROGMEM/homer-lc.aac", "out_lc.wav"},
        {"HE-AAC v1 SBR Sample",  "../../examples/PlayAACFromPROGMEM/homer-he-v1.aac", "out_he_v1.wav"},
        {"HE-AAC v2 PS Sample",   "../../examples/PlayAACFromPROGMEM/homer-he-v2.aac", "out_he_v2.wav"},
    };

    bool all_passed = true;
    size_t num_tests = sizeof(test_suite) / sizeof(test_suite[0]);

    for (size_t i = 0; i < num_tests; i++) {
        if (!RunDecodeTest(test_suite[i])) {
            all_passed = false;
        }
    }

    return all_passed ? 0 : 1;
}
