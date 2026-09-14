typedef enum YulTokenType_Enum {
    YulTokenType_None,
    YulTokenType_Let,
    YulTokenType_If,
    YulTokenType_For,
    YulTokenType_Leave,
    YulTokenType_Break,
    YulTokenType_Continue,
    YulTokenType_Function,
    YulTokenType_Switch,
    YulTokenType_Case,
    YulTokenType_Default,
    YulTokenType_LBrace,
    YulTokenType_RBrace,
    YulTokenType_LParen,
    YulTokenType_RParen,
    YulTokenType_Identifier,
    YulTokenType_Comment,
    YulTokenType_NumberLit,
    YulTokenType_StringLit,
    YulTokenType_HexNumberLit,
    YulTokenType_BoolLit,
    YulTokenType_HexStringLit,
    YulTokenType_Dot,
    YulTokenType_Comma,
    YulTokenType_ColonEqual,
    YulTokenType_RightArrow,
    YulTokenType_EOF,
    YulTokenType_Count,
} YulTokenType_Enum;

typedef u8 YulTokenType;

static YulTokenType
tokenToYulTokenLUT[TokenType_Count] = {
    [TokenType_As] = YulTokenType_Identifier,
    [TokenType_Do] = YulTokenType_Identifier,
    [TokenType_Is] = YulTokenType_Identifier,
    [TokenType_New] = YulTokenType_Identifier,
    [TokenType_Try] = YulTokenType_Identifier,
    [TokenType_Wei] = YulTokenType_Identifier,
    [TokenType_Days] = YulTokenType_Identifier,
    [TokenType_Else] = YulTokenType_Identifier,
    [TokenType_Emit] = YulTokenType_Identifier,
    [TokenType_Enum] = YulTokenType_Identifier,
    [TokenType_Gwei] = YulTokenType_Identifier,
    [TokenType_Pure] = YulTokenType_Identifier,
    [TokenType_Type] = YulTokenType_Identifier,
    [TokenType_View] = YulTokenType_Identifier,
    [TokenType_Catch] = YulTokenType_Identifier,
    [TokenType_Ether] = YulTokenType_Identifier,
    [TokenType_Event] = YulTokenType_Identifier,
    [TokenType_Hours] = YulTokenType_Identifier,
    [TokenType_Using] = YulTokenType_Identifier,
    [TokenType_Weeks] = YulTokenType_Identifier,
    [TokenType_While] = YulTokenType_Identifier,
    [TokenType_Years] = YulTokenType_Identifier,
    [TokenType_Delete] = YulTokenType_Identifier,
    [TokenType_Import] = YulTokenType_Identifier,
    [TokenType_Memory] = YulTokenType_Identifier,
    [TokenType_Pragma] = YulTokenType_Identifier,
    [TokenType_Public] = YulTokenType_Identifier,
    [TokenType_Return] = YulTokenType_Identifier,
    [TokenType_Struct] = YulTokenType_Identifier,
    [TokenType_Indexed] = YulTokenType_Identifier,
    [TokenType_Library] = YulTokenType_Identifier,
    [TokenType_Mapping] = YulTokenType_Identifier,
    [TokenType_Minutes] = YulTokenType_Identifier,
    [TokenType_Private] = YulTokenType_Identifier,
    [TokenType_Returns] = YulTokenType_Identifier,
    [TokenType_Seconds] = YulTokenType_Identifier,
    [TokenType_Storage] = YulTokenType_Identifier,
    [TokenType_Virtual] = YulTokenType_Identifier,
    [TokenType_Abstract] = YulTokenType_Identifier,
    [TokenType_Assembly] = YulTokenType_Identifier,
    [TokenType_Calldata] = YulTokenType_Identifier,
    [TokenType_Constant] = YulTokenType_Identifier,
    [TokenType_Contract] = YulTokenType_Identifier,
    [TokenType_External] = YulTokenType_Identifier,
    [TokenType_Fallback] = YulTokenType_Identifier,
    [TokenType_Internal] = YulTokenType_Identifier,
    [TokenType_Modifier] = YulTokenType_Identifier,
    [TokenType_Override] = YulTokenType_Identifier,
    [TokenType_Anonymous] = YulTokenType_Identifier,
    [TokenType_Immutable] = YulTokenType_Identifier,
    [TokenType_Interface] = YulTokenType_Identifier,
    [TokenType_Unchecked] = YulTokenType_Identifier,
    [TokenType_Constructor] = YulTokenType_Identifier,
    [TokenType_Layout] = YulTokenType_Identifier,
    [TokenType_At] = YulTokenType_Identifier,
    [TokenType_Transient] = YulTokenType_Identifier,
    [TokenType_From] = YulTokenType_Identifier,
    [TokenType_Receive] = YulTokenType_Identifier,
    [TokenType_Revert] = YulTokenType_Identifier,
    [TokenType_Error] = YulTokenType_Identifier,
    [TokenType_Global] = YulTokenType_Identifier,
    [TokenType_Payable] = YulTokenType_Identifier,
    [TokenType_If] = YulTokenType_If,
    [TokenType_For] = YulTokenType_For,
    [TokenType_Function] = YulTokenType_Function,
    [TokenType_Break] = YulTokenType_Break,
    [TokenType_Continue] = YulTokenType_Continue,
    [TokenType_LBrace] = YulTokenType_LBrace,
    [TokenType_RBrace] = YulTokenType_RBrace,
    [TokenType_LParen] = YulTokenType_LParen,
    [TokenType_RParen] = YulTokenType_RParen,
    [TokenType_NumberLit] = YulTokenType_NumberLit,
    [TokenType_StringLit] = YulTokenType_StringLit,
    [TokenType_HexNumberLit] = YulTokenType_HexNumberLit,
    [TokenType_False] = YulTokenType_BoolLit,
    [TokenType_True] = YulTokenType_BoolLit,
    [TokenType_HexStringLit] = YulTokenType_HexStringLit,
    [TokenType_Dot] = YulTokenType_Dot,
    [TokenType_Comma] = YulTokenType_Comma,
};

