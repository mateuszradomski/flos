#include "utest.h"
#include "format.c"

typedef struct TestData {
  String input;
  String output;
} TestData;

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
showDifferences(String result, String expected) {
  SplitIterator resultIterator = stringSplit(result, '\n');
  SplitIterator expectedIterator = stringSplit(expected, '\n');

  while(true) {
    String resultLine = stringNextInSplit(&resultIterator);
    String expectedLine = stringNextInSplit(&expectedIterator);

    if(resultLine.data == 0 && expectedLine.data == 0) {
      break;
    }

    if(resultLine.data == 0 && expectedLine.data != 0) {
      printf("Result is shorter than expected\n");
      break;
    }

    if(resultLine.data != 0 && expectedLine.data == 0) {
      printf("Result is longer than expected\n");
      break;
    }

    if(!stringMatch(resultLine, expectedLine)) {
      printf("Result does not match expected on the following line:\n");
      printf("%.*s\n", (int)resultLine.size, resultLine.data);
      printf("%.*s\n", (int)expectedLine.size, expectedLine.data);
      break;
    }
  }
}

static void
testGivenFile(char *path) {
}

struct SolFmtFixture{
  char *path;
};

UTEST_F_SETUP(SolFmtFixture) { }

UTEST_F_TEARDOWN(SolFmtFixture) {
    Arena arena = arenaCreate(1024*1024, 4096, 32);
    TestData data = readTestInput(&arena, utest_fixture->path);

    String result = stringTrim(format(&arena, data.input));

    showDifferences(result, data.output);
    ASSERT_TRUE(stringMatch(result, data.output));
    arenaDestroy(&arena);
}

UTEST_F(SolFmtFixture, structs) {
  utest_fixture->path = "tests/structs.sol";
}

UTEST_F(SolFmtFixture, enums) {
  utest_fixture->path = "tests/enums.sol";
}

UTEST_F(SolFmtFixture, addressPayable) {
  utest_fixture->path = "tests/addressPayable.sol";
}

UTEST_MAIN();
