#include "./src/utils.c"
#include "./src/tokenize.c"
#include "./src/yulLexer.c"
#include "./src/parser.c"
#include "./src/renderTree.c"

String format(Arena *arena, String input) {
    TokenizeResult tokens = tokenize(input, arena);
    Parser parser = createParser(tokens, arena);
    ASTNode node = parseSourceUnit(&parser);
    String result = renderTree(arena, node, input, tokens);

    return result;
}
