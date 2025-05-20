#include "./src/utils.c"
#include "./src/tokenize.c"
#include "./src/yulLexer.c"
#include "./src/parser.c"
#include "./src/renderTree.c"

String format(Arena *arena, String input) {
    gCyclesTable[Measurement_Tokenize] = -readCPUTimer();
    TokenizeResult tokens = tokenize(input, arena);
    gCyclesTable[Measurement_Tokenize] += readCPUTimer();

    gCyclesTable[Measurement_Parse] = -readCPUTimer();
    Parser parser = createParser(tokens, arena);
    ASTNode node = parseSourceUnit(&parser);
    gCyclesTable[Measurement_Parse] += readCPUTimer();

    String result = renderTree(arena, node, input, tokens);

    return result;
}