typedef struct YulLexer {
    TokenizeResult tokens;
    u32 tokenCount;
    u32 currentPosition;
    u32 lastPosition;
} YulLexer;

static YulLexer
createYulLexer(TokenizeResult tokens, u32 tokenCount, u32 currentPosition) {
    YulLexer result = {
        .tokens = tokens,
        .tokenCount = tokenCount,
        .currentPosition = currentPosition,
        .lastPosition = currentPosition,
    };

    return result;
}

// let     = 1
// case    = 4
// leave   = 5
// switch  = 0
// default = 3
//
// let     = 29797
// case    = 25971
// leave   = 25974
// switch  = 26723
// default = 29804

#define STR_U16(s) ((u16)((u8)(s)[0] | ((u16)(u8)(s)[1] << 8)))

static const u32 yulKeywordPreHashes[8] = {
    [1] = STR_U16("et"), [4] = STR_U16("se"),
    [5] = STR_U16("ve"), [0] = STR_U16("ch"),
    [3] = STR_U16("lt")
};

static const u32 yulKeywordLens[8] = {
    [1] = LIT_TO_STR("let").size,
    [4] = LIT_TO_STR("case").size,
    [5] = LIT_TO_STR("leave").size,
    [0] = LIT_TO_STR("switch").size,
    [3] = LIT_TO_STR("default").size
};

static const u8 yulKeywordStrings[8][8] = {
    [1] = { 'l', 'e', 't' },
    [4] = { 'c', 'a', 's', 'e',  },
    [5] = { 'l', 'e', 'a', 'v', 'e' },
    [0] = { 's', 'w', 'i', 't', 'c', 'h' },
    [3] = { 'd', 'e', 'f', 'a', 'u', 'l', 't' },
};

static const YulTokenType yulKeywordTypes[8] = {
    [1] = YulTokenType_Let,
    [4] = YulTokenType_Case,
    [5] = YulTokenType_Leave,
    [0] = YulTokenType_Switch,
    [3] = YulTokenType_Default,
};

static YulTokenType
categorizeYulSymbolPTable(String symbol) {
    u16 preHash = *((u16 *)(symbol.data + symbol.size - 2));
    u32 index = (preHash >> 2) & 7;

    if(yulKeywordLens[index] != symbol.size) return YulTokenType_Identifier;

    const u8 *actual = symbol.data;
    const u8 *expected = yulKeywordStrings[index];
    u64 diff = *(u64 *)actual ^ *(u64 *)expected;
    diff <<= (8 - symbol.size) << 3;
    return diff == 0 ? yulKeywordTypes[index] : YulTokenType_Identifier;
}

