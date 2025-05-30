#include "munit.h"
#include "format.c"

#include <sys/ioctl.h>
#include <unistd.h>
#include <glob.h>

#define ANSI_RED   "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RESET "\x1b[0m"
#define ANSI_GREY  "\x1b[90m"

static u64
consoleWidth() {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return w.ws_col;
}

typedef struct TestData {
  String input;
  String output;
} TestData;

typedef struct DiffIterator {
    String *leftLines;
    u32 leftLineCount;
    String *rightLines;
    u32 rightLineCount;
    String *commonLines;
    u32 commonLineCount;

    u32 i, j, commonLineIndex;
} DiffIterator;

typedef struct DiffResult {
    String commonPrevious;
    String commonNext;
    String left;
    String right;
} DiffResult;

static bool
validDiff(DiffResult r) {
    return r.left.data != 0x0 || r.right.data != 0x0;
}

static u64
flatIndex(u64 x, u64 y, u64 width) {
    return y * width + x;
}

static DiffIterator
diffStrings(Arena *arena, String before, String after) {
    u32 leftLineCount = 0;
    for(u64 i = 0; i < before.size; i++) {
        leftLineCount += before.data[i] == '\n';
    }

    u32 rightLineCount = 0;
    for(u64 i = 0; i < after.size; i++) {
        rightLineCount += after.data[i] == '\n';
    }

    u16 *distances = arrayPush(arena, u16, (rightLineCount + 1) * (leftLineCount + 1));
    String *leftLines = arrayPush(arena, String, leftLineCount);
    String *rightLines = arrayPush(arena, String, rightLineCount);

    u32 dWidth = leftLineCount + 1;
    distances[flatIndex(0, 0, dWidth)] = 0;
    distances[flatIndex(0, 1, dWidth)] = 0;
    distances[flatIndex(1, 0, dWidth)] = 0;

    SplitIterator leftLineIt = stringSplit(before, '\n');
    for(u32 i = 0; i < leftLineCount; i++) {
        String leftLine = stringNextInSplit(&leftLineIt);
        assert(leftLine.data != 0x0);
        leftLines[i] = leftLine;

        SplitIterator rightLineIt = stringSplit(after, '\n');
        for(u32 j = 0; j < rightLineCount; j++) {
            String rightLine = stringNextInSplit(&rightLineIt);
            assert(rightLine.data != 0x0);
            rightLines[j] = rightLine;

            u32 value = 0;
            if(stringMatch(rightLine, leftLine)) {
                value = 1 + distances[flatIndex(i, j, dWidth)];
            } else {
                value = MAX(distances[flatIndex(i + 1, j, dWidth)], distances[flatIndex(i, j + 1, dWidth)]);
            }
            distances[flatIndex(i + 1, j + 1, dWidth)] = value;
        }
    }

    s32 index = distances[flatIndex(leftLineCount, rightLineCount, dWidth)];
    u32 commonLineCount = index + 1;
    String *commonLines = arrayPush(arena, String, commonLineCount);
    for(u32 i = 0; i < index; i++) {
        commonLines[i] = LIT_TO_STR("");
    }

    s32 i = leftLineCount;
    s32 j = rightLineCount;
    while(i > 0 && j > 0) {
        if(stringMatch(leftLines[i - 1], rightLines[j - 1])) {
            commonLines[--index] = leftLines[--i];
            j -= 1;
        } else if(distances[flatIndex(i - 1, j, dWidth)] > distances[flatIndex(i, j - 1, dWidth)]) {
            i -= 1;
        } else {
            j -= 1;
        }
    }

    return (DiffIterator){
        .leftLines = leftLines,
        .leftLineCount = leftLineCount,
        .rightLines = rightLines,
        .rightLineCount = rightLineCount,
        .commonLines = commonLines,
        .commonLineCount = commonLineCount,
    };
}

static DiffResult
diffNext(DiffIterator *it) {
    DiffResult result = {};

    for(; it->commonLineIndex < it->commonLineCount && !validDiff(result); it->commonLineIndex++) {
        String commonLine = it->commonLines[it->commonLineIndex];
        result.commonNext = commonLine;

        while(it->i < it->leftLineCount && !stringMatch(it->leftLines[it->i], commonLine)) {
            String removedLine = it->leftLines[it->i++];

            result.left.data = result.left.data == 0x0 ? removedLine.data : result.left.data;
            result.left.size += removedLine.size + 1;
        }

        while(it->j < it->rightLineCount && !stringMatch(it->rightLines[it->j], commonLine)) {
            String addedLine = it->rightLines[it->j++];

            result.right.data = result.right.data == 0x0 ? addedLine.data : result.right.data;
            result.right.size += addedLine.size + 1;
        }

        if(it->i < it->leftLineCount && it->j < it->rightLineCount) {
            it->i++;
            it->j++;
        }
    }

    return result;
}

