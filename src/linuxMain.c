#include "./src/format.c"
#include "./src/platform.c"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>

typedef struct StringQueueElement {
    char *path;
    _Atomic u64 sequenceNumber;
} StringQueueElement;

typedef struct StringQueue {
    // SPMC queue
    _Atomic u64 tail;
    u8 paddingTail[64 - sizeof(u64)];

    u64 head;
    StringQueueElement elements[512];
} StringQueue;

static void
stringQueueEnqueueBlocking(StringQueue *q, char *path) {
    for(;;) {
        u64 mask = ARRAY_LENGTH(q->elements) - 1;
        u64 position = q->head;
        u64 index = position & mask;
        StringQueueElement *element = q->elements + index;

        u64 sequence = atomic_load_explicit(&element->sequenceNumber, memory_order_acquire);
        if(sequence == position) {
            q->head = position + 1;
            element->path = path;
            atomic_store_explicit(&element->sequenceNumber, position + 1, memory_order_release);
            break;
        } else if (sequence < position) {
            sched_yield();
        } else {
            assert(false);
        }
    }
}

static char *
stringQueueDequeueBlocking(StringQueue *q) {
    for(;;) {
        u64 mask = ARRAY_LENGTH(q->elements) - 1;
        u64 position = atomic_load_explicit(&q->tail, memory_order_relaxed);
        u64 index = position & mask;
        StringQueueElement *element = q->elements + index;

        u64 sequence = atomic_load_explicit(&element->sequenceNumber, memory_order_acquire);
        if(sequence == position + 1) {
            if(atomic_compare_exchange_weak(&q->tail, &position, position + 1)) {
                atomic_store_explicit(&element->sequenceNumber, position + mask + 1, memory_order_release);

                return element->path;
            }
        } else {
            sched_yield();
        }
    }
}

typedef struct StringBucket {
    String values[256];
    u32 count;
    struct StringBucket *next;
} StringBucket;

typedef struct BucketedStringList {
    StringBucket *first;
    StringBucket *last;
    u32 count;
} BucketedStringList;

static StringBucket *
listPushString(BucketedStringList *list, String value, Arena *arena) {
    StringBucket *bucket = 0x0;
    if(list->count == 0) {
        bucket = structPush(arena, StringBucket);
        SLL_STACK_PUSH(list->first, bucket);
    } else {
        bucket = list->first;
    }

    if(bucket->count >= ARRAY_LENGTH(bucket->values)) {
        bucket = structPush(arena, StringBucket);
        SLL_STACK_PUSH(list->first, bucket);
    }

    bucket->values[bucket->count++] = value;
    list->count += 1;

    return bucket;
}

static String
listPopString(BucketedStringList *list) {
    StringBucket *bucket = list->first;
    if(bucket->count == 0 && bucket->next && bucket->next->count > 0) {
        list->first = bucket->next;

        if(list->last) {
            list->last->next = bucket;
        }

        list->last = bucket;
        list->last->next = 0x0;

        bucket = list->first;
    }

    assert(bucket->count > 0);
    String result = bucket->values[--bucket->count];
    list->count -= 1;

    return result;
}

typedef struct Walker {
    StringQueue *queue;

    BucketedStringList directoryStack;

    Arena *arena;
} Walker;

static void
walkerEnqueueFile(Walker *w, String parent, String path) {
    if(stringEndsWith(path, LIT_TO_STR(".sol"))) {
        String string = stringPushf(w->arena, "%s/%s", parent.data, path.data);
        stringQueueEnqueueBlocking(w->queue, (char *)string.data);
    }
}

static void
walkerPushDirectory(Walker *w, String parent, String path) {
    String string = stringPushf(w->arena, "%s/%s", parent.data, path.data);
    listPushString(&w->directoryStack, string, w->arena);
}

