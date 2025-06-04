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

    result.timings[Measurement_Tokenize] -= readCPUTimer();
    TokenizeResult tokens = tokenize(input, arena);
    result.timings[Measurement_Tokenize] += readCPUTimer();

    result.timings[Measurement_Parse] -= readCPUTimer();
    Parser parser = createParser(tokens, parseArena);
    ASTNode node = parseSourceUnit(&parser);
    result.timings[Measurement_Parse] += readCPUTimer();

    result.timings[Measurement_BuildDoc] -= readCPUTimer();
    Render render = createRender(arena, input, tokens);
    buildDocument(&render, &node, input, tokens);
    result.timings[Measurement_BuildDoc] += readCPUTimer();

    result.timings[Measurement_RenderDoc] -= readCPUTimer();
    renderDocument(&render);
    result.timings[Measurement_RenderDoc] += readCPUTimer();

    dumpDocument(&render);

    result.source = stringTrimLeft((String){ .data = render.writer.data, .size = render.writer.size });
    return result;
}
