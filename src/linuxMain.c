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
printBenchHeader(void) {
    printf("  %-10s  %7s %7s %7s      %6s %6s %6s     %8s\n",
           "", "--- ms", "(min/avg", "max) ---", "- cyc/b", "(min/avg", "max) -", "");
    printf("  %-10s  %7s %7s %7s      %6s %6s %6s     %8s\n",
           "Stage", "min", "avg", "max", "min", "avg", "max", "runs");
}

static void
printBenchEntry(const char *name, TestEntry t, double cpuFreq, u64 contentSize) {
    double toMs  = 1000.0 / cpuFreq;
    double toCpb = 1.0 / (double)contentSize;

    printf("  %-10s  %7.3f %7.3f %7.3f      %6.2f %6.2f %6.2f     %8llu\n",
           name,
           t.minCycles * toMs,  t.avgCycles * toMs,  t.maxCycles * toMs,
           t.minCycles * toCpb, t.avgCycles * toCpb, t.maxCycles * toCpb,
           t.runCount);
}

static void
printCountdown(u64 startedAt, u64 testDuration, u64 *lastWholeSecond, const char *label) {
    u64 now = readTimer();
    u64 endAt = startedAt + testDuration;
    u64 remaining = (endAt > now) ? (endAt - now) : 0;
    u64 wholeSecond = remaining / NS_IN_SECOND;
    if(wholeSecond != *lastWholeSecond) {
        printf("\r  %s: %llu seconds left... ", label, (unsigned long long)wholeSecond);
        fflush(stdout);
        *lastWholeSecond = wholeSecond;
    }
}

static void
clearCountdownLine(void) {
    printf("\r\033[K");
    fflush(stdout);
}

static void
repetitionTesterMain(Arena *arena, String content) {
    u64 startedAt = readTimer();
    u64 maxTiming = 0;
    u64 minTiming = (u64)(-1);
    u64 timingSum = 0;
    u64 timingCount = 0;
    u64 testDuration = 10 * NS_IN_SECOND;
    u64 lastWhole = (u64)-1;

    TokenizeResult tokens;

    while(startedAt + testDuration > readTimer()) {
        u64 start = arenaPos(arena);

        u64 timing = -readTimer();
        tokens = tokenize(content, arena);
        timing += readTimer();

        arenaPopTo(arena, start);

        timingSum += timing;
        timingCount += 1;
        maxTiming = maxTiming > timing ? maxTiming : timing;
        if(timing < minTiming) {
            minTiming = timing;
            startedAt = readTimer();
        }

        printCountdown(startedAt, testDuration, &lastWhole, "Tokenize");
    }
    clearCountdownLine();

    TestEntry lex = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    tokens = tokenize(content, arena);
    Parser parser;
    ASTNode node;

    startedAt = readTimer();
    maxTiming = 0;
    minTiming = (u64)(-1);
    timingSum = 0;
    timingCount = 0;
    lastWhole = (u64)-1;

    while(startedAt + testDuration > readTimer()) {
        u64 start = arenaPos(arena);

        u64 timing = -readTimer();
        parser = createParser(tokens, arena);
        node = parseSourceUnit(&parser);
        timing += readTimer();

        arenaPopToZero(arena, start);

        timingSum += timing;
        timingCount += 1;
        maxTiming = maxTiming > timing ? maxTiming : timing;
        if(timing < minTiming) {
            minTiming = timing;
            startedAt = readTimer();
        }
        printCountdown(startedAt, testDuration, &lastWhole, "Parse");
    }
    clearCountdownLine();

    TestEntry parse = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    tokens = tokenize(content, arena);
    parser = createParser(tokens, arena);
    node = parseSourceUnit(&parser);

    Render render = createRender(arena, content, tokens);
    Render cleanRender = render;

    startedAt = readTimer();
    maxTiming = 0;
    minTiming = (u64)(-1);
    timingSum = 0;
    timingCount = 0;
    lastWhole = (u64)-1;

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
        printCountdown(startedAt, testDuration, &lastWhole, "Document build");
    }
    clearCountdownLine();

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
    lastWhole = (u64)-1;

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

        printCountdown(startedAt, testDuration, &lastWhole, "Document render");
    }
    clearCountdownLine();

    TestEntry docRender = {
        .minCycles = minTiming,
        .maxCycles = maxTiming,
        .avgCycles = timingSum / timingCount,
        .runCount = timingCount,
    };

    double cpuFreq = NS_IN_SECOND;
    printBenchHeader();
    printBenchEntry("Tokenize",  lex,       cpuFreq, content.size);
    printBenchEntry("Parse",     parse,     cpuFreq, content.size);
    printBenchEntry("BuildDoc",  docBuild,  cpuFreq, content.size);
    printBenchEntry("RenderDoc", docRender, cpuFreq, content.size);
}