static void
fsWalker(Walker *w, char **paths, u32 pathCount) {
    for(u32 i = 0; i < pathCount; i++) {
        struct stat st = fileStat(paths[i]);
        bool isDir = S_ISDIR(st.st_mode);
        bool isFile = S_ISREG(st.st_mode);

        String topPath = { .data = (u8 *)paths[i], .size = strlen(paths[i]) };
        if(isFile && stringEndsWith(topPath, LIT_TO_STR(".sol"))) {
            stringQueueEnqueueBlocking(w->queue, paths[i]);
        } else if(isDir) {
            listPushString(&w->directoryStack, topPath, w->arena);
        }

        while(w->directoryStack.count > 0) {
            String directory = listPopString(&w->directoryStack);
            walkDirectory(w, w->arena, directory);
        }
    }
}

typedef struct WalkerMainParams {
    Arena *arena;
    StringQueue *queue;
    char **paths;
    u32 pathCount;
} WalkerMainParams;

static void *
walkerThreadMain(void *arg) {
    WalkerMainParams *params = (WalkerMainParams *)arg;
    Walker walker = {
        .arena = params->arena,
        .queue = params->queue,
    };

    fsWalker(&walker, params->paths, params->pathCount);

    return 0x0;
}

typedef struct FormatMetrics {
    u64 fileRead;
    u64 tokenize;
    u64 parse;
    u64 buildDoc;
    u64 renderDoc;

    u64 inputBytes;
    u64 inputFiles;
} FormatMetrics;

typedef struct ThreadWork {
    StringQueue   *queue;
    FormatMetrics  metrics;
} ThreadWork;

