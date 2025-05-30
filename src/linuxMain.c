#include "./src/format.c"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED   "\x1b[31m"
#define ANSI_CYAN  "\x1b[36m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_GREY  "\x1b[90m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"

#define ANSI_CYAN_LIGHT "\x1b[96m"
#define ANSI_GREEN_LIGHT "\x1b[92m"
#define ANSI_MAGENTA_LIGHT "\x1b[95m"

#ifdef __linux__
#include <sys/sysinfo.h>
static u32
getProcessorCount() {
    return get_nprocs();
}
#elif __APPLE__
#include <sys/sysctl.h>
static u32
getProcessorCount() {
    int mib[] = { CTL_HW, HW_NCPU };

    int numCPU;
    size_t len = sizeof(numCPU);
    sysctl(mib, 2, &numCPU, &len, NULL, 0);

    return numCPU;
}
#endif

typedef struct FormatMetrics {
    u64 fileRead;
    u64 tokenize;
    u64 parse;
    u64 buildDoc;
    u64 renderDoc;

    u64 memoryUsed;

    u64 inputBytes;
} FormatMetrics;

typedef struct {
    char **paths;
    u32     pathCount;
    _Atomic u32 next;   /* atomic index of the next file to process */
} WorkQueue;

typedef struct ThreadWork {
    WorkQueue     *queue;
    FormatMetrics  metrics;
} ThreadWork;

static void *
threadWorker(void *arg) {
    ThreadWork *tw = arg;
    WorkQueue  *q  = tw->queue;

    Arena arena = arenaCreate(11 * Megabyte, 32 * Kilobyte, 64);
    Arena parseArena = arenaCreate(5 * Megabyte, 32 * Kilobyte, 64);
    FormatMetrics m = {0};

    for (;;) {
        u32 i = atomic_fetch_add_explicit(&q->next, 1, memory_order_relaxed);
        if (i >= q->pathCount) break;

        u64 start = arenaPos(&arena);
        u64 startParse = arenaPos(&parseArena);

        m.fileRead -= readCPUTimer();
        String content = readFile(&arena, q->paths[i]);
        m.fileRead += readCPUTimer();

        FormatResult r = format(&arena, &parseArena, content);

        arenaPopTo(&arena, start);
        arenaPopToZero(&parseArena, startParse);

        m.inputBytes += content.size;
        m.tokenize   += r.timings[Measurement_Tokenize];
        m.parse      += r.timings[Measurement_Parse];
        m.buildDoc   += r.timings[Measurement_BuildDoc];
        m.renderDoc  += r.timings[Measurement_RenderDoc];
    }

    tw->metrics = m;
    arenaDestroy(&arena);
    return 0;
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

    char *filepath = "tests/parserbuilding.sol";
    bool repetitionTester = false;

    if(argCount == 3) {
        assert(stringMatch((String){ .data = (u8 *)args[1], .size = strlen(args[1]) }, LIT_TO_STR("rep")));
        filepath = args[2];
        repetitionTester = true;
    }

    if(repetitionTester) {
        Arena arena = arenaCreate(16 * Megabyte, 32 * Kilobyte, 64);
        String content = readFile(&arena, filepath);
        repetitionTesterMain(&arena, content);
    } else {
        Arena arena = arenaCreate(16 * Megabyte, 32 * Kilobyte, 64);

        u64 elapsed = -readCPUTimer();
        char **paths = args + 1;
        u32 pathCount = argCount - 1;

        u32 processorCount = getProcessorCount();

        WorkQueue queue = { .paths = paths, .pathCount = pathCount, .next = 0 };
        ThreadWork *jobs = arrayPush(&arena, ThreadWork, processorCount);
        pthread_t *threads = arrayPush(&arena, pthread_t, processorCount);

        for(u32 i = 0; i < processorCount; i++) {
            jobs[i].queue = &queue;
            pthread_create(&threads[i], 0x0, threadWorker, &jobs[i]);
        }

        for(u32 i = 0; i < processorCount; i++) {
            pthread_join(threads[i], 0x0);
        }

        elapsed += readCPUTimer();

        u32 inputBytes = 0;
        for(u32 i = 0; i < processorCount; i++) {
            inputBytes += jobs[i].metrics.inputBytes;
        }

        double cpuFreq = readCPUFrequency();
        printf("Formatted %s%d%s files in %s%.0fms%s %s(%.0f MB/s)%s\n",
               ANSI_CYAN_LIGHT,
               argCount - 1,
               ANSI_RESET,
               ANSI_GREEN_LIGHT,
               (double)elapsed / cpuFreq * 1e3,
               ANSI_RESET,
               ANSI_MAGENTA_LIGHT,
               ((inputBytes / ((double)elapsed / cpuFreq)) / 1e6),
               ANSI_RESET
               );
        FormatMetrics sum = { 0 };
        for(u32 i = 0; i < processorCount; i++) {
            FormatMetrics metrics = jobs[i].metrics;

            sum.fileRead += metrics.fileRead;
            sum.tokenize += metrics.tokenize;
            sum.parse += metrics.parse;
            sum.buildDoc += metrics.buildDoc;
            sum.renderDoc += metrics.renderDoc;

            u32 elapsedSum = metrics.fileRead + metrics.tokenize + metrics.parse + metrics.buildDoc + metrics.renderDoc;

            // printf("Timings:\n");
            // printf("  File read:  %9llu cycles, %f ms\n", metrics.fileRead,  (double)metrics.fileRead / cpuFreq * 1e3);
            // printf("  Tokenize:   %9llu cycles, %f ms\n", metrics.tokenize,  (double)metrics.tokenize / cpuFreq * 1e3);
            // printf("  Parse:      %9llu cycles, %f ms\n", metrics.parse,     (double)metrics.parse / cpuFreq * 1e3);
            // printf("  BuildDoc:   %9llu cycles, %f ms\n", metrics.buildDoc,  (double)metrics.buildDoc / cpuFreq * 1e3);
            // printf("  RenderDoc:  %9llu cycles, %f ms\n", metrics.renderDoc, (double)metrics.renderDoc / cpuFreq * 1e3);
            // printf("  ---------\n");
            // printf("  Sum:        %9llu cycles, %f ms\n", elapsed, (double)elapsedSum / cpuFreq * 1e3);
        }

        printf("Thread avg: %s%.0fms read%s, %s%.0fms token%s, %s%.0fms parse%s, %s%.0fms build%s, %s%.0fms render%s\n",
               ANSI_YELLOW, (double)(sum.fileRead / processorCount ) / cpuFreq * 1e3, ANSI_RESET,
               ANSI_BLUE, (double)(sum.tokenize / processorCount ) / cpuFreq * 1e3, ANSI_RESET,
               ANSI_RED, (double)(sum.parse / processorCount ) / cpuFreq * 1e3, ANSI_RESET,
               ANSI_GREEN, (double)(sum.buildDoc / processorCount ) / cpuFreq * 1e3, ANSI_RESET,
               ANSI_CYAN, (double)(sum.renderDoc / processorCount ) / cpuFreq * 1e3, ANSI_RESET);
    }

}