static TestData
readTestInput(Arena *arena, char *path) {
  TestData result = { 0 };

  FILE *fd = fopen(path, "rb");
  fseek(fd, 0L, SEEK_END);
  u32 contentLength = ftell(fd);

  fseek(fd, 0L, SEEK_SET);
  u8 *memory = arrayPush(arena, u8, contentLength + 1);

  int readBytes = fread(memory, contentLength, 1, fd);
  fclose(fd);
  memory[contentLength] = 0;

  String content = { .data = memory, .size = contentLength };

  String delimiter = LIT_TO_STR("// Above input, below output");
  u32 i = 0;
  for(i = 0; i < content.size - delimiter.size; i++) {
    String substring = { .data = content.data + i, .size = delimiter.size };
    if(stringMatch(substring, delimiter)) {
      break;
    }
  }

  assert(i > 0);
  result.input = (String){ .data = memory, .size = i };
  result.output = (String){ .data = memory + i + delimiter.size, .size = contentLength - i - delimiter.size };

  result.input = stringTrim(stringUnixLines(result.input));
  result.output = stringTrim(stringUnixLines(result.output));

  result.output.data[result.output.size] = 0;

  return result;
}

static void
showDifferences(Arena *arena, String result, String expected) {
    if(stringMatch(result, expected)) {
        return;
    } else {
        FILE *resultFile = fopen("result.sol", "wb");
        fwrite(result.data, 1, result.size, resultFile);
        fclose(resultFile);

        FILE *expectedFile = fopen("expected.sol", "wb");
        fwrite(expected.data, 1, expected.size, expectedFile);
        fclose(expectedFile);
    }

    DiffIterator diffIt = diffStrings(arena, expected, result);

    u32 width = consoleWidth();
    u32 oddWidth = width % 2 == 1;
    u32 dividerChars = 4 - oddWidth;
    u32 charsInPanel = (width - dividerChars) / 2;

    DiffResult diffResult = { 0 };
    bool leading = false;
    printf("\n");
    while(validDiff(diffResult = diffNext(&diffIt))) {
        if(!leading) {
            leading = true;
            for(u32 i = 0; i < width; i++) { printf("-"); }
        }

        if(diffResult.commonPrevious.size > 0) {
            printf(ANSI_GREY "%.*s\n" ANSI_RESET, (int)diffResult.commonPrevious.size, diffResult.commonPrevious.data);
        }

        SplitIterator leftIt = stringSplit(diffResult.left, '\n');
        SplitIterator rightIt = stringSplit(diffResult.right, '\n');
        String left;
        String right;

        while((left = stringNextInSplit(&leftIt)).data != 0x0 | (right = stringNextInSplit(&rightIt)).data != 0x0) {
            printf("[");
            u32 stringSize = MIN(charsInPanel, left.size);
            printf(ANSI_RED "%.*s" ANSI_RESET, stringSize, left.data);
            for(u32 i = 0; i < charsInPanel - stringSize; i++) { printf(" "); }

            printf((oddWidth ? "|" : "]["));

            stringSize = MIN(charsInPanel, right.size);
            printf(ANSI_GREEN "%.*s" ANSI_RESET, stringSize, right.data);
            for(u32 i = 0; i < charsInPanel - stringSize; i++) { printf(" "); }
            printf("]\n");
        }

        if(diffResult.commonNext.size > 0) {
            printf(ANSI_GREY "%.*s\n" ANSI_RESET, (int)diffResult.commonNext.size, diffResult.commonNext.data);
        }

        for(u32 i = 0; i < width; i++) { printf("-"); }
    }
}

static MunitResult
testFormat(const MunitParameter params[], void* userData) {
    char *path = params[0].value;

    Arena arena = arenaCreate(1024*1024, 4096, 32);
    TestData data = readTestInput(&arena, path);

    String result = stringTrim(format(&arena, &arena, data.input).source);

    showDifferences(&arena, result, data.output);
    munit_assert_true(stringMatch(result, data.output));

    String result2 = stringTrim(format(&arena, &arena, result).source);
    showDifferences(&arena, result, result2);
    munit_assert_true(stringMatch(result2, result));
    arenaDestroy(&arena);

    return MUNIT_OK;
}

int main (int argc, const char* argv[]) {
    glob_t prettierFiles = { 0 };
    glob_t foundryFiles = { 0 };
    glob_t flosFiles = { 0 };
    assert(glob("./tests/prettier/*.sol", 0, 0x0, &prettierFiles) == 0);
    assert(glob("./tests/foundry/*.sol", 0, 0x0, &foundryFiles) == 0);
    assert(glob("./tests/flos/*.sol", 0, 0x0, &flosFiles) == 0);

    char **paths = calloc(sizeof(char *), prettierFiles.gl_pathc + foundryFiles.gl_pathc + flosFiles.gl_pathc + 1);
    for(int i = 0; i < prettierFiles.gl_pathc; i++) {
        paths[i] = prettierFiles.gl_pathv[i];
    }
    for(int i = 0; i < foundryFiles.gl_pathc; i++) {
        paths[i + prettierFiles.gl_pathc] = foundryFiles.gl_pathv[i];
    }
    for(int i = 0; i < flosFiles.gl_pathc; i++) {
        paths[i + prettierFiles.gl_pathc + foundryFiles.gl_pathc] = flosFiles.gl_pathv[i];
    }
    paths[prettierFiles.gl_pathc + foundryFiles.gl_pathc + flosFiles.gl_pathc] = 0;

    MunitParameterEnum test_params[] = {
        { "path", paths },
        { NULL, NULL },
    };

    MunitTest tests[] = {
        { (char*) "/format", testFormat, NULL, NULL, MUNIT_TEST_OPTION_NONE, test_params },
        { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
    };

    MunitSuite suite = { "/flos", tests, NULL, 1, MUNIT_SUITE_OPTION_NONE };

  return munit_suite_main(&suite, NULL, argc, (char **)argv);
}
