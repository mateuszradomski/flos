#include "./src/utils.c"
#include "./src/tokenize.c"
#include "./src/yulLexer.c"
#include "./src/parser.c"
#include "./src/renderTree.c"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <math.h>

typedef unsigned long long u64;

String format(Arena *arena, String input) {
    TokenizeResult tokens = tokenize(input, arena);
    Parser parser = createParser(tokens, arena);
    ASTNode node = parseSourceUnit(&parser);
    String result = renderTree(arena, node, input, tokens);

    return result;
}

int main(int argCount, char **args) {
    u64 elapsed = -__rdtsc();
    Arena arena = arenaCreate(128 * Megabyte, 32 * Kilobyte, 64);

    char *filepath = "tests/parserbuilding.sol";
    if(argCount == 2) {
        filepath = args[1];
    }

    FILE *fd = fopen(filepath, "rb");
    fseek(fd, 0L, SEEK_END);
    u32 contentLength = ftell(fd);

    fseek(fd, 0L, SEEK_SET);
    u8 *content = arrayPush(&arena, u8, contentLength + 1);

    for(u32 i = 0; i < contentLength; i++) {
        content[i] = 0xaa;
    }
    int readBytes = fread(content, contentLength, 1, fd);
    fclose(fd);
    content[contentLength] = 0;

    size_t memoryUsed = arenaFreeBytes(&arena);
    String contentString = (String) { .data = content, .size = contentLength };
    String result = format(&arena, contentString);
    memoryUsed -= arenaFreeBytes(&arena);

    FILE *output = fopen("output.sol", "w");
    fwrite(result.data, result.size, 1, output);
    fclose(output);

    elapsed += __rdtsc();
    printf("Done in [%zu cycles][%f ms][%f MB/s]\n", elapsed, (double)elapsed / 4.2e9 * 1e3, ((contentLength / ((double)elapsed / 4.2e9)) / 1e6));
}
