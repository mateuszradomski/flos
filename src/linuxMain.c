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
#define ANSI_MAGENTA "\x1b[35m"

typedef unsigned long long u64;

typedef struct FormatMetrics {
    u64 fileRead;
    u64 tokenize;
    u64 parse;
    u64 buildDoc;
    u64 renderDoc;

    u64 memoryUsed;

    u64 inputBytes;
} FormatMetrics;

static FormatMetrics
formatMain(Arena *arena, char **paths, u32 pathCount) {
    FormatMetrics metrics = { 0 };

    for(int i = 0; i < pathCount; i++) {
        u64 startPosition = arenaPos(arena);

        metrics.fileRead -= readCPUTimer();
        String content = readFile(arena, paths[i]);
        metrics.fileRead += readCPUTimer();

        FormatResult result = format(arena, content);

        arenaPopTo(arena, startPosition);

        metrics.inputBytes += content.size;
        metrics.tokenize   += result.timings[Measurement_Tokenize];
        metrics.parse      += result.timings[Measurement_Parse];
        metrics.buildDoc   += result.timings[Measurement_BuildDoc];
        metrics.renderDoc  += result.timings[Measurement_RenderDoc];
    }

    return metrics;
}

typedef struct TestEntry {
    u64 minCycles;
	u64 maxCycles;
	u64 avgCycles;
	u64 runCount;
} TestEntry;

static void
repetitionTesterMain(Arena *arena, String content) {
    TokenizeResult tokens = tokenize(content, arena);
    Parser parser = createParser(tokens, arena);
    ASTNode node = parseSourceUnit(&parser);

    Render render = createRender(arena, content, tokens);
    Render cleanRender = render;

    u64 startedAt = readCPUTimer();
    u64 maxTiming = 0;
    u64 minTiming = (u64)(-1);
    u64 timingSum = 0;
    u64 timingCount = 0;
    u64 testDuration = 10 * readCPUFrequency();

    while(startedAt + testDuration > readCPUTimer()) {
        u64 timing = -readCPUTimer();
        buildDocument(&render, &node, content, tokens);
        timing += readCPUTimer();
        render = cleanRender;

        timingSum += timing;
        timingCount += 1;
        maxTiming = maxTiming > timing ? maxTiming : timing;
        if(timing < minTiming) {
            minTiming = timing;
            startedAt = readCPUTimer();
        }
    }

    TestEntry docBuild = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    buildDocument(&render, &node, content, tokens);

    startedAt = readCPUTimer();
    maxTiming = 0;
    minTiming = (u64)(-1);
    timingSum = 0;
    timingCount = 0;

    while(startedAt + testDuration > readCPUTimer()) {
        u64 timing = -readCPUTimer();
        renderDocument(&render);
        timing += readCPUTimer();
        render.writer = cleanRender.writer;

        timingSum += timing;
        timingCount += 1;
        maxTiming = maxTiming > timing ? maxTiming : timing;
        if(timing < minTiming) {
            minTiming = timing;
            startedAt = readCPUTimer();
        }
    }

    TestEntry docRender = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    double cpuFreq = readCPUFrequency();
    printf("BuildDoc:\n");
    printf("  Minimum     %9llu cycles, %f ms, %f cycles/b\n", docBuild.minCycles, (double)docBuild.minCycles / cpuFreq * 1e3, (double)docBuild.minCycles / content.size);
    printf("  Average     %9llu cycles, %f ms, %f cycles/b\n", docBuild.avgCycles, (double)docBuild.avgCycles / cpuFreq * 1e3, (double)docBuild.avgCycles / content.size);
    printf("  Maximum     %9llu cycles, %f ms, %f cycles/b\n", docBuild.maxCycles, (double)docBuild.maxCycles / cpuFreq * 1e3, (double)docBuild.maxCycles / content.size);
    printf("  ---------\n");
    printf("  Run count   %9llu\n", docBuild.runCount);
    printf("\n");
    printf("RenderDoc:\n");
    printf("  Minimum     %9llu cycles, %f ms, %f cycles/b\n", docRender.minCycles, (double)docRender.minCycles / cpuFreq * 1e3, (double)docRender.minCycles / content.size);
    printf("  Average     %9llu cycles, %f ms, %f cycles/b\n", docRender.avgCycles, (double)docRender.avgCycles / cpuFreq * 1e3, (double)docRender.avgCycles / content.size);
    printf("  Maximum     %9llu cycles, %f ms, %f cycles/b\n", docRender.maxCycles, (double)docRender.maxCycles / cpuFreq * 1e3, (double)docRender.maxCycles / content.size);
    printf("  ---------\n");
    printf("  Run count   %9llu\n", docRender.runCount);
}

int main(int argCount, char **args) {
    Arena arena = arenaCreate(128 * Megabyte, 32 * Kilobyte, 64);

    char *filepath = "tests/parserbuilding.sol";
    bool repetitionTester = false;

    if(argCount == 3) {
        assert(stringMatch((String){ .data = (u8 *)args[1], .size = strlen(args[1]) }, LIT_TO_STR("rep")));
        filepath = args[2];
        repetitionTester = true;
    }

    if(repetitionTester) {
        String content = readFile(&arena, filepath);
        repetitionTesterMain(&arena, content);
    } else {
        u64 elapsed = -readCPUTimer();
        FormatMetrics metrics = formatMain(&arena, args + 1, argCount - 1);
        elapsed += readCPUTimer();

        double cpuFreq = readCPUFrequency();
        printf(ANSI_MAGENTA "Formatted %d files in %f ms with throughput %f MB/s\n" ANSI_RESET, argCount - 1, (double)elapsed / cpuFreq * 1e3, ((metrics.inputBytes / ((double)elapsed / cpuFreq)) / 1e6));
        printf("Timings:\n");
        printf("  File read:  %9llu cycles, %f ms\n", metrics.fileRead,  (double)metrics.fileRead / cpuFreq * 1e3);
        printf("  Tokenize:   %9llu cycles, %f ms\n", metrics.tokenize,  (double)metrics.tokenize / cpuFreq * 1e3);
        printf("  Parse:      %9llu cycles, %f ms\n", metrics.parse,     (double)metrics.parse / cpuFreq * 1e3);
        printf("  BuildDoc:   %9llu cycles, %f ms\n", metrics.buildDoc,  (double)metrics.buildDoc / cpuFreq * 1e3);
        printf("  RenderDoc:  %9llu cycles, %f ms\n", metrics.renderDoc, (double)metrics.renderDoc / cpuFreq * 1e3);
        printf("  ---------\n");
        printf("  Sum:        %9llu cycles, %f ms\n", elapsed, (double)elapsed / cpuFreq * 1e3);
    }

}
