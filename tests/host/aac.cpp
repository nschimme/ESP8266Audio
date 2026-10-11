#include <Arduino.h>
#include "AudioFileSourceSTDIO.h"
#include "AudioOutputSTDIO.h"
#include "AudioGeneratorAAC.h"
#include <string>

struct TestCase {
    const char *name;
    const char *infile;
    const char *outfile;
};

static std::string DeriveOutputFilename(const std::string &infile) {
    size_t dot_pos = infile.find_last_of('.');
    size_t slash_pos = infile.find_last_of("/\\");

    /* Ensure dot belongs to extension, not parent directory */
    if (dot_pos != std::string::npos && (slash_pos == std::string::npos || dot_pos > slash_pos)) {
        return infile.substr(0, dot_pos) + ".wav";
    }
    return infile + ".wav";
}

static std::string ResolveInputPath(const char *rel_path) {
    AudioFileSourceSTDIO test1(rel_path);
    if (test1.isOpen()) {
        return rel_path;
    }

    /* Handle path resolution whether invoked from repo root or tests/host/ */
    std::string path_str = rel_path;
    if (path_str.rfind("../../", 0) == 0) {
        std::string stripped = path_str.substr(6);
        AudioFileSourceSTDIO test2(stripped.c_str());
        if (test2.isOpen()) {
            return stripped;
        }
    } else {
        std::string prepended = "../../" + path_str;
        AudioFileSourceSTDIO test3(prepended.c_str());
        if (test3.isOpen()) {
            return prepended;
        }
    }

    return rel_path;
}

static bool RunDecodeTest(const TestCase &tc) {
    std::string resolved_in = ResolveInputPath(tc.infile);
    AudioFileSourceSTDIO in(resolved_in.c_str());
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

    printf("=== Running Test: %s [%s -> %s] ===\n", tc.name, resolved_in.c_str(), tc.outfile);
    if (!aac.begin(&in, &out)) {
        printf("ERROR: aac.begin failed for %s!\n", resolved_in.c_str());
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
        std::string infile = argv[1];
        std::string outfile = (argc > 2) ? argv[2] : DeriveOutputFilename(infile);
        TestCase custom_tc = {
            "Custom Input",
            infile.c_str(),
            outfile.c_str()
        };
        return RunDecodeTest(custom_tc) ? 0 : 1;
    }

    const TestCase test_suite[] = {
        {"AAC-LC Default Sample", "examples/PlayAACFromPROGMEM/homer-lc.aac",  "out_lc.wav"},
        {"HE-AAC v1 SBR Sample",  "examples/PlayAACFromPROGMEM/homer-sbr.aac", "out_sbr.wav"},
        {"HE-AAC v2 PS Sample",   "examples/PlayAACFromPROGMEM/homer-ps.aac",  "out_ps.wav"},
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
