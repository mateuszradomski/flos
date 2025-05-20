#include "./src/format.c"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

#define ANSI_RED   "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RESET "\x1b[0m"
#define ANSI_GREY  "\x1b[90m"

typedef unsigned long long u64;

int main(int argCount, char **args) {
    Arena arena = arenaCreate(128 * Megabyte, 32 * Kilobyte, 64);

    char *filepath = "tests/parserbuilding.sol";
    if(argCount == 2) {
        filepath = args[1];
    }

    String content = readFile(&arena, filepath);

    u64 elapsed = -readCPUTimer();
    size_t memoryUsed = arenaFreeBytes(&arena);
    String result = format(&arena, content);
    memoryUsed -= arenaFreeBytes(&arena);

    FILE *output = fopen("output.sol", "w");
    fwrite(result.data, result.size, 1, output);
    fclose(output);

    FILE *input = fopen("input.sol", "w");
    fwrite(content.data, content.size, 1, input);
    fclose(input);

    double cpuFreq = readCPUFrequency();
    elapsed += readCPUTimer();
    if(stringMatch(stringTrim(result), stringTrim(content))) {
        printf(ANSI_GREEN "The formatted output is the same as input!\n" ANSI_RESET);
    } else {
        printf(ANSI_RED "The formatted output is different from input!\n" ANSI_RESET);
    }

    printf("Done with throughput %f MB/s\n", ((content.size / ((double)elapsed / cpuFreq)) / 1e6));
    printf("Timings:\n");
    printf("  Tokenize:   %9llu cycles, %f ms\n", gCyclesTable[Measurement_Tokenize], (double)gCyclesTable[Measurement_Tokenize] / cpuFreq * 1e3);
    printf("  Parse:      %9llu cycles, %f ms\n", gCyclesTable[Measurement_Parse], (double)gCyclesTable[Measurement_Parse] / cpuFreq * 1e3);
    printf("  BuildDoc:   %9llu cycles, %f ms\n", gCyclesTable[Measurement_BuildDoc], (double)gCyclesTable[Measurement_BuildDoc] / cpuFreq * 1e3);
    printf("  RenderDoc:  %9llu cycles, %f ms\n", gCyclesTable[Measurement_RenderDoc], (double)gCyclesTable[Measurement_RenderDoc] / cpuFreq * 1e3);
    printf("  ---------\n");
    printf("  Sum:        %9llu cycles, %f ms\n", elapsed, (double)elapsed / cpuFreq * 1e3);
}