static void *
threadWorker(void *arg) {
    ThreadWork *tw = arg;
    StringQueue *q = tw->queue;

    Arena arena = arenaCreate(11 * Megabyte, 11 * Megabyte, 64);
    Arena parseArena = arenaCreate(5 * Megabyte, 5 * Megabyte, 64);
    FormatMetrics m = {0};

    for (;;) {
        char *path = stringQueueDequeueBlocking(q);
        if(path == 0x0) { break; }

        u64 start = arenaPos(&arena);
        u64 startParse = arenaPos(&parseArena);

        m.fileRead -= readTimer();
        String content = readFile(&arena, path);
        m.fileRead += readTimer();

        FormatResult r = format(&arena, &parseArena, content);

        if(content.size != r.source.size || memcmp(content.data, r.source.data, content.size) != 0) {
            FILE *f = fopen(path, "wb");
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
    arenaDestroy(&parseArena);
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

static void
printThreadMetrics(ThreadWork *jobs, u32 processorCount) {
    int processorCountDigits = (int)base10DigitCount(processorCount - 1);
    printf(
        "Thread : %sFiles%s : %sRead%s : %sToken%s : %sParse%s : %sBuild%s : %sRender%s : %sOverall%s\n",
        ANSI_GREEN_LIGHT, ANSI_RESET,
        ANSI_YELLOW,      ANSI_RESET,
        ANSI_BLUE,        ANSI_RESET,
        ANSI_RED,         ANSI_RESET,
        ANSI_GREEN,       ANSI_RESET,
        ANSI_CYAN,        ANSI_RESET,
        ANSI_MAGENTA,     ANSI_RESET
    );

    for(u32 i = 0; i < processorCount; i++) {
        FormatMetrics metrics = jobs[i].metrics;

        u32 elapsedSum = metrics.fileRead + metrics.tokenize + metrics.parse + metrics.buildDoc + metrics.renderDoc;

        double fileReadThroughput  = (metrics.inputBytes / ((double)metrics.fileRead  / NS_IN_SECOND)) / 1e6;
        double tokenizeThroughput  = (metrics.inputBytes / ((double)metrics.tokenize  / NS_IN_SECOND)) / 1e6;
        double parseThroughput     = (metrics.inputBytes / ((double)metrics.parse     / NS_IN_SECOND)) / 1e6;
        double buildDocThroughput  = (metrics.inputBytes / ((double)metrics.buildDoc  / NS_IN_SECOND)) / 1e6;
        double renderDocThroughput = (metrics.inputBytes / ((double)metrics.renderDoc / NS_IN_SECOND)) / 1e6;
        double overallThroughput   = (metrics.inputBytes / ((double)elapsedSum        / NS_IN_SECOND)) / 1e6;

        int leftPaddingForThread = 6 - processorCountDigits;
        printf(
            "%*s%0*d%s%*llu%s%s%*.0f%s%s%*.0f%s%s%*.0f%s%s%*.0f%s%s%*.0f%s%s%*.0f%s\n",
            leftPaddingForThread, "T", processorCountDigits, i,
            ANSI_GREEN_LIGHT, 3 + 5,  metrics.inputFiles, ANSI_RESET,
            ANSI_YELLOW,      3 + 4,  fileReadThroughput, ANSI_RESET,
            ANSI_BLUE,        3 + 5,  tokenizeThroughput, ANSI_RESET,
            ANSI_RED,         3 + 5,     parseThroughput, ANSI_RESET,
            ANSI_GREEN,       3 + 5,  buildDocThroughput, ANSI_RESET,
            ANSI_CYAN,        3 + 6, renderDocThroughput, ANSI_RESET,
            ANSI_MAGENTA,     3 + 7,   overallThroughput, ANSI_RESET
        );
    }
}

int main(int argCount, char **args) {
    bool repetitionTester = false;

    if(argCount == 3 && stringMatch((String){ .data = (u8 *)args[1], .size = strlen(args[1]) }, LIT_TO_STR("rep"))) {
        repetitionTester = true;
    }

    if(repetitionTester) {
        char *filepath = args[2];

        Arena arena = arenaCreate(16 * Megabyte, 32 * Kilobyte, 64);
        String content = readFile(&arena, filepath);
        repetitionTesterMain(&arena, content);
    } else {
        Arena arena = arenaCreate(16 * Megabyte, 32 * Kilobyte, 64);

        u64 elapsed = -readTimer();
        char **paths = args + 1;
        u32 pathCount = argCount - 1;

        if(pathCount == 0) {
            printf("Usage: %s <paths>\n", args[0]);
            return 1;
        }

        StringQueue queue = { };
        for(u32 i = 0; i < ARRAY_LENGTH(queue.elements); i++) {
            queue.elements[i].sequenceNumber = i;
        }

        WalkerMainParams params = {
            .arena = &arena,
            .queue = &queue,
            .paths = paths,
            .pathCount = pathCount
        };

        pthread_t walkerThread;
        pthread_create(&walkerThread, 0x0, walkerThreadMain, &params);

        u32 processorCount = getProcessorCount();
        u32 formatterThreadCount = processorCount - 1;
        ThreadWork *jobs = arrayPush(&arena, ThreadWork, formatterThreadCount);
        pthread_t *threads = arrayPush(&arena, pthread_t, formatterThreadCount);

        for(u32 i = 0; i < formatterThreadCount; i++) {
            jobs[i].queue = &queue;
            pthread_create(&threads[i], 0x0, threadWorker, &jobs[i]);
        }

        pthread_join(walkerThread, 0x0);
        // All the files have been enqueued. Push poison pills to stop the formatters.
        for(u32 i = 0; i < formatterThreadCount; i++) {
            stringQueueEnqueueBlocking(&queue, 0x0);
        }

        for(u32 i = 0; i < formatterThreadCount; i++) {
            pthread_join(threads[i], 0x0);
        }

        elapsed += readTimer();

        u32 inputBytes = 0;
        for(u32 i = 0; i < formatterThreadCount; i++) {
            inputBytes += jobs[i].metrics.inputBytes;
        }

        printThreadMetrics(jobs, formatterThreadCount);

        printf(
            "Formatted %s%llu%s files in %s%.0fms%s %s(%.0f MB/s)%s\n",
            ANSI_CYAN_LIGHT,    queue.tail - formatterThreadCount,                       ANSI_RESET,
            ANSI_GREEN_LIGHT,   (double)elapsed / 1e6,                                   ANSI_RESET,
            ANSI_MAGENTA_LIGHT, ((inputBytes / ((double)elapsed / NS_IN_SECOND)) / 1e6), ANSI_RESET
        );
    }

}