static void
printThreadMetrics(ThreadWork *jobs, u32 processorCount) {
    u32 activeThreads = 0;
    u64 totalFiles = 0, totalBytes = 0;
    u64 totalRead = 0, totalToken = 0, totalParse = 0, totalBuild = 0, totalRender = 0;
    
    for (u32 i = 0; i < processorCount; i++) {
        FormatMetrics m = jobs[i].metrics;
        if (m.inputFiles == 0) continue;
        activeThreads++;
        totalFiles  += m.inputFiles;
        totalBytes  += m.inputBytes;
        totalRead   += m.fileRead;
        totalToken  += m.tokenize;
        totalParse  += m.parse;
        totalBuild  += m.buildDoc;
        totalRender += m.renderDoc;
    }
    
    if (activeThreads == 0) {
        printf("  %s(No threads processed any files)%s\n", ANSI_DIM, ANSI_RESET);
        return;
    }

    printf("\n");
    printf("  %s┏━━━━━━━━━━┳━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━┳━━━━━━━━━━━━┓%s\n", 
           ANSI_DIM, ANSI_RESET);
    printf("  %s┃%s  %sThread%s  %s┃%s  %sFiles%s  %s┃%s   %sRead%s   %s┃%s  %sToken%s   %s┃%s  %sParse%s   %s┃%s  %sBuild%s   %s┃%s  %sRender%s  %s┃%s  %sOverall%s   %s┃%s\n",
           ANSI_DIM, ANSI_RESET, ANSI_WHITE_BRIGHT,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_GREEN_LIGHT, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_YELLOW,      ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_BLUE,        ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_RED,         ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_GREEN,       ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_CYAN,        ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_MAGENTA,     ANSI_RESET, ANSI_DIM, ANSI_RESET);
    printf("  %s┃          ┃         ┃%s   %sMB/s%s   %s┃%s   %sMB/s%s   %s┃%s   %sMB/s%s   %s┃%s   %sMB/s%s   %s┃%s   %sMB/s%s   %s┃%s    %sMB/s%s    %s┃%s\n",
           ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET, ANSI_DIM, ANSI_RESET);
    printf("  %s┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫%s\n",
           ANSI_DIM, ANSI_RESET);

    for (u32 i = 0; i < processorCount; i++) {
        FormatMetrics m = jobs[i].metrics;
        if (m.inputFiles == 0) continue;

        u64 elapsedSum = m.fileRead + m.tokenize + m.parse + m.buildDoc + m.renderDoc;

        double readTP   = (m.inputBytes / ((double)m.fileRead  / NS_IN_SECOND)) / 1e6;
        double tokenTP  = (m.inputBytes / ((double)m.tokenize  / NS_IN_SECOND)) / 1e6;
        double parseTP  = (m.inputBytes / ((double)m.parse     / NS_IN_SECOND)) / 1e6;
        double buildTP  = (m.inputBytes / ((double)m.buildDoc  / NS_IN_SECOND)) / 1e6;
        double renderTP = (m.inputBytes / ((double)m.renderDoc / NS_IN_SECOND)) / 1e6;
        double totalTP  = (m.inputBytes / ((double)elapsedSum  / NS_IN_SECOND)) / 1e6;

        printf("  %s┃%s    %sT%-3u%s  %s┃%s %s%6llu%s  %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s  %s%8.1f%s  %s┃%s\n",
               ANSI_DIM, ANSI_RESET, ANSI_WHITE_BRIGHT, i, ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_GREEN_LIGHT, (unsigned long long)m.inputFiles, ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_YELLOW,      readTP,   ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_BLUE,        tokenTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_RED,         parseTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_GREEN,       buildTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_CYAN,        renderTP, ANSI_RESET, ANSI_DIM, ANSI_RESET,
               ANSI_MAGENTA,     totalTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET);
    }

    printf("  %s┣━━━━━━━━━━╋━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━╋━━━━━━━━━━━━┫%s\n", ANSI_DIM, ANSI_RESET);

    u64 elapsedTotal = totalRead + totalToken + totalParse + totalBuild + totalRender;
    double sumReadTP   = (totalBytes / ((double)totalRead   / NS_IN_SECOND)) / 1e6;
    double sumTokenTP  = (totalBytes / ((double)totalToken  / NS_IN_SECOND)) / 1e6;
    double sumParseTP  = (totalBytes / ((double)totalParse  / NS_IN_SECOND)) / 1e6;
    double sumBuildTP  = (totalBytes / ((double)totalBuild  / NS_IN_SECOND)) / 1e6;
    double sumRenderTP = (totalBytes / ((double)totalRender / NS_IN_SECOND)) / 1e6;
    double sumTotalTP  = (totalBytes / ((double)elapsedTotal / NS_IN_SECOND)) / 1e6;

    double sumReadPart   = ((double)totalRead / elapsedTotal) * 100;
    double sumTokenPart  = ((double)totalToken / elapsedTotal) * 100;
    double sumParsePart  = ((double)totalParse / elapsedTotal) * 100;
    double sumBuildPart  = ((double)totalBuild / elapsedTotal) * 100;
    double sumRenderPart = ((double)totalRender / elapsedTotal) * 100;

    printf("  %s┃%s  %s∑ Total%s %s┃%s %s%6llu%s  %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s %s%8.1f%s %s┃%s  %s%8.1f%s  %s┃%s\n",
           ANSI_DIM, ANSI_RESET, ANSI_WHITE_BRIGHT ANSI_BOLD, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_GREEN_LIGHT ANSI_BOLD, (unsigned long long)totalFiles, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_YELLOW ANSI_BOLD,      sumReadTP,   ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_BLUE ANSI_BOLD,        sumTokenTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_RED ANSI_BOLD,         sumParseTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_GREEN ANSI_BOLD,       sumBuildTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_CYAN ANSI_BOLD,        sumRenderTP, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_MAGENTA ANSI_BOLD,     sumTotalTP,  ANSI_RESET, ANSI_DIM, ANSI_RESET);
    printf("  %s┃%s  %s   Part%s %s┃%s         %s┃%s %s%7.1f%%%s %s┃%s %s%7.1f%%%s %s┃%s %s%7.1f%%%s %s┃%s %s%7.1f%%%s %s┃%s %s%7.1f%%%s %s┃%s            %s┃%s\n",
           ANSI_DIM, ANSI_RESET, ANSI_WHITE_BRIGHT ANSI_BOLD, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET,
           ANSI_YELLOW ANSI_BOLD,      sumReadPart, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_BLUE ANSI_BOLD,        sumTokenPart,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_RED ANSI_BOLD,         sumParsePart,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_GREEN ANSI_BOLD,       sumBuildPart,  ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_CYAN ANSI_BOLD,        sumRenderPart, ANSI_RESET, ANSI_DIM, ANSI_RESET,
           ANSI_DIM, ANSI_RESET);

    printf("  %s┗━━━━━━━━━━┻━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━┻━━━━━━━━━━━━┛%s\n", ANSI_DIM, ANSI_RESET);
    printf("\n");
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
