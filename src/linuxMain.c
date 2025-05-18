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

static u64
readCPUTimer()
{
#if defined(__x86_64__) || defined(_M_AMD64)
    return __rdtsc();
#else
    u64 tsc = 0;
    asm volatile("mrs %0, cntvct_el0" : "=r"(tsc));
    return tsc;
#endif
}

static u64
readCPUFrequency()
{
#if defined(__x86_64__) || defined(_M_AMD64)
    return 4.2e9;
#else
    return 24e6;
#endif
}

int main(int argCount, char **args) {
    u64 elapsed = -readCPUTimer();
    Arena arena = arenaCreate(128 * Megabyte, 32 * Kilobyte, 64);

    char *filepath = "tests/parserbuilding.sol";
    if(argCount == 2) {
        filepath = args[1];
    }

    String content = readFile(&arena, filepath);

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

    printf("Done in [%llu cycles][%f ms][%f MB/s]\n", elapsed, (double)elapsed / cpuFreq * 1e3, ((content.size / ((double)elapsed / cpuFreq)) / 1e6));
}
