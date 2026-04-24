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

typedef struct YulToken {
    YulTokenType type;
    String string;
} YulToken;

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

static YulToken
advanceYulToken(YulLexer *lexer) {
    if (lexer->currentPosition >= lexer->tokenCount) {
        YulToken result = {
            .type = YulTokenType_EOF,
        };
        return result;
    }

    YulToken result = { 0 };
    lexer->lastPosition = lexer->currentPosition;
    TokenType tokenType = getTokenType(lexer->tokens, lexer->currentPosition);
    String tokenString = getTokenString(lexer->tokens, lexer->currentPosition);
    lexer->currentPosition += 1;
    if(tokenToYulTokenLUT[tokenType] != YulTokenType_None) {
        result.type = tokenToYulTokenLUT[tokenType];
        result.string = tokenString;
    } else {
    switch(tokenType) {
        case TokenType_Symbol: {
            if(stringMatch(tokenString, LIT_TO_STR("let"))) {
                result.type = YulTokenType_Let;
            } else if(stringMatch(tokenString, LIT_TO_STR("case"))) {
                result.type = YulTokenType_Case;
            } else if(stringMatch(tokenString, LIT_TO_STR("leave"))) {
                result.type = YulTokenType_Leave;
            } else if(stringMatch(tokenString, LIT_TO_STR("switch"))) {
                result.type = YulTokenType_Switch;
            } else if(stringMatch(tokenString, LIT_TO_STR("default"))) {
                result.type = YulTokenType_Default;
            } else {
                result.type = YulTokenType_Identifier;
            }

            result.string = tokenString;
        } break;
        case TokenType_Colon: {
            assert(getTokenType(lexer->tokens, lexer->currentPosition++) == TokenType_Equal);
            result.string = tokenString;
            result.type = YulTokenType_ColonEqual;
        } break;
        case TokenType_Minus: {
            assert(getTokenType(lexer->tokens, lexer->currentPosition++) == TokenType_RTick);
            result.string = tokenString;
            result.type = YulTokenType_RightArrow;
        } break;
        default: {
            javascriptPrintString("Unknown token type in Yul lexer\n");
            javascriptPrintNumber(tokenType);
            assert(0);
        }
    }
    }

    return result;
}

static YulToken
peekYulToken(YulLexer *lexer) {
    u32 position = lexer->currentPosition;
    u32 lastPosition = lexer->lastPosition;
    YulToken result = advanceYulToken(lexer);
    lexer->currentPosition = position;
    lexer->lastPosition = lastPosition;
    return result;
}

static TokenId
peekYulLastToken(YulLexer *lexer) {
    return lexer->lastPosition;
}

static TokenId
parseYulIdentifier(YulLexer *lexer) {
    TokenType tokenType = getTokenType(lexer->tokens, lexer->currentPosition);

    u32 isIdent =
        tokenType == TokenType_As |
        tokenType == TokenType_Do |
        tokenType == TokenType_Is |
        tokenType == TokenType_New |
        tokenType == TokenType_Try |
        tokenType == TokenType_Wei |
        tokenType == TokenType_Days |
        tokenType == TokenType_Else |
        tokenType == TokenType_From |
        tokenType == TokenType_Emit |
        tokenType == TokenType_Enum |
        tokenType == TokenType_Gwei |
        tokenType == TokenType_Pure |
        tokenType == TokenType_Type |
        tokenType == TokenType_View |
        tokenType == TokenType_Error |
        tokenType == TokenType_Catch |
        tokenType == TokenType_Ether |
        tokenType == TokenType_Event |
        tokenType == TokenType_Hours |
        tokenType == TokenType_Using |
        tokenType == TokenType_Weeks |
        tokenType == TokenType_While |
        tokenType == TokenType_Years |
        tokenType == TokenType_Delete |
        tokenType == TokenType_Import |
        tokenType == TokenType_Memory |
        tokenType == TokenType_Symbol |
        tokenType == TokenType_Revert |
        tokenType == TokenType_Global |
        tokenType == TokenType_Pragma |
        tokenType == TokenType_Public |
        tokenType == TokenType_Return |
        tokenType == TokenType_Struct |
        tokenType == TokenType_Indexed |
        tokenType == TokenType_Library |
        tokenType == TokenType_Mapping |
        tokenType == TokenType_Minutes |
        tokenType == TokenType_Private |
        tokenType == TokenType_Returns |
        tokenType == TokenType_Seconds |
        tokenType == TokenType_Receive |
        tokenType == TokenType_Storage |
        tokenType == TokenType_Virtual |
        tokenType == TokenType_Abstract |
        tokenType == TokenType_Assembly |
        tokenType == TokenType_Calldata |
        tokenType == TokenType_Constant |
        tokenType == TokenType_Contract |
        tokenType == TokenType_External |
        tokenType == TokenType_Fallback |
        tokenType == TokenType_Internal |
        tokenType == TokenType_Modifier |
        tokenType == TokenType_Override |
        tokenType == TokenType_Anonymous |
        tokenType == TokenType_Immutable |
        tokenType == TokenType_Interface |
        tokenType == TokenType_Unchecked |
        tokenType == TokenType_Constructor |
        tokenType == TokenType_Payable;

    if(isIdent) {
        lexer->currentPosition++;
        return lexer->currentPosition - 1;
    } else {
        return INVALID_TOKEN_ID;
    }
}