static YulTokenType
advanceYulToken(YulLexer *lexer) {
    if (lexer->currentPosition >= lexer->tokenCount) {
        return YulTokenType_EOF;
    }

    YulTokenType result = 0;
    lexer->lastPosition = lexer->currentPosition;
    TokenType tokenType = getTokenType(lexer->tokens, lexer->currentPosition);
    switch(tokenType) {
        case TokenType_Symbol: {
            String tokenString = getTokenString(lexer->tokens, lexer->currentPosition);
            result = categorizeYulSymbolPTable(tokenString);
        } break;
        case TokenType_Colon: {
            assert(getTokenType(lexer->tokens, ++lexer->currentPosition) == TokenType_Equal);
            result = YulTokenType_ColonEqual;
        } break;
        case TokenType_Minus: {
            assert(getTokenType(lexer->tokens, ++lexer->currentPosition) == TokenType_RTick);
            result = YulTokenType_RightArrow;
        } break;
        default: {
            result = tokenToYulTokenLUT[tokenType];
            assert(result != YulTokenType_None);
        }
    }

    lexer->currentPosition += 1;
    return result;
}

static YulTokenType
peekYulToken(YulLexer *lexer) {
    u32 position = lexer->currentPosition;
    u32 lastPosition = lexer->lastPosition;
    YulTokenType result = advanceYulToken(lexer);
    lexer->currentPosition = position;
    lexer->lastPosition = lastPosition;
    return result;
}

static bool
acceptYulToken(YulLexer *lexer, YulTokenType type) {
    u32 position = lexer->currentPosition;
    u32 lastPosition = lexer->lastPosition;
    YulTokenType result = advanceYulToken(lexer);
    bool matches = result == type; 
    if(matches) {
        return true;
    }

    lexer->currentPosition = position;
    lexer->lastPosition = lastPosition;
    return false;
}

static TokenId
peekYulLastToken(YulLexer *lexer) {
    return lexer->lastPosition;
}

static bool
yulIdentifierLUT[TokenType_Count] = {
    [TokenType_As] = true,
    [TokenType_Do] = true,
    [TokenType_Is] = true,
    [TokenType_New] = true,
    [TokenType_Try] = true,
    [TokenType_Wei] = true,
    [TokenType_Days] = true,
    [TokenType_Else] = true,
    [TokenType_From] = true,
    [TokenType_Emit] = true,
    [TokenType_Enum] = true,
    [TokenType_Gwei] = true,
    [TokenType_Pure] = true,
    [TokenType_Type] = true,
    [TokenType_View] = true,
    [TokenType_Error] = true,
    [TokenType_Catch] = true,
    [TokenType_Ether] = true,
    [TokenType_Event] = true,
    [TokenType_Hours] = true,
    [TokenType_Using] = true,
    [TokenType_Weeks] = true,
    [TokenType_While] = true,
    [TokenType_Years] = true,
    [TokenType_Delete] = true,
    [TokenType_Import] = true,
    [TokenType_Memory] = true,
    [TokenType_Symbol] = true,
    [TokenType_Revert] = true,
    [TokenType_Global] = true,
    [TokenType_Pragma] = true,
    [TokenType_Public] = true,
    [TokenType_Return] = true,
    [TokenType_Struct] = true,
    [TokenType_Indexed] = true,
    [TokenType_Library] = true,
    [TokenType_Mapping] = true,
    [TokenType_Minutes] = true,
    [TokenType_Private] = true,
    [TokenType_Returns] = true,
    [TokenType_Seconds] = true,
    [TokenType_Receive] = true,
    [TokenType_Storage] = true,
    [TokenType_Virtual] = true,
    [TokenType_Abstract] = true,
    [TokenType_Assembly] = true,
    [TokenType_Calldata] = true,
    [TokenType_Constant] = true,
    [TokenType_Contract] = true,
    [TokenType_External] = true,
    [TokenType_Fallback] = true,
    [TokenType_Internal] = true,
    [TokenType_Modifier] = true,
    [TokenType_Override] = true,
    [TokenType_Anonymous] = true,
    [TokenType_Immutable] = true,
    [TokenType_Interface] = true,
    [TokenType_Unchecked] = true,
    [TokenType_Constructor] = true,
    [TokenType_Payable] = true,
    [TokenType_At] = true,
    [TokenType_Transient] = true,
    [TokenType_Layout] = true,
};

static TokenId
parseYulIdentifier(YulLexer *lexer) {
    TokenType tokenType = getTokenType(lexer->tokens, lexer->currentPosition);

    if(yulIdentifierLUT[tokenType]) {
        lexer->currentPosition++;
        return lexer->currentPosition - 1;
    } else {
        return INVALID_TOKEN_ID;
    }
}
