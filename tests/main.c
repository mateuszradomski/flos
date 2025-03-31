#include "utest.h"
#include "format.c"

#include <sys/ioctl.h>
#include <unistd.h>

#define ANSI_RED   "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_RESET "\x1b[0m"
#define ANSI_GREY  "\x1b[90m"

typedef unsigned long long u64;

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

struct SolFmtFixture{
  char *path;
};

UTEST_F_SETUP(SolFmtFixture) { }

UTEST_F_TEARDOWN(SolFmtFixture) {
    Arena arena = arenaCreate(1024*1024, 4096, 32);
    TestData data = readTestInput(&arena, utest_fixture->path);

    String result = stringTrim(format(&arena, data.input));

    showDifferences(&arena, result, data.output);
    ASSERT_TRUE(stringMatch(result, data.output));

    String result2 = stringTrim(format(&arena, result));
    ASSERT_TRUE_MSG(stringMatch(result2, result), "Formatting is not stable");
    arenaDestroy(&arena);
}

UTEST_F(SolFmtFixture, structs) { utest_fixture->path = "tests/structs.sol"; }
UTEST_F(SolFmtFixture, enums) { utest_fixture->path = "tests/enums.sol"; }
UTEST_F(SolFmtFixture, addressPayable) { utest_fixture->path = "tests/addressPayable.sol"; }
UTEST_F(SolFmtFixture, basicIterator) { utest_fixture->path = "tests/basicIterator.sol"; }
UTEST_F(SolFmtFixture, breakingChangesV0_8_0) { utest_fixture->path = "tests/breakingChangesV0.8.0.sol"; }
UTEST_F(SolFmtFixture, customErrors) { utest_fixture->path = "tests/customErrors.sol"; }
UTEST_F(SolFmtFixture, enumsBigger) { utest_fixture->path = "tests/enumsBigger.sol"; }
UTEST_F(SolFmtFixture, hashLiteral) { utest_fixture->path = "tests/hashLiteral.sol"; }
UTEST_F(SolFmtFixture, immutable) { utest_fixture->path = "tests/immutable.sol"; }
UTEST_F(SolFmtFixture, indexRangeAccess) { utest_fixture->path = "tests/indexRangeAccess.sol"; }
UTEST_F(SolFmtFixture, modifierDefinitions) { utest_fixture->path = "tests/modifierDefinitions.sol"; }
UTEST_F(SolFmtFixture, modifierInvocations) { utest_fixture->path = "tests/modifierInvocations.sol"; }
UTEST_F(SolFmtFixture, numberLiteral) { utest_fixture->path = "tests/numberLiteral.sol"; }
UTEST_F(SolFmtFixture, pragma) { utest_fixture->path = "tests/pragma.sol"; }
UTEST_F(SolFmtFixture, simpleAuction) { utest_fixture->path = "tests/simpleAuction.sol"; }
UTEST_F(SolFmtFixture, simpleStorage) { utest_fixture->path = "tests/simpleStorage.sol"; }
UTEST_F(SolFmtFixture, blanklines) { utest_fixture->path = "tests/blanklines.sol"; }
UTEST_F(SolFmtFixture, typeDefinitions) { utest_fixture->path = "tests/typeDefinitions.sol"; }
UTEST_F(SolFmtFixture, operators) { utest_fixture->path = "tests/operators.sol"; }
UTEST_F(SolFmtFixture, comments) { utest_fixture->path = "tests/comments.sol"; }
UTEST_F(SolFmtFixture, imports) { utest_fixture->path = "tests/imports.sol"; }
UTEST_F(SolFmtFixture, blockComments) { utest_fixture->path = "tests/blockComments.sol"; }
UTEST_F(SolFmtFixture, constructorModifier) { utest_fixture->path = "tests/constructorModifier.sol"; }
UTEST_F(SolFmtFixture, using) { utest_fixture->path = "tests/using.sol"; }
UTEST_F(SolFmtFixture, typedefs) { utest_fixture->path = "tests/typedef.sol"; }
UTEST_F(SolFmtFixture, constVar) { utest_fixture->path = "tests/constVar.sol"; }
UTEST_F(SolFmtFixture, constVarBreaking) { utest_fixture->path = "tests/constVarBreaking.sol"; }
UTEST_F(SolFmtFixture, arrays) { utest_fixture->path = "tests/arrays.sol"; }
UTEST_F(SolFmtFixture, inbox) { utest_fixture->path = "tests/inbox.sol"; }
UTEST_F(SolFmtFixture, libraries) { utest_fixture->path = "tests/libraries.sol"; }
UTEST_F(SolFmtFixture, ownable) { utest_fixture->path = "tests/ownable.sol"; }
UTEST_F(SolFmtFixture, styleGuideMappings) { utest_fixture->path = "tests/styleGuideMappings.sol"; }
UTEST_F(SolFmtFixture, binaryOperators) { utest_fixture->path = "tests/binaryOperators.sol"; }
UTEST_F(SolFmtFixture, conditional) { utest_fixture->path = "tests/conditional.sol"; }
UTEST_F(SolFmtFixture, constructors) { utest_fixture->path = "tests/constructors.sol"; }
UTEST_F(SolFmtFixture, contractDefinition) { utest_fixture->path = "tests/contractDefinition.sol"; }
UTEST_F(SolFmtFixture, etc) { utest_fixture->path = "tests/etc.sol"; }
UTEST_F(SolFmtFixture, forStatement) { utest_fixture->path = "tests/forStatement.sol"; }
UTEST_F(SolFmtFixture, functionDefinitions) { utest_fixture->path = "tests/functionDefinitions.sol"; }
UTEST_F(SolFmtFixture, functionDefinitions_v0_5_0) { utest_fixture->path = "tests/functionDefinitions_v0_5_0.sol"; }
UTEST_F(SolFmtFixture, indexOf) { utest_fixture->path = "tests/indexOf.sol"; }
UTEST_F(SolFmtFixture, inheritanceSpecifier) { utest_fixture->path = "tests/inheritanceSpecifier.sol"; }
UTEST_F(SolFmtFixture, memberAccess) { utest_fixture->path = "tests/memberAccess.sol"; }
UTEST_F(SolFmtFixture, nameValueExpression) { utest_fixture->path = "tests/nameValueExpression.sol"; }
UTEST_F(SolFmtFixture, sampleCrowdSale) { utest_fixture->path = "tests/sampleCrowdSale.sol"; }
UTEST_F(SolFmtFixture, splittableCommodity) { utest_fixture->path = "tests/splittableCommodity.sol"; }
UTEST_F(SolFmtFixture, stateVariableDeclarations) { utest_fixture->path = "tests/stateVariableDeclarations.sol"; }
UTEST_F(SolFmtFixture, tuples) { utest_fixture->path = "tests/tuples.sol"; }
UTEST_F(SolFmtFixture, whileStatements) { utest_fixture->path = "tests/whileStatements.sol"; }
UTEST_F(SolFmtFixture, tryCatch) { utest_fixture->path = "tests/tryCatch.sol"; }
UTEST_F(SolFmtFixture, binaryOperationGroup) { utest_fixture->path = "tests/binaryOperationGroup.sol"; }
UTEST_F(SolFmtFixture, experimentalTerneries) { utest_fixture->path = "tests/experimentalTerneries.sol"; }
UTEST_F(SolFmtFixture, functionCalls) { utest_fixture->path = "tests/functionCalls.sol"; }
UTEST_F(SolFmtFixture, ifStatements) { utest_fixture->path = "tests/ifStatements.sol"; }
UTEST_F(SolFmtFixture, assembly) { utest_fixture->path = "tests/assembly.sol"; }
UTEST_F(SolFmtFixture, allFeatures) { utest_fixture->path = "tests/allFeatures.sol"; }
UTEST_F(SolFmtFixture, proxy) { utest_fixture->path = "tests/proxy.sol"; }
UTEST_F(SolFmtFixture, controlStructures) { utest_fixture->path = "tests/controlStructures.sol"; }
UTEST_F(SolFmtFixture, issue355) { utest_fixture->path = "tests/issue355.sol"; }
UTEST_F(SolFmtFixture, issue799) { utest_fixture->path = "tests/issue799.sol"; }
UTEST_F(SolFmtFixture, issue843) { utest_fixture->path = "tests/issue843.sol"; }

UTEST_MAIN();
