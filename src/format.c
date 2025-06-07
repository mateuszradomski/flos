#include "./src/utils.c"
#include "./src/tokenize.c"
#include "./src/yulLexer.c"
#include "./src/parser.c"
#include "./src/renderTree.c"

enum MeasurementId {
    Measurement_Tokenize = 0,
    Measurement_Parse,
    Measurement_BuildDoc,
    Measurement_RenderDoc,
};

typedef struct FormatResult {
    String source;
    u64 timings[8];
} FormatResult;

FormatResult format(Arena *arena, Arena *parseArena, String input) {
    FormatResult result = { 0 };

    result.timings[Measurement_Tokenize] -= readTimer();
    TokenizeResult tokens = tokenize(input, arena);
    result.timings[Measurement_Tokenize] += readTimer();

    result.timings[Measurement_Parse] -= readTimer();
    Parser parser = createParser(tokens, parseArena);
    ASTNode node = parseSourceUnit(&parser);
    result.timings[Measurement_Parse] += readTimer();

    result.timings[Measurement_BuildDoc] -= readTimer();
    Render render = createRender(arena, input, tokens);
    buildDocument(&render, &node, input, tokens);
    result.timings[Measurement_BuildDoc] += readTimer();

    result.timings[Measurement_RenderDoc] -= readTimer();
    renderDocument(&render);
    result.timings[Measurement_RenderDoc] += readTimer();

    dumpDocument(&render);

    result.source = stringTrimLeft((String){ .data = render.writer.data, .size = render.writer.size });
    return result;
}
