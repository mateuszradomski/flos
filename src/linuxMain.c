#include "./src/format.c"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

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

    char *content = readFile(&arena, filepath);

    size_t memoryUsed = arenaFreeBytes(&arena);
    String contentString = (String) { .data = content, .size = contentLength };
    String result = format(&arena, contentString);
    memoryUsed -= arenaFreeBytes(&arena);

    FILE *output = fopen("output.sol", "w");
    fwrite(result.data, result.size, 1, output);
    fclose(output);

    double cpuFreq = readCPUFrequency();
    elapsed += readCPUTimer();
    printf("Done in [%llu cycles][%f ms][%f MB/s]\n", elapsed, (double)elapsed / cpuFreq * 1e3, ((contentLength / ((double)elapsed / cpuFreq)) / 1e6));
}
