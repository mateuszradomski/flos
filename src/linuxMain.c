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
    u64 inputFiles;
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

    Arena arena = arenaCreate(11 * Megabyte, 11 * Megabyte, 64);
    Arena parseArena = arenaCreate(5 * Megabyte, 5 * Megabyte, 64);
    FormatMetrics m = {0};

    for (;;) {
        u32 i = atomic_fetch_add_explicit(&q->next, 1, memory_order_relaxed);
        if (i >= q->pathCount) break;

        u64 start = arenaPos(&arena);
        u64 startParse = arenaPos(&parseArena);

        m.fileRead -= readTimer();
        String content = readFile(&arena, q->paths[i]);
        m.fileRead += readTimer();

        FormatResult r = format(&arena, &parseArena, content);

        if(content.size != r.source.size || memcmp(content.data, r.source.data, content.size) != 0) {
            FILE *f = fopen(q->paths[i], "wb");
            fwrite(r.source.data, 1, r.source.size, f);
            fclose(f);
        }

        arenaPopTo(&arena, start);
        arenaPopToZero(&parseArena, startParse);

        m.inputBytes += content.size;
        m.tokenize   += r.timings[Measurement_Tokenize];
        m.parse      += r.timings[Measurement_Parse];
        m.buildDoc   += r.timings[Measurement_BuildDoc];
        m.renderDoc  += r.timings[Measurement_RenderDoc];
        m.inputFiles += 1;
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

    u64 startedAt = readTimer();
    u64 maxTiming = 0;
    u64 minTiming = (u64)(-1);
    u64 timingSum = 0;
    u64 timingCount = 0;
    u64 testDuration = 10 * NS_IN_SECOND;

    while(startedAt + testDuration > readTimer()) {
        u64 timing = -readTimer();
        buildDocument(&render, &node, content, tokens);
        timing += readTimer();
        render = cleanRender;

        timingSum += timing;
        timingCount += 1;
        maxTiming = maxTiming > timing ? maxTiming : timing;
        if(timing < minTiming) {
            minTiming = timing;
            startedAt = readTimer();
        }
    }

    TestEntry docBuild = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    buildDocument(&render, &node, content, tokens);

    startedAt = readTimer();
    maxTiming = 0;
    minTiming = (u64)(-1);
    timingSum = 0;
    timingCount = 0;

    while(startedAt + testDuration > readTimer()) {
        u64 timing = -readTimer();
        renderDocument(&render);
        timing += readTimer();
        render.writer = cleanRender.writer;

        timingSum += timing;
        timingCount += 1;
        maxTiming = maxTiming > timing ? maxTiming : timing;
        if(timing < minTiming) {
            minTiming = timing;
            startedAt = readTimer();
        }
    }

    TestEntry docRender = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    double cpuFreq = NS_IN_SECOND;
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

static u64
log10I(u64 v) {
    u64 result = 0;
    if (v >= 10000000000000000000ULL) return 19;
    if (v >= 1000000000000000000ULL) return 18;
    if (v >= 100000000000000000ULL) return 17;
    if (v >= 10000000000000000ULL) return 16;
    if (v >= 1000000000000000ULL) return 15;
    if (v >= 100000000000000ULL) return 14;
    if (v >= 10000000000000ULL) return 13;
    if (v >= 1000000000000ULL) return 12;
    if (v >= 100000000000ULL) return 11;
    if (v >= 10000000000ULL) return 10;
    if (v >= 1000000000ULL) return 9;
    if (v >= 100000000ULL) return 8;
    if (v >= 10000000ULL) return 7;
    if (v >= 1000000ULL) return 6;
    if (v >= 100000ULL) return 5;
    if (v >= 10000ULL) return 4;
    if (v >= 1000ULL) return 3;
    if (v >= 100ULL) return 2;
    if (v >= 10ULL) return 1;
    return 0;
}

static u64
base10DigitCount(u64 v) {
    return log10I(v) + 1;
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

        u64 elapsed = -readTimer();
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

        elapsed += readTimer();

        u32 inputBytes = 0;
        for(u32 i = 0; i < processorCount; i++) {
            inputBytes += jobs[i].metrics.inputBytes;
        }

        printf("Formatted %s%d%s files in %s%.0fms%s %s(%.0f MB/s)%s\n",
               ANSI_CYAN_LIGHT,
               argCount - 1,
               ANSI_RESET,
               ANSI_GREEN_LIGHT,
               (double)elapsed / 1e6,
               ANSI_RESET,
               ANSI_MAGENTA_LIGHT,
               ((inputBytes / ((double)elapsed / NS_IN_SECOND)) / 1e6),
               ANSI_RESET
               );
        FormatMetrics sum = { 0 };

        int processorCountDigits = (int)base10DigitCount(processorCount);
        int fileCountDigits = 0;
        int fileReadDigits = 0, tokenizeDigits = 0, parseDigits = 0, buildDocDigits = 0, renderDocDigits = 0;
        for(u32 i = 0; i < processorCount; i++) {
            FormatMetrics metrics = jobs[i].metrics;

            fileCountDigits = MAX(fileCountDigits, (int)base10DigitCount(metrics.inputFiles));

            double fileReadThroughput  = (metrics.inputBytes / ((double)metrics.fileRead  / NS_IN_SECOND)) / 1e6;
            double tokenizeThroughput  = (metrics.inputBytes / ((double)metrics.tokenize  / NS_IN_SECOND)) / 1e6;
            double parseThroughput     = (metrics.inputBytes / ((double)metrics.parse     / NS_IN_SECOND)) / 1e6;
            double buildDocThroughput  = (metrics.inputBytes / ((double)metrics.buildDoc  / NS_IN_SECOND)) / 1e6;
            double renderDocThroughput = (metrics.inputBytes / ((double)metrics.renderDoc / NS_IN_SECOND)) / 1e6;

            fileReadDigits  = MAX(fileReadDigits,  (int)base10DigitCount((u64)fileReadThroughput));
            tokenizeDigits  = MAX(tokenizeDigits,  (int)base10DigitCount((u64)tokenizeThroughput));
            parseDigits     = MAX(parseDigits,     (int)base10DigitCount((u64)parseThroughput));
            buildDocDigits  = MAX(buildDocDigits,  (int)base10DigitCount((u64)buildDocThroughput));
            renderDocDigits = MAX(renderDocDigits, (int)base10DigitCount((u64)renderDocThroughput));
        }

        for(u32 i = 0; i < processorCount; i++) {
            FormatMetrics metrics = jobs[i].metrics;

            sum.fileRead += metrics.fileRead;
            sum.tokenize += metrics.tokenize;
            sum.parse += metrics.parse;
            sum.buildDoc += metrics.buildDoc;
            sum.renderDoc += metrics.renderDoc;

            u32 elapsedSum = metrics.fileRead + metrics.tokenize + metrics.parse + metrics.buildDoc + metrics.renderDoc;

            double fileReadThroughput  = (metrics.inputBytes / ((double)metrics.fileRead  / NS_IN_SECOND)) / 1e6;
            double tokenizeThroughput  = (metrics.inputBytes / ((double)metrics.tokenize  / NS_IN_SECOND)) / 1e6;
            double parseThroughput     = (metrics.inputBytes / ((double)metrics.parse     / NS_IN_SECOND)) / 1e6;
            double buildDocThroughput  = (metrics.inputBytes / ((double)metrics.buildDoc  / NS_IN_SECOND)) / 1e6;
            double renderDocThroughput = (metrics.inputBytes / ((double)metrics.renderDoc / NS_IN_SECOND)) / 1e6;

            printf(" T%0*d - %*llu files @ %s%*.0fMB/s read%s, %s%*.0fMB/s token%s, %s%*.0fMB/s parse%s, %s%*.0fMB/s build%s, %s%*.0fMB/s render%s\n",
                   processorCountDigits, i, fileCountDigits, metrics.inputFiles,
                   ANSI_YELLOW, fileReadDigits,  fileReadThroughput, ANSI_RESET,
                   ANSI_BLUE,   tokenizeDigits,  tokenizeThroughput, ANSI_RESET,
                   ANSI_RED,    parseDigits,     parseThroughput, ANSI_RESET,
                   ANSI_GREEN,  buildDocDigits,  buildDocThroughput, ANSI_RESET,
                   ANSI_CYAN,   renderDocDigits, renderDocThroughput, ANSI_RESET);
        }

        printf("Thread avg: %s%.0fms read%s, %s%.0fms token%s, %s%.0fms parse%s, %s%.0fms build%s, %s%.0fms render%s\n",
               ANSI_YELLOW, (double)(sum.fileRead / processorCount)  / 1e6, ANSI_RESET,
               ANSI_BLUE,   (double)(sum.tokenize / processorCount)  / 1e6, ANSI_RESET,
               ANSI_RED,    (double)(sum.parse / processorCount)     / 1e6, ANSI_RESET,
               ANSI_GREEN,  (double)(sum.buildDoc / processorCount)  / 1e6, ANSI_RESET,
               ANSI_CYAN,   (double)(sum.renderDoc / processorCount) / 1e6, ANSI_RESET);
    }

}
