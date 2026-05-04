#include "./src/utils.c"

typedef struct FormatConfig {
    u32 maxLineWidth;
    u32 indentWidth;
} FormatConfig;

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

static FormatConfig
defaultFormatConfig() {
    return (FormatConfig){
        .maxLineWidth = 120,
        .indentWidth = 4,
    };
};

FormatResult formatWithConfig(Arena *arena, Arena *parseArena, String input, FormatConfig config) {
    FormatResult result = { 0 };

    if(input.size == 0) {
        result.source = input;
        return result;
    }

    result.timings[Measurement_Tokenize] -= readTimer();
    TokenizeResult tokens = tokenize(input, arena);
    result.timings[Measurement_Tokenize] += readTimer();

    result.timings[Measurement_Parse] -= readTimer();
    Parser parser = createParser(tokens, parseArena);
    ASTNode node = parseSourceUnit(&parser);
    result.timings[Measurement_Parse] += readTimer();

    result.timings[Measurement_BuildDoc] -= readTimer();
    Render render = createRender(arena, input, tokens, config);
    buildDocument(&render, &node, input, tokens);
    result.timings[Measurement_BuildDoc] += readTimer();

    result.timings[Measurement_RenderDoc] -= readTimer();
    renderDocument(&render);
    result.timings[Measurement_RenderDoc] += readTimer();

    dumpDocument(&render);

    result.source = stringTrimLeft((String){ .data = render.writer.data, .size = render.writer.size });
    return result;
}

FormatResult format(Arena *arena, Arena *parseArena, String input) {
    FormatConfig defaultConfig = defaultFormatConfig();
    return formatWithConfig(arena, parseArena, input, defaultConfig);
}
