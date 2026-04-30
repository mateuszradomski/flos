#include "baseTypeTable.h"

typedef enum ASTNodeType_Enum {
    ASTNodeType_None,
    ASTNodeType_SourceUnit,
    ASTNodeType_Import, ASTNodeType_EnumDefinition,
    ASTNodeType_Struct,
    ASTNodeType_BaseType,
    ASTNodeType_FunctionType,
    ASTNodeType_MappingType,
    ASTNodeType_IdentifierPath,
    ASTNodeType_ArrayType,
    ASTNodeType_Error,
    ASTNodeType_Event,
    ASTNodeType_Typedef,
    ASTNodeType_ConstVariable,
    ASTNodeType_NumberLitExpression,
    ASTNodeType_StringLitExpression,
    ASTNodeType_BoolLitExpression,
    ASTNodeType_IdentifierExpression,
    ASTNodeType_BinaryExpression,
    ASTNodeType_TupleExpression,
    ASTNodeType_UnaryExpression,
    ASTNodeType_FunctionCallExpression,
    ASTNodeType_MemberAccessExpression,
    ASTNodeType_ArrayAccessExpression,
    ASTNodeType_FunctionDefinition,
    ASTNodeType_BlockStatement,
    ASTNodeType_ReturnStatement,
    ASTNodeType_ExpressionStatement,
    ASTNodeType_IfStatement,
    ASTNodeType_VariableDeclarationStatement,
    ASTNodeType_VariableDeclaration,
    ASTNodeType_NewExpression,
    ASTNodeType_VariableDeclarationTupleStatement,
    ASTNodeType_WhileStatement,
    ASTNodeType_ContractDefinition,
    ASTNodeType_RevertStatement,
    ASTNodeType_StateVariableDeclaration,
    ASTNodeType_LibraryDefinition,
    ASTNodeType_TerneryExpression,
    ASTNodeType_ForStatement,
    ASTNodeType_BreakStatement,
    ASTNodeType_ContinueStatement,
    ASTNodeType_UnaryExpressionPostfix,
    ASTNodeType_HexStringLitExpression,
    ASTNodeType_ArraySliceExpression,
    ASTNodeType_UncheckedBlockStatement,
    ASTNodeType_ModifierDefinition,
    ASTNodeType_FallbackFunction,
    ASTNodeType_ReceiveFunction,
    ASTNodeType_EmitStatement,
    ASTNodeType_ConstructorDefinition,
    ASTNodeType_NamedParameterExpression,
    ASTNodeType_InterfaceDefinition,
    ASTNodeType_AbstractContractDefinition,
    ASTNodeType_InheritanceSpecifier,
    ASTNodeType_Pragma,
    ASTNodeType_ModifierInvocation,
    ASTNodeType_Using,
    ASTNodeType_UnicodeStringLitExpression,
    ASTNodeType_InlineArrayExpression,
    ASTNodeType_DoWhileStatement,
    ASTNodeType_TryStatement,
    ASTNodeType_CatchStatement,
    ASTNodeType_AssemblyStatement,
    ASTNodeType_YulBlockStatement,
    ASTNodeType_YulVariableDeclaration,
    ASTNodeType_YulNumberLitExpression,
    ASTNodeType_YulStringLitExpression,
    ASTNodeType_YulHexNumberLitExpression,
    ASTNodeType_YulBoolLitExpression,
    ASTNodeType_YulHexStringLitExpression,
    ASTNodeType_YulMemberAccessExpression,
    ASTNodeType_YulFunctionCallExpression,
    ASTNodeType_YulVariableAssignment,
    ASTNodeType_YulIfStatement,
    ASTNodeType_YulForStatement,
    ASTNodeType_YulLeaveStatement,
    ASTNodeType_YulBreakStatement,
    ASTNodeType_YulContinueStatement,
    ASTNodeType_YulFunctionDefinition,
    ASTNodeType_YulSwitchStatement,
    ASTNodeType_YulCaseStatement,
    ASTNodeType_Count,
} ASTNodeType_Enum;

typedef u32 ASTNodeType;

typedef struct ASTNode ASTNode;

// NOTE(radomski): Represents not only empty but missing entirely
#define MISSING_ELEMENT ((void *)((u64)0xCAFEBABECAFEBABE))

typedef struct ASTNodeStruct {
    TokenId nameTokenId;
    ASTNode *firstMember;
} ASTNodeStruct;

typedef struct ASTNodeBaseType {
    TokenId typeName;
    u32 payable;
} ASTNodeBaseType;

typedef struct ASTNodeIdentifierPath {
    TokenIdList identifiers;
} ASTNodeIdentifierPath;

typedef struct ASTNodeMapping {
    ASTNode *keyType;
    TokenId keyIdentifier;
    ASTNode *valueType;
    TokenId valueIdentifier;
} ASTNodeMapping;

typedef struct ASTNodeFunctionType {
    ASTNode *firstParameter;
    TokenId visibility;
    TokenId stateMutability;
    ASTNode *firstReturnParameter;
} ASTNodeFunctionType;

typedef struct ASTNodeArrayType {
    ASTNode *elementType;
    ASTNode *lengthExpression;
} ASTNodeArrayType;

typedef struct ASTNodeError {
    TokenId identifier;
    ASTNode *firstParameter;
} ASTNodeError;

typedef struct ASTNodeEvent {
    TokenId identifier;
    ASTNode *firstParameter;
    u32 anonymous;
} ASTNodeEvent;

typedef struct ASTNodeTypedef {
    TokenId identifier;
    ASTNode *type;
} ASTNodeTypedef;

typedef struct ASTNodeConstVariable {
    ASTNode *type;
    ASTNode *expression;
    TokenId identifier;
    TokenId visibility;
    TokenId mutability;
    TokenId override;
    ASTNode *firstOverride;
} ASTNodeConstVariable;

typedef struct ASTNodeNumberLitExpression {
    TokenId value;
    TokenId subdenomination;
} ASTNodeNumberLitExpression;

typedef struct ASTNodeStringLitExpression {
    TokenIdList values;
} ASTNodeStringLitExpression;

typedef struct ASTNodeTokenLitExpression {
    TokenId value;
} ASTNodeTokenLitExpression;

typedef struct ASTNodeBinaryExpression {
    ASTNode *left;
    ASTNode *right;
    u32 operator;
} ASTNodeBinaryExpression;

typedef struct ASTNodeTupleExpression {
    ASTNode *firstElement;
} ASTNodeTupleExpression;

typedef struct ASTNodeUnaryExpression {
    u32 operator;
    ASTNode *subExpression;
} ASTNodeUnaryExpression;

typedef struct ASTNodeNewExpression {
    ASTNode *typeName;
} ASTNodeNewExpression;

typedef struct ASTNodeFunctionCallExpression {
    ASTNode *expression;
    ASTNode *firstArgumentExpression;
    TokenIdList argumentsName;
} ASTNodeFunctionCallExpression;

typedef struct ASTNodeMemberAccessExpression {
    ASTNode *expression;
    TokenId memberName;
} ASTNodeMemberAccessExpression;

typedef struct ASTNodeArrayAccessExpression {
    ASTNode *expression;
    ASTNode *indexExpression;
} ASTNodeArrayAccessExpression;

typedef struct ASTNodeArraySliceExpression {
    ASTNode *expression;
    ASTNode *leftFenceExpression;
    ASTNode *rightFenceExpression;
} ASTNodeArraySliceExpression;

typedef struct ASTNodeTerneryExpression {
    ASTNode *condition;
    ASTNode *trueExpression;
    ASTNode *falseExpression;
} ASTNodeTerneryExpression;

typedef struct ASTNodeFunctionDefinition {
    TokenId name;
    ASTNode *firstParameter;
    TokenId visibility;
    TokenId stateMutability;
    TokenId virtual;
    TokenId override;
    ASTNode *firstReturnParameter;
    ASTNode *body;
    ASTNode *firstOverride;
    ASTNode *firstModifier;
} ASTNodeFunctionDefinition;

typedef struct ASTNodeBlockStatement {
    ASTNode *firstStatement;
} ASTNodeBlockStatement;

typedef struct ASTNodeReturnStatement {
    ASTNode *expression;
} ASTNodeReturnStatement;

typedef struct ASTNodeIfStatement {
    ASTNode *conditionExpression;
    ASTNode *trueStatement;
    ASTNode *falseStatement;
} ASTNodeIfStatement;

typedef struct ASTNodeVariableDeclaration {
    ASTNode *type;
    u32 name;
    TokenId dataLocation;
} ASTNodeVariableDeclaration;

typedef struct ASTNodeVariableDeclarationStatement {
    ASTNode *variableDeclaration;
    ASTNode *initialValue;
} ASTNodeVariableDeclarationStatement;

typedef struct ASTNodeVariableDeclarationTupleStatement {
    ASTNode *firstDeclaration;
    ASTNode *initialValue;
} ASTNodeVariableDeclarationTupleStatement;

typedef struct ASTNodeWhileStatement {
    ASTNode *expression;
    ASTNode *body;
} ASTNodeWhileStatement;

typedef struct ASTNodeInheritanceSpecifier {
    ASTNode *identifier;
    TokenIdList argumentsName;
    ASTNode *firstArgumentExpression;
} ASTNodeInheritanceSpecifier;

typedef ASTNodeInheritanceSpecifier ASTNodeModifierInvocation;

typedef struct ASTNodeContractDefinition {
    TokenId name;
    ASTNode *firstElement;
    ASTNode *firstBaseContract;
} ASTNodeContractDefinition;

typedef struct ASTNodeLibraryDefinition {
    TokenId name;
    ASTNode *firstElement;
} ASTNodeLibraryDefinition;

typedef struct ASTNodeForStatement {
    ASTNode *variableStatement;
    ASTNode *conditionExpression;
    ASTNode *incrementExpression;
    ASTNode *body;
} ASTNodeForStatement;

typedef struct ASTNodeUncheckedBlockStatement {
    ASTNode *block;
} ASTNodeUncheckedBlockStatement;

typedef struct ASTNodeRevertStatement {
    ASTNode *expression;
} ASTNodeRevertStatement;

typedef struct ASTNodeEmitStatement {
    ASTNode *expression;
} ASTNodeEmitStatement;

typedef struct ASTNodeConstructorDefinition {
    ASTNode *firstParameter;
    TokenId visibility;
    TokenId stateMutability;
    TokenId virtual;
    ASTNode *firstModifier;
    ASTNode *body;
} ASTNodeConstructorDefinition;

typedef struct ASTNodeNamedParametersExpression {
    ASTNode *expression;
    u32 listStartToken; // NOTE(radomski): unused
    u32 listEndToken; // NOTE(radomski): unused
    TokenIdList names;
    ASTNode *firstExpression;
} ASTNodeNamedParametersExpression;

typedef struct ASTNodeNameValue {
    TokenId name;
    ASTNode *value;
} ASTNodeNameValue;

typedef struct ASTNodePragma {
    TokenId major;
    TokenIdList following;
} ASTNodePragma;

typedef struct ASTNodeUsing {
    ASTNode *firstIdentifier;
    U16List operators;
    ASTNode *forType;
    TokenId global;
    u8 onLibrary;
} ASTNodeUsing;

typedef struct ASTNodeInlineArrayExpression {
    ASTNode *firstExpression;
} ASTNodeInlineArrayExpression;

typedef struct ASTNodeTryStatement {
    ASTNode *expression;
    ASTNode *firstReturnParameter;
    ASTNode *body;
    ASTNode *firstCatch;
} ASTNodeTryStatement;

typedef struct ASTNodeCatchStatement {
    TokenId identifier;
    ASTNode *firstParameter;
    ASTNode *body;
} ASTNodeCatchStatement;

typedef struct ASTNodeAssemblyStatement {
    u8 isEVMAsm;
    TokenIdList flags;
    ASTNode *body;
} ASTNodeAssemblyStatement;

typedef struct ASTNodeYulVariableDeclaration {
    TokenIdList identifiers;
    ASTNode *value;
} ASTNodeYulVariableDeclaration;

typedef struct ASTNodeYulVariableAssignment {
    ASTNode *firstPath;
    ASTNode *value;
} ASTNodeYulVariableAssignment;

typedef struct ASTNodeYulNumberLitExpression {
    TokenId value;
} ASTNodeYulNumberLitExpression;

typedef struct ASTNodeYulIdentifierPathExpression {
    u32 count;
    TokenId identifiers[2];
} ASTNodeYulIdentifierPathExpression;

typedef struct ASTNodeYulFunctionCallExpression {
    TokenId identifier;
    ASTNode *firstArgument;
} ASTNodeYulFunctionCallExpression;

typedef struct ASTNodeYulIfStatement {
    ASTNode *expression;
    ASTNode *body;
} ASTNodeYulIfStatement;

typedef struct ASTNodeYulForStatement {
    ASTNode *variableDeclaration;
    ASTNode *condition;
    ASTNode *increment;
    ASTNode *body;
} ASTNodeYulForStatement;

typedef struct ASTNodeYulFunctionDefinition {
    TokenId identifier;
    TokenIdList parameters;
    TokenIdList returnParameters;
    ASTNode *body;
} ASTNodeYulFunctionDefinition;

typedef struct ASTNodeYulCase {
    ASTNode *literal;
    ASTNode *block;
} ASTNodeYulCase;

typedef struct ASTNodeYulSwitchStatement {
    ASTNode *expression;
    ASTNode *defaultBlock;
    ASTNode *firstCase;
} ASTNodeYulSwitchStatement;

typedef struct ASTNodeSourceUnit {
    ASTNode *firstChild;
} ASTNodeSourceUnit;

typedef struct ASTNodeImport {
    TokenId pathTokenId;
    TokenId unitAliasTokenId;
    TokenIdList symbols;
    TokenIdList symbolAliases;
} ASTNodeImport;

typedef struct ASTNodeEnum {
    TokenId nameTokenId;
    TokenIdList values;
} ASTNodeEnum;

typedef struct ASTNode {
    ASTNodeType type;

    u32 startToken;
    u32 endToken;

    ASTNode *next;

    union {
        ASTNodeSourceUnit sourceUnitNode;
        ASTNodeImport importNode;
        ASTNodePragma pragmaNode;
        ASTNodeEnum enumNode;
        ASTNodeStruct structNode;
        ASTNodeBaseType baseTypeNode;
        ASTNodeIdentifierPath identifierPathNode;
        ASTNodeMapping mappingNode;
        ASTNodeFunctionType functionTypeNode;
        ASTNodeArrayType arrayTypeNode;
        ASTNodeError errorNode;
        ASTNodeEvent eventNode;
        ASTNodeTypedef typedefNode;
        ASTNodeConstVariable constVariableNode;
        ASTNodeConstVariable stateVariableDeclarataion;
        ASTNodeNumberLitExpression numberLitExpressionNode;
        ASTNodeStringLitExpression stringLitExpressionNode;
        ASTNodeTokenLitExpression boolLitExpressionNode;
        ASTNodeTokenLitExpression identifierExpressionNode;
        ASTNodeBinaryExpression binaryExpressionNode;
        ASTNodeTupleExpression tupleExpressionNode;
        ASTNodeUnaryExpression unaryExpressionNode;
        ASTNodeNewExpression newExpressionNode;
        ASTNodeFunctionCallExpression functionCallExpressionNode;
        ASTNodeMemberAccessExpression memberAccessExpressionNode;
        ASTNodeArrayAccessExpression arrayAccessExpressionNode;
        ASTNodeArraySliceExpression arraySliceExpressionNode;
        ASTNodeTerneryExpression terneryExpressionNode;
        ASTNodeNamedParametersExpression namedParametersExpressionNode;
        ASTNodeInlineArrayExpression inlineArrayExpressionNode;
        ASTNodeFunctionDefinition functionDefinitionNode;
        ASTNodeBlockStatement blockStatementNode;
        ASTNodeUncheckedBlockStatement uncheckedBlockStatementNode;
        ASTNodeReturnStatement returnStatementNode;
        ASTNodeReturnStatement expressionStatementNode;
        ASTNodeIfStatement ifStatementNode;
        ASTNodeVariableDeclarationStatement variableDeclarationStatementNode;
        ASTNodeVariableDeclaration variableDeclarationNode;
        ASTNodeVariableDeclarationTupleStatement variableDeclarationTupleStatementNode;
        ASTNodeWhileStatement whileStatementNode;
        ASTNodeWhileStatement doWhileStatementNode;
        ASTNodeInheritanceSpecifier inheritanceSpecifierNode;
        ASTNodeModifierInvocation modifierInvocationNode;
        ASTNodeContractDefinition contractDefinitionNode;
        ASTNodeLibraryDefinition libraryDefinitionNode;
        ASTNodeRevertStatement revertStatementNode;
        ASTNodeForStatement forStatementNode;
        ASTNodeEmitStatement emitStatementNode;
        ASTNodeTryStatement tryStatementNode;
        ASTNodeCatchStatement catchStatementNode;
        ASTNodeAssemblyStatement assemblyStatementNode;
        ASTNodeConstructorDefinition constructorDefinitionNode;
        ASTNodeNameValue nameValueNode;
        ASTNodeUsing usingNode;

        ASTNodeBlockStatement yulBlockStatementNode;
        ASTNodeYulVariableDeclaration yulVariableDeclarationNode;
        ASTNodeYulNumberLitExpression yulNumberLitExpressionNode;
        ASTNodeYulNumberLitExpression yulStringLitExpressionNode;
        ASTNodeYulNumberLitExpression yulHexNumberLitExpressionNode;
        ASTNodeYulNumberLitExpression yulBoolLitExpressionNode;
        ASTNodeYulNumberLitExpression yulHexStringLitExpressionNode;
        ASTNodeYulIdentifierPathExpression yulIdentifierPathExpressionNode;
        ASTNodeYulFunctionCallExpression yulFunctionCallExpressionNode;
        ASTNodeYulVariableAssignment yulVariableAssignmentNode;
        ASTNodeYulIfStatement yulIfStatementNode;
        ASTNodeYulForStatement yulForStatementNode;
        ASTNodeYulFunctionDefinition yulFunctionDefinitionNode;
        ASTNodeYulSwitchStatement yulSwitchStatementNode;
        ASTNodeYulCase yulCaseNode;
    };
} ASTNode;

typedef struct Parser {
    TokenizeResult tokens;
    u32 tokenCount;
    u32 current;
    ASTNode *nodes;
    u32 nodeCount;
    u32 nodeCapacity;

    Arena *arena;
} Parser;

static Parser
createParser(TokenizeResult tokens, Arena *arena) {
    Parser parser = {
        .tokens = tokens,
        .tokenCount = tokens.count,
        .current = 0,
        .arena = arena,
        .nodeCount = 0,
        .nodeCapacity = tokens.count, // NOTE(radomski): Should be tokens.count / 1.5 with growable container
    };

    parser.nodes = arrayPush(arena, ASTNode, parser.nodeCapacity);

    return parser;
}

static ASTNode *
allocateNode(Parser *parser) {
    assert(parser->nodeCount < parser->nodeCapacity);
    return &parser->nodes[parser->nodeCount++];
}

#define reportError(parser, userErrorFormat, ...) _reportError(parser, __FILE__, __LINE__, userErrorFormat, ##__VA_ARGS__)
#define assertError(condition, parser, userErrorFormat, ...) _assertError((bool)(condition), parser, __FILE__, __LINE__, userErrorFormat, ##__VA_ARGS__)

static void
reportErrorVarArgs(Parser *parser, const char *file, u32 line, const char *userErrorFormat, va_list args) {
#ifdef WASM
    va_list copiedArgs;
    va_copy(copiedArgs, args);
    String userError = stringPushfv(parser->arena, userErrorFormat, copiedArgs);
    va_end(copiedArgs);

    u32 byteOffset = parser->tokens.tokenStrings[parser->current].data - parser->tokens.tokenStrings[0].data;
    String error = stringPushf(parser->arena, "ERROR [%s:%d] - [%d]: %S", file, line, byteOffset, userError);

    memoryUsed = arenaTakenBytes(parser->arena);
    bumpPointerArenaTop = (unsigned int)structPush(parser->arena, int);
    javascriptThrowErrorStringPtr(&error);
    unreachable();
#else
    assert(false);
#endif
}

static void
_reportError(Parser *parser, const char *file, u32 line, const char *userErrorFormat, ...) {
    va_list args;
    va_start(args, userErrorFormat);
    reportErrorVarArgs(parser, file, line, userErrorFormat, args);
    va_end(args);
}

static void
_assertError(bool condition, Parser *parser, const char *file, u32 line, const char *userErrorFormat, ...) {
    if(!condition) {
        va_list args;
        va_start(args, userErrorFormat);
        reportErrorVarArgs(parser, file, line, userErrorFormat, args);
        va_end(args);
    }
}

static TokenType
peekTokenType(Parser *parser) {
    return getTokenType(parser->tokens, parser->current);
}

static String
peekTokenString(Parser *parser) {
    return getTokenString(parser->tokens, parser->current);
}

static u32
peekLastTokenId(Parser *parser) {
    assertError(parser->current > 0, parser, "No tokens to peek");
    return parser->current - 1;
}

static TokenType
peekLastTokenType(Parser *parser) {
    assertError(parser->current > 0, parser, "No tokens to peek");
    return getTokenType(parser->tokens, parser->current - 1);
}

static String
peekLastTokenString(Parser *parser) {
    assertError(parser->current > 0, parser, "No tokens to peek");
    return getTokenString(parser->tokens, parser->current - 1);
}

static TokenType
advanceToken(Parser *parser) {
    return getTokenType(parser->tokens, parser->current++);
}

static bool
acceptToken(Parser *parser, TokenType type) {
    bool result = peekTokenType(parser) == type;
    parser->current += result ? 1 : 0;
    return result;
}

static bool
nextTokenIs(Parser *parser, TokenType type) {
    return peekTokenType(parser) == type;
}

static void
_expectToken(Parser *parser, TokenType type, const char *file, u32 line) {
    bool success = acceptToken(parser, type);
    if(!success) {
        _reportError(parser, file, line, "Unexpected token => encountered (%S) but wanted (%S)", tokenTypeToString(peekTokenType(parser)), tokenTypeToString(type));
    }
}

#define expectToken(parser, type) _expectToken(parser, type, __FILE__, __LINE__)

static u32
getCurrentParserPosition(Parser *parser) {
    return parser->current;
}

static void
setCurrentParserPosition(Parser *parser, u32 newPosition) {
    parser->current = newPosition;
}

static bool parseExpression(Parser *parser, ASTNode *node);

static bool
acceptYulToken(YulLexer *lexer, YulTokenType type) {
    if(peekYulToken(lexer).type == type) {
        advanceYulToken(lexer);
        return true;
    }

    return false;
}

#define expectYulToken(parser, lexer, type) _expectYulToken(parser, lexer, type, __FILE__, __LINE__)

static void
_expectYulToken(Parser *parser, YulLexer *lexer, YulTokenType type, const char *file, u32 line) {
    bool success = acceptYulToken(lexer, type);
    if(!success) {
        _reportError(parser, file, line, "Unexpected token => encountered (%d) but wanted (%d)", peekYulToken(lexer).type, type);
    }
}

static TokenId
parseIdentifier(Parser *parser) {
    TokenType tokenType = peekTokenType(parser);

    u32 isIdent =
        tokenType == TokenType_Symbol |
        tokenType == TokenType_From |
        tokenType == TokenType_Receive |
        tokenType == TokenType_Revert |
        tokenType == TokenType_Error |
        tokenType == TokenType_Global |
        tokenType == TokenType_Payable |
        tokenType == TokenType_Szabo |
        tokenType == TokenType_Finney;

    if(isIdent) {
        acceptToken(parser, tokenType);
        return peekLastTokenId(parser);
    } else {
        return INVALID_TOKEN_ID;
    }

    return peekLastTokenId(parser);
}

static TokenId
parseSubdenomination(Parser *parser) {
    TokenType tokenType = peekTokenType(parser);

    u32 isSubdenomination =
        tokenType == TokenType_Wei |
        tokenType == TokenType_Gwei |
        tokenType == TokenType_Ether |
        tokenType == TokenType_Seconds |
        tokenType == TokenType_Minutes |
        tokenType == TokenType_Hours |
        tokenType == TokenType_Days |
        tokenType == TokenType_Weeks |
        tokenType == TokenType_Years |
        tokenType == TokenType_Finney |
        tokenType == TokenType_Szabo;

    if(isSubdenomination) {
        acceptToken(parser, tokenType);
        return peekLastTokenId(parser);
    } else {
        return INVALID_TOKEN_ID;
    }
}

static bool
isFixedPointNumberType(String string, String fixedPointPrefix) {
    if(stringStartsWith(string, fixedPointPrefix)) {
        String suffix = {
            .data = string.data + fixedPointPrefix.size,
            .size = string.size - fixedPointPrefix.size
        };

        SplitIterator it = stringSplit(suffix, 'x');
        String MString = stringNextInSplit(&it);
        String NString = stringNextInSplit(&it);

        if(MString.data == 0x0 && NString.data == 0x0) {
            return false;
        }

        if(!stringIsInteger(MString) || !stringIsInteger(NString)) {
            return false;
        }

        u32 M = stringToInteger(MString);
        u32 N = stringToInteger(NString);
        bool isMValid = (M >= 8 && M <= 256 && (M % 8) == 0);
        bool isNValid = (N <= 80);

        return isMValid && isNValid;
    }

    return false;
}

static u32
baseTypePreHash(String s) {
    u32 v = *(u32*)(s.data + s.size - 4);
    return (v >> 8) | ((u32)(u8)s.data[0] << 24);
}

static bool
isBaseTypeName(String string) {
    const u8 *actual = string.data;
    u32 size = (u32)string.size;

    u32 preHash = baseTypePreHash(string);
    u32 index = (preHash * baseTypeHashC) >> baseTypeHashS;

    if(baseTypePreHashes[index] != preHash) { return false; }
    if(baseTypeLens[index] != size)         { return false; }

    const u8 *expected = (const u8 *)baseTypeTexts[index];
    u64 diff = *(u64 *)actual ^ *(u64 *)expected;
    diff <<= (8 - size) << 3;

    if(diff == 0) { return true; }

    if(string.size >= 7 && string.size <= 12) {
        switch(string.data[0]) {
            case 'u': {
                return isFixedPointNumberType(string, LIT_TO_STR("ufixed"));
            }
            case 'f': {
                return isFixedPointNumberType(string, LIT_TO_STR("fixed"));
            }
        }
    }

    return false;
}

static bool parseType(Parser *parser, ASTNode *node);

static bool
parseVariableDeclarationIntoList(Parser *parser, ASTNode **first, ASTNode **last) {
    ASTNode *node = allocateNode(parser);
    node->type = ASTNodeType_VariableDeclaration;
    node->startToken = parser->current;
    ASTNodeVariableDeclaration *variableDeclaration = &node->variableDeclarationNode;

    variableDeclaration->type = allocateNode(parser);
    bool isSuccess = parseType(parser, variableDeclaration->type);
    if(!isSuccess) {
        return false;
    }

    variableDeclaration->dataLocation = INVALID_TOKEN_ID;
    if(acceptToken(parser, TokenType_Memory)) {
        variableDeclaration->dataLocation = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_Storage)) {
        variableDeclaration->dataLocation = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_Calldata)) {
        variableDeclaration->dataLocation = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_Indexed)) {
        variableDeclaration->dataLocation = peekLastTokenId(parser);
    }

    variableDeclaration->name = parseIdentifier(parser);
    node->endToken = parser->current - 1;

    SLL_QUEUE_PUSH(*first, *last, node);

    return true;
}

static void
parseFunctionParameters(Parser *parser, ASTNode **first, ASTNode **last) {
    parseVariableDeclarationIntoList(parser, first, last);
    while(acceptToken(parser, TokenType_Comma)) {
        assertError(parseVariableDeclarationIntoList(parser, first, last),
                    parser, "Expected variable declaration after comma");
    }
}

static void
parseIdentifierPath(Parser *parser, ASTNode *node) {
    node->startToken = parser->current;
    node->type = ASTNodeType_IdentifierPath;
    do {
        TokenId nextIdentifier = parseIdentifier(parser);
        assertError(nextIdentifier != INVALID_TOKEN_ID, parser,
                    "Expected identifier after dot, received (%S)", tokenTypeToString(peekTokenType(parser)));
        listPushTokenId(&node->identifierPathNode.identifiers, nextIdentifier, parser->arena);
    } while(acceptToken(parser, TokenType_Dot));
    node->endToken = parser->current - 1;
}

static bool
parseType(Parser *parser, ASTNode *node) {
    TokenId identifier = INVALID_TOKEN_ID;

    u32 startToken = parser->current;
    if(acceptToken(parser, TokenType_Mapping)) {
        node->startToken = startToken;
        node->type = ASTNodeType_MappingType;
        ASTNodeMapping *mapping = &node->mappingNode;

        expectToken(parser, TokenType_LParen);
        mapping->keyType = allocateNode(parser);
        parseType(parser, mapping->keyType);
        assertError(mapping->keyType->type == ASTNodeType_BaseType ||
                    mapping->keyType->type == ASTNodeType_IdentifierPath,
                    parser, "Mapping key type must be a base type or an identifier path");
        mapping->keyIdentifier = parseIdentifier(parser);

        expectToken(parser, TokenType_Equal);
        expectToken(parser, TokenType_RTick);

        mapping->valueType = allocateNode(parser);
        parseType(parser, mapping->valueType);
        mapping->valueIdentifier = parseIdentifier(parser);
        expectToken(parser, TokenType_RParen);
    } else if(acceptToken(parser, TokenType_Function)) {
        if(!acceptToken(parser, TokenType_LParen)) {
            return false;
        }

        node->startToken = startToken;
        node->type = ASTNodeType_FunctionType;
        ASTNodeFunctionType *function = &node->functionTypeNode;

        ASTNode *lastParameter = 0x0;
        if(!acceptToken(parser, TokenType_RParen)) {
            parseFunctionParameters(parser, &function->firstParameter, &lastParameter);
            expectToken(parser, TokenType_RParen);
        }

        function->stateMutability = INVALID_TOKEN_ID;
        function->visibility = INVALID_TOKEN_ID;
        for(;;) {
            if(acceptToken(parser, TokenType_Internal)) {
                if(function->visibility != INVALID_TOKEN_ID) {
                    parser->current -= 1;
                    break;
                }
                function->visibility = peekLastTokenId(parser);
            } else if(acceptToken(parser, TokenType_External)) {
                if(function->visibility != INVALID_TOKEN_ID) {
                    parser->current -= 1;
                    break;
                }
                function->visibility = peekLastTokenId(parser);
            } else if(acceptToken(parser, TokenType_Private)) {
                if(function->visibility != INVALID_TOKEN_ID) {
                    parser->current -= 1;
                    break;
                }
                function->visibility = peekLastTokenId(parser);
            } else if(acceptToken(parser, TokenType_Public)) {
                if(function->visibility != INVALID_TOKEN_ID) {
                    parser->current -= 1;
                    break;
                }
                function->visibility = peekLastTokenId(parser);
            } else if(acceptToken(parser, TokenType_Pure)) {
                function->stateMutability = peekLastTokenId(parser);
            } else if(acceptToken(parser, TokenType_View)) {
                function->stateMutability = peekLastTokenId(parser);
            } else if(acceptToken(parser, TokenType_Payable)) {
                function->stateMutability = peekLastTokenId(parser);
            } else {
                break;
            }
        }

        if(acceptToken(parser, TokenType_Returns)) {
            expectToken(parser, TokenType_LParen);

            ASTNode *lastReturnParameter = 0x0;
            if(!acceptToken(parser, TokenType_RParen)) {
                parseFunctionParameters(parser, &function->firstReturnParameter, &lastReturnParameter);
                expectToken(parser, TokenType_RParen);
            }
        }
    } else if((identifier = parseIdentifier(parser)) != INVALID_TOKEN_ID) {
        if(isBaseTypeName(getTokenString(parser->tokens, identifier))) {
            node->startToken = startToken;
            node->type = ASTNodeType_BaseType;
            node->baseTypeNode.typeName = identifier;

            node->baseTypeNode.payable = 0;
            if(acceptToken(parser, TokenType_Payable)) {
                bool isAddress = stringMatch(getTokenString(parser->tokens, identifier), LIT_TO_STR("address"));
                if(!isAddress) {
                    reportError(parser, "Only address types can be payable");
                }
                node->baseTypeNode.payable = 1;
            }
        } else {
            parser->current -= 1;
            parseIdentifierPath(parser, node);
        }
    } else {
        return false;
    }
    node->endToken = parser->current - 1;

    while(acceptToken(parser, TokenType_LBracket)) {
        ASTNode *copy = allocateNode(parser);
        *copy = *node;

        node->startToken = startToken;
        node->type = ASTNodeType_ArrayType;
        node->arrayTypeNode.elementType = copy;
        node->arrayTypeNode.lengthExpression = 0x0;
        if(acceptToken(parser, TokenType_Colon)) {
            return false;
        }

        if(!acceptToken(parser, TokenType_RBracket)) {
            node->arrayTypeNode.lengthExpression = allocateNode(parser);
            parseExpression(parser, node->arrayTypeNode.lengthExpression);
            if(!acceptToken(parser, TokenType_RBracket)) {
                return false;
            }
        }
        node->endToken = parser->current - 1;
    }

    return true;
}

static void
parsePragma(Parser *parser, ASTNode *node) {
    node->startToken = parser->current - 1;
    node->type = ASTNodeType_Pragma;
    ASTNodePragma *pragma = &node->pragmaNode;

    pragma->major = parseIdentifier(parser);
    assertError(pragma->major != INVALID_TOKEN_ID, parser,
                "Expected identifier after pragma, received (%S)", tokenTypeToString(peekTokenType(parser)));

    while(!acceptToken(parser, TokenType_Semicolon)) {
        if(acceptToken(parser, TokenType_EOF)) {
            reportError(parser, "Unexpected EOF while parsing pragma");
        }

        TokenId part = parser->current++;
        listPushTokenId(&pragma->following, part, parser->arena);
    }
    assertError(pragma->following.count > 0, parser, "Expected pragma body");

    node->endToken = parser->current - 1;
}

static void
parseImport(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_Import;
    node->startToken = parser->current - 1;
    ASTNodeImport *import = &node->importNode;

    if(acceptToken(parser, TokenType_StringLit)) {
        import->pathTokenId = peekLastTokenId(parser);

        import->unitAliasTokenId = INVALID_TOKEN_ID;
        if(acceptToken(parser, TokenType_As)) {
            TokenId unitAliasTokenId = parseIdentifier(parser);
            assertError(unitAliasTokenId != INVALID_TOKEN_ID, parser,
                        "Expected identifier in import alias, received (%S)", tokenTypeToString(peekTokenType(parser)));
            import->unitAliasTokenId = unitAliasTokenId;
        }
    } else if(acceptToken(parser, TokenType_Star)) {
        expectToken(parser, TokenType_As);
        TokenId unitAliasTokenId = parseIdentifier(parser);
        assertError(unitAliasTokenId != INVALID_TOKEN_ID, parser,
                    "Expected identifier in import alias, received (%S)", tokenTypeToString(peekTokenType(parser)));
        import->unitAliasTokenId = unitAliasTokenId;
        expectToken(parser, TokenType_From);
        expectToken(parser, TokenType_StringLit);
        import->pathTokenId = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_LBrace)) {
        do {
            TokenId symbolName = parseIdentifier(parser);
            assertError(symbolName != INVALID_TOKEN_ID, parser,
                        "Expected identifier in import symbol list, received (%S)", tokenTypeToString(peekTokenType(parser)));
            listPushTokenId(&import->symbols, symbolName, parser->arena);

            if(acceptToken(parser, TokenType_As)) {
                TokenId symbolAliasName = parseIdentifier(parser);
                assertError(symbolAliasName != INVALID_TOKEN_ID, parser,
                            "Expected identifier in import symbol alias, received (%S)", tokenTypeToString(peekTokenType(parser)));
                listPushTokenId(&import->symbolAliases, symbolAliasName, parser->arena);
            } else {
                listPushTokenId(&import->symbolAliases, INVALID_TOKEN_ID, parser->arena);
            }
        } while(acceptToken(parser, TokenType_Comma));

        expectToken(parser, TokenType_RBrace);
        expectToken(parser, TokenType_From);
        expectToken(parser, TokenType_StringLit);
        import->pathTokenId = peekLastTokenId(parser);
        import->unitAliasTokenId = INVALID_TOKEN_ID;
    } else {
        reportError(parser, "Unexpected token while parsing import - %S",
                    tokenTypeToString(peekTokenType(parser)));
    }


    expectToken(parser, TokenType_Semicolon);
    node->endToken = parser->current - 1;
}

static u16
parseUserDefinableOperator(Parser *parser) {
    if(nextTokenIs(parser, TokenType_Ampersand) |
       nextTokenIs(parser, TokenType_Tylde) |
       nextTokenIs(parser, TokenType_Pipe) |
       nextTokenIs(parser, TokenType_Carrot) |
       nextTokenIs(parser, TokenType_Plus) |
       nextTokenIs(parser, TokenType_Divide) |
       nextTokenIs(parser, TokenType_Percent) |
       nextTokenIs(parser, TokenType_Star) |
       nextTokenIs(parser, TokenType_Minus) |
       nextTokenIs(parser, TokenType_EqualEqual) |
       nextTokenIs(parser, TokenType_RTick) |
       nextTokenIs(parser, TokenType_RightEqual) |
       nextTokenIs(parser, TokenType_LTick) |
       nextTokenIs(parser, TokenType_LeftEqual) |
       nextTokenIs(parser, TokenType_NotEqual)
       ) {
        return advanceToken(parser);
    }

    reportError(parser, "Unexpected token while parsing user definable operators - %S",
                tokenTypeToString(peekTokenType(parser)));
    return 0;
}

static void
parseUsing(Parser *parser, ASTNode *node) {
    node->startToken = parser->current - 1;
    if(acceptToken(parser, TokenType_LParen)) {
        reportError(parser, "Using statement with parenthesis is not supported");
    }

    node->type = ASTNodeType_Using;
    ASTNodeUsing *using = &node->usingNode;

    ASTNode *last = using->firstIdentifier;
    if(acceptToken(parser, TokenType_LBrace)) {
        using->onLibrary = 0;
        do {
            ASTNode *identifier = allocateNode(parser);
            bool isSuccess = parseType(parser, identifier);
            assertError(isSuccess, parser, "Expected type in using statement, received (%S)", tokenTypeToString(peekTokenType(parser)));
            SLL_QUEUE_PUSH(using->firstIdentifier, last, identifier);

            u16 operator = 0;
            if(acceptToken(parser, TokenType_As)) {
                operator = parseUserDefinableOperator(parser);
            }
            listPushU16(&using->operators, operator, parser->arena);
        } while(acceptToken(parser, TokenType_Comma));

        expectToken(parser, TokenType_RBrace);
    } else {
        using->onLibrary = 1;
        ASTNode *identifier = allocateNode(parser);
        parseType(parser, identifier);
        SLL_QUEUE_PUSH(using->firstIdentifier, last, identifier);
    }

    expectToken(parser, TokenType_For);

    if(acceptToken(parser, TokenType_Star)) {
        using->forType = 0x0;
    } else {
        using->forType = allocateNode(parser);
        parseType(parser, using->forType);
    }

    using->global = INVALID_TOKEN_ID;
    if(acceptToken(parser, TokenType_Global)) {
        using->global = peekLastTokenId(parser);
    }

    expectToken(parser, TokenType_Semicolon);
    node->endToken = parser->current - 1;
}

static void
parseEnum(Parser *parser, ASTNode *node) {
    node->startToken = parser->current - 1;
    node->type = ASTNodeType_EnumDefinition;

    ASTNodeEnum *enumNode = &node->enumNode;
    TokenId nameTokenId = parseIdentifier(parser);
    assertError(nameTokenId != INVALID_TOKEN_ID, parser,
                "Name of enum must be an identifier, received (%S)", tokenTypeToString(peekTokenType(parser)));
    enumNode->nameTokenId = nameTokenId;
    expectToken(parser, TokenType_LBrace);

    if(!acceptToken(parser, TokenType_RBrace)) {
        do {
            TokenId valueName = parseIdentifier(parser);
            assertError(valueName != INVALID_TOKEN_ID, parser,
                        "Enum value must be an identifier, received (%S)", tokenTypeToString(peekTokenType(parser)));
            listPushTokenId(&enumNode->values, valueName, parser->arena);
        } while(acceptToken(parser, TokenType_Comma));

        expectToken(parser, TokenType_RBrace);
    }

    node->endToken = parser->current - 1;
}

static void
parseStruct(Parser *parser, ASTNode *baseNode) {
    baseNode->startToken = parser->current - 1;
    baseNode->type = ASTNodeType_Struct;
    ASTNodeStruct *node = &baseNode->structNode;

    TokenId nameTokenId = parseIdentifier(parser);
    assertError(nameTokenId != INVALID_TOKEN_ID, parser,
                "Name of struct must be an identifier, received (%S)", tokenTypeToString(peekTokenType(parser)));
    node->nameTokenId = nameTokenId;
    expectToken(parser, TokenType_LBrace);

    ASTNode *lastMember = 0x0;
    while(!acceptToken(parser, TokenType_RBrace)) {
        parseVariableDeclarationIntoList(parser, &node->firstMember, &lastMember);
        expectToken(parser, TokenType_Semicolon);
    }

    baseNode->endToken = parser->current - 1;
}

static void
parseError(Parser *parser, ASTNode *node) {
    node->startToken = parser->current - 1;
    node->type = ASTNodeType_Error;
    ASTNodeError *error = &node->errorNode;
    error->identifier = parseIdentifier(parser);
    assertError(error->identifier != INVALID_TOKEN_ID, parser,
                "Name of error must be an identifier, received (%S)", tokenTypeToString(peekTokenType(parser)));

    expectToken(parser, TokenType_LParen);
    ASTNode *lastParameter = 0x0;
    if(!acceptToken(parser, TokenType_RParen)) {
        parseFunctionParameters(parser, &error->firstParameter, &lastParameter);
        expectToken(parser, TokenType_RParen);
    }
    expectToken(parser, TokenType_Semicolon);
    node->endToken = parser->current - 1;
}

static void
parseEvent(Parser *parser, ASTNode *node) {
    node->startToken = parser->current - 1;
    node->type = ASTNodeType_Event;
    ASTNodeEvent *event = &node->eventNode;
    event->identifier = parseIdentifier(parser);
    assertError(event->identifier != INVALID_TOKEN_ID, parser,
                "Name of event must be an identifier, received (%S)", tokenTypeToString(peekTokenType(parser)));

    expectToken(parser, TokenType_LParen);

    ASTNode *lastParameter = 0x0;
    if(!acceptToken(parser, TokenType_RParen)) {
        parseFunctionParameters(parser, &event->firstParameter, &lastParameter);
        expectToken(parser, TokenType_RParen);
    }

    if(acceptToken(parser, TokenType_Anonymous)) {
        event->anonymous = 1;
    }

    expectToken(parser, TokenType_Semicolon);
    node->endToken = parser->current - 1;
}

static void
parseTypedef(Parser *parser, ASTNode *node) {
    node->startToken = parser->current - 1;
    node->type = ASTNodeType_Typedef;
    ASTNodeTypedef *typedefNode = &node->typedefNode;

    typedefNode->identifier = parseIdentifier(parser);
    assertError(typedefNode->identifier != INVALID_TOKEN_ID, parser,
                "Name of typedef must be an identifier, received (%S)", tokenTypeToString(peekTokenType(parser)));

    expectToken(parser, TokenType_Is);

    typedefNode->type = allocateNode(parser);
    parseType(parser, typedefNode->type);
    assertError(typedefNode->type->type == ASTNodeType_BaseType, parser,
                "Typedef type must be a base type");
    expectToken(parser, TokenType_Semicolon);
    node->endToken = parser->current - 1;
}

static bool
isOperator(TokenType type) {
    switch(type) {
        case TokenType_PlusPlus:
        case TokenType_MinusMinus:
        case TokenType_Dot:
        case TokenType_LParen:
        case TokenType_LBrace:
        case TokenType_LBracket:
        case TokenType_StarStar:
        case TokenType_Star:
        case TokenType_Divide:
        case TokenType_Percent:
        case TokenType_Plus:
        case TokenType_Minus:
        case TokenType_Ampersand:
        case TokenType_Carrot:
        case TokenType_Pipe:
        case TokenType_LeftShift:
        case TokenType_RightShift:
        case TokenType_RightShiftZero:
        case TokenType_LeftEqual:
        case TokenType_RightEqual:
        case TokenType_LTick:
        case TokenType_RTick:
        case TokenType_EqualEqual:
        case TokenType_NotEqual:
        case TokenType_LogicalAnd:
        case TokenType_LogicalOr:
        case TokenType_Equal:
        case TokenType_OrEqual:
        case TokenType_XorEqual:
        case TokenType_AndEqual:
        case TokenType_LeftShiftEqual:
        case TokenType_RightShiftEqual:
        case TokenType_PlusEqual:
        case TokenType_MinusEqual:
        case TokenType_StarEqual:
        case TokenType_DivideEqual:
        case TokenType_PercentEqual:
        case TokenType_QuestionMark: return true;
        default: return false;
    }
}

static u32
getOperatorPrecedence(Parser *parser, TokenType type) {
    switch(type) {
        case TokenType_PlusPlus:
        case TokenType_MinusMinus:
        case TokenType_Dot:
        case TokenType_LParen:
        case TokenType_LBrace:
        case TokenType_LBracket: return -1;
        case TokenType_StarStar: return -3;
        case TokenType_Star:
        case TokenType_Divide:
        case TokenType_Percent: return -4;
        case TokenType_Plus:
        case TokenType_Minus: return -5;
        case TokenType_LeftShift:
        case TokenType_RightShift:
        case TokenType_RightShiftZero: return -6;
        case TokenType_Ampersand: return -7;
        case TokenType_Carrot: return -8;
        case TokenType_Pipe: return -9;
        case TokenType_LeftEqual:
        case TokenType_RightEqual:
        case TokenType_LTick:
        case TokenType_RTick: return -10;
        case TokenType_EqualEqual:
        case TokenType_NotEqual: return -11;
        case TokenType_LogicalAnd: return -12;
        case TokenType_LogicalOr: return -13;
        case TokenType_QuestionMark: return -14;
        case TokenType_Equal:
        case TokenType_OrEqual:
        case TokenType_XorEqual:
        case TokenType_AndEqual:
        case TokenType_LeftShiftEqual:
        case TokenType_RightShiftEqual:
        case TokenType_PlusEqual:
        case TokenType_MinusEqual:
        case TokenType_StarEqual:
        case TokenType_DivideEqual:
        case TokenType_PercentEqual: return -15;
        default: reportError(parser, "Found token is not an operator - %S", tokenTypeToString(type));
    }

    return 0;
}

static bool
isUnaryOperator(TokenType type) {
    switch(type) {
        case TokenType_New:
        case TokenType_Exclamation:
        case TokenType_Minus:
        case TokenType_Plus:
        case TokenType_Tylde:
        case TokenType_PlusPlus:
        case TokenType_MinusMinus:
        case TokenType_Delete: return true;
        default: return false;
    }
}

static u32
getUnaryOperatorPrecedence(Parser *parser, TokenType type) {
    switch(type) {
        case TokenType_New: return -1;
        case TokenType_Exclamation:
        case TokenType_Minus:
        case TokenType_Plus:
        case TokenType_Tylde:
        case TokenType_PlusPlus:
        case TokenType_MinusMinus:
        case TokenType_Delete: return -2;
        default: reportError(parser, "Found token is not an unary operator - %S", tokenTypeToString(type));
    }

    return 0;
}

static bool parseExpressionImpl(Parser *parser, ASTNode *node, u32 previousPrecedence);

static void
parseCallArgumentList(Parser *parser, ASTNode **firstExpression, TokenIdList *names) {
    ASTNode *lastExpression = 0x0;
    if(!acceptToken(parser, TokenType_RParen)) {
        if(acceptToken(parser, TokenType_LBrace)) {
            do {
                TokenId identifier = parseIdentifier(parser);
                assertError(identifier != INVALID_TOKEN_ID, parser,
                            "Expected identifier in call argument list, received (%S)", tokenTypeToString(peekTokenType(parser)));
                listPushTokenId(names, identifier, parser->arena);

                expectToken(parser, TokenType_Colon);

                ASTNode *expression = allocateNode(parser);
                parseExpressionImpl(parser, expression, 0);

                SLL_QUEUE_PUSH(*firstExpression, lastExpression, expression);
            } while(acceptToken(parser, TokenType_Comma));
            expectToken(parser, TokenType_RBrace);
        } else {
            do {
                ASTNode *argument = allocateNode(parser);
                parseExpressionImpl(parser, argument, 0);

                SLL_QUEUE_PUSH(*firstExpression, lastExpression, argument);
            } while(acceptToken(parser, TokenType_Comma));
        }

        expectToken(parser, TokenType_RParen);
    }
}

static ASTNode *
parseFunctionCallExpression(Parser *parser, ASTNode *node) {
    ASTNode *expression = allocateNode(parser);
    *expression = *node;
    expression->endToken = parser->current - 2;

    memset(node, 0, sizeof(ASTNode));
    node->startToken = expression->startToken;
    node->type = ASTNodeType_FunctionCallExpression;
    ASTNodeFunctionCallExpression *functionCall = &node->functionCallExpressionNode;

    functionCall->expression = expression;
    parseCallArgumentList(parser, &functionCall->firstArgumentExpression, &functionCall->argumentsName);

    return node;
}

static bool
parseExpressionImpl(Parser *parser, ASTNode *node, u32 previousPrecedence) {
    u32 startPosition = getCurrentParserPosition(parser);
    node->startToken = parser->current;

    if(acceptToken(parser, TokenType_HexNumberLit)) {
        node->type = ASTNodeType_NumberLitExpression;
        node->numberLitExpressionNode.value = peekLastTokenId(parser);
        node->numberLitExpressionNode.subdenomination = INVALID_TOKEN_ID;
    } else if(acceptToken(parser, TokenType_NumberLit)) {
        node->type = ASTNodeType_NumberLitExpression;
        node->numberLitExpressionNode.value = peekLastTokenId(parser);
        node->numberLitExpressionNode.subdenomination = parseSubdenomination(parser);
    } else if(acceptToken(parser, TokenType_StringLit)) {
        node->type = ASTNodeType_StringLitExpression;
        do {
            TokenId literal = peekLastTokenId(parser);
            listPushTokenId(&node->stringLitExpressionNode.values, literal, parser->arena);
        } while(acceptToken(parser, TokenType_StringLit));
    } else if(acceptToken(parser, TokenType_HexStringLit)) {
        node->type = ASTNodeType_HexStringLitExpression;
        do {
            TokenId literal = peekLastTokenId(parser);
            listPushTokenId(&node->stringLitExpressionNode.values, literal, parser->arena);
        } while(acceptToken(parser, TokenType_HexStringLit));
    } else if(acceptToken(parser, TokenType_UnicodeStringLit)) {
        node->type = ASTNodeType_UnicodeStringLitExpression;
        do {
            TokenId literal = peekLastTokenId(parser);
            listPushTokenId(&node->stringLitExpressionNode.values, literal, parser->arena);
        } while(acceptToken(parser, TokenType_UnicodeStringLit));
    } else if(acceptToken(parser, TokenType_True) || acceptToken(parser, TokenType_False)) {
        node->type = ASTNodeType_BoolLitExpression;
        node->boolLitExpressionNode.value = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_LParen)) {
        node->type = ASTNodeType_TupleExpression;

        ASTNode *lastElement = 0x0;
        if(!acceptToken(parser, TokenType_RParen)) {
            do {
                ASTNode *element = allocateNode(parser);
                element->type = ASTNodeType_None;

                if(!nextTokenIs(parser, TokenType_RParen) && !nextTokenIs(parser, TokenType_Comma)) {
                    parseExpressionImpl(parser, element, 0);
                }
                SLL_QUEUE_PUSH(node->tupleExpressionNode.firstElement, lastElement, element);
            } while (acceptToken(parser, TokenType_Comma));

            expectToken(parser, TokenType_RParen);
        }
    } else if(acceptToken(parser, TokenType_LBracket)) {
        node->type = ASTNodeType_InlineArrayExpression;
        ASTNodeInlineArrayExpression *array = &node->inlineArrayExpressionNode;

        ASTNode *lastExpression = 0x0;
        do {
            ASTNode *element = allocateNode(parser);
            parseExpressionImpl(parser, element, 0);
            SLL_QUEUE_PUSH(array->firstExpression, lastExpression, element);
        } while(acceptToken(parser, TokenType_Comma));

        expectToken(parser, TokenType_RBracket);
    } else if(isUnaryOperator(peekTokenType(parser))) {
        u32 operator = peekTokenType(parser);

        if(operator == TokenType_New) {
            node->type = ASTNodeType_NewExpression;
            node->newExpressionNode.typeName = allocateNode(parser);
            advanceToken(parser);

            parseType(parser, node->newExpressionNode.typeName);
        } else {
            node->type = ASTNodeType_UnaryExpression;
            node->unaryExpressionNode.operator = operator;
            node->unaryExpressionNode.subExpression = allocateNode(parser);
            advanceToken(parser);

            u32 precedence = getUnaryOperatorPrecedence(parser, node->unaryExpressionNode.operator);
            parseExpressionImpl(parser, node->unaryExpressionNode.subExpression, precedence);
        }
    } else if(parseIdentifier(parser) != INVALID_TOKEN_ID) {
        String identString = peekLastTokenString(parser);

        if(isBaseTypeName(identString)) {
            u32 startPosition = getCurrentParserPosition(parser);
            parser->current -= 1;
            if(!parseType(parser, node)) {
                setCurrentParserPosition(parser, startPosition);
                u32 startPosition = getCurrentParserPosition(parser);
                node->type = ASTNodeType_BaseType;
                node->baseTypeNode.typeName = peekLastTokenId(parser);

                node->baseTypeNode.payable = 0;
                if(acceptToken(parser, TokenType_Payable)) {
                    assertError(stringMatch(getTokenString(parser->tokens, node->baseTypeNode.typeName), LIT_TO_STR("address")),
                                parser, "Only address types can be payable");
                    node->baseTypeNode.payable = 1;
                }
            }
        } else {
            node->type = ASTNodeType_IdentifierExpression;
            node->identifierExpressionNode.value = peekLastTokenId(parser);
        }
    } else if(acceptToken(parser, TokenType_Type)) {
        node->type = ASTNodeType_IdentifierExpression;
        node->identifierExpressionNode.value = peekLastTokenId(parser);

        expectToken(parser, TokenType_LParen);
        parseFunctionCallExpression(parser, node);
    } else {
        reportError(parser, "Unexpected token while parsing expression - %S",
                    tokenTypeToString(peekTokenType(parser)));
    }

    while(true) {
        startPosition = getCurrentParserPosition(parser);
        TokenType type = peekTokenType(parser);
        if(!isOperator(type)) {
            break;
        }

        u32 precedence = getOperatorPrecedence(parser, type);
        if(precedence <= previousPrecedence) {
            break;
        }
        advanceToken(parser);

        if(type == TokenType_LParen) {
            parseFunctionCallExpression(parser, node);
            continue;
        } else if(type == TokenType_LBracket) {
            ASTNode *expression = allocateNode(parser);
            *expression = *node;
            expression->endToken = parser->current - 2;

            ASTNode *firstExpression = 0x0;
            ASTNode *secondExpression = 0x0;

            ASTNodeType type = ASTNodeType_ArrayAccessExpression;
            if(!acceptToken(parser, TokenType_RBracket)) {
                if(!nextTokenIs(parser, TokenType_Colon)) {
                    firstExpression = allocateNode(parser);
                    parseExpressionImpl(parser, firstExpression, 0);
                }

                if(acceptToken(parser, TokenType_Colon)) {
                    type = ASTNodeType_ArraySliceExpression;
                    if(!acceptToken(parser, TokenType_RBracket)) {
                        secondExpression = allocateNode(parser);
                        parseExpressionImpl(parser, secondExpression, 0);
                        expectToken(parser, TokenType_RBracket);
                    }
                } else {
                    expectToken(parser, TokenType_RBracket);
                }
            }

            node->type = type;
            if(type == ASTNodeType_ArrayAccessExpression) {
                node->arrayAccessExpressionNode.expression = expression;
                node->arrayAccessExpressionNode.indexExpression = firstExpression;
            } else {
                node->arraySliceExpressionNode.expression = expression;
                node->arraySliceExpressionNode.leftFenceExpression = firstExpression;
                node->arraySliceExpressionNode.rightFenceExpression = secondExpression;
            }
            continue;
        } else if(type == TokenType_LBrace) {
            ASTNode *expression = allocateNode(parser);
            *expression = *node;
            expression->endToken = parser->current - 2;

            memset(node, 0, sizeof(ASTNode));
            node->startToken = expression->startToken;
            node->type = ASTNodeType_NamedParameterExpression;
            node->namedParametersExpressionNode.expression = expression;

            node->namedParametersExpressionNode.listStartToken = parser->current;
            ASTNode *lastExpression = 0x0;
            do {
                TokenId identifier = parseIdentifier(parser);

                if(identifier == INVALID_TOKEN_ID) {
                    setCurrentParserPosition(parser, startPosition);
                    *node = *expression;
                    return false;
                }

                assertError(identifier != INVALID_TOKEN_ID, parser,
                            "Expected identifier in named parameter list, received (%S)", tokenTypeToString(peekTokenType(parser)));
                listPushTokenId(&node->namedParametersExpressionNode.names, identifier, parser->arena);

                if(!acceptToken(parser, TokenType_Colon)) {
                    setCurrentParserPosition(parser, startPosition);
                    *node = *expression;
                    return false;
                }

                ASTNode *expression = allocateNode(parser);
                parseExpressionImpl(parser, expression, 0);

                SLL_QUEUE_PUSH(node->namedParametersExpressionNode.firstExpression, lastExpression, expression);
            } while(acceptToken(parser, TokenType_Comma));

            node->namedParametersExpressionNode.listEndToken = parser->current - 1;

            expectToken(parser, TokenType_RBrace);
            continue;
        } else if(type == TokenType_Dot) {
            ASTNode *expression = allocateNode(parser);
            *expression = *node;
            expression->endToken = parser->current - 2;

            node->type = ASTNodeType_MemberAccessExpression;
            node->memberAccessExpressionNode.expression = expression;
            node->memberAccessExpressionNode.memberName = parseIdentifier(parser);
            continue;
        } else if(type == TokenType_QuestionMark) {
            ASTNode *expression = allocateNode(parser);
            *expression = *node;
            expression->endToken = parser->current - 2;

            node->type = ASTNodeType_TerneryExpression;
            node->terneryExpressionNode.condition = expression;
            node->terneryExpressionNode.trueExpression = allocateNode(parser);
            node->terneryExpressionNode.falseExpression = allocateNode(parser);

            parseExpressionImpl(parser, node->terneryExpressionNode.trueExpression, 0);
            expectToken(parser, TokenType_Colon);
            parseExpressionImpl(parser, node->terneryExpressionNode.falseExpression, 0);
            continue;
        } else if(type == TokenType_PlusPlus | type == TokenType_MinusMinus) {
            ASTNode *expression = allocateNode(parser);
            *expression = *node;
            expression->endToken = parser->current - 2;

            node->type = ASTNodeType_UnaryExpressionPostfix;
            node->unaryExpressionNode.operator = type;
            node->unaryExpressionNode.subExpression = expression;
            continue;
        }

        ASTNode *left = allocateNode(parser);
        *left = *node;
        left->endToken = parser->current - 2;

        node->type = ASTNodeType_BinaryExpression;
        node->startToken = left->startToken;
        node->binaryExpressionNode.left = left;
        node->binaryExpressionNode.right = allocateNode(parser);
        node->binaryExpressionNode.operator = type;
        node->endToken = parser->current - 1;

        // Exponentiation has right associativity
        precedence -= type == TokenType_StarStar;

        if(!parseExpressionImpl(parser, node->binaryExpressionNode.right, precedence)) {
            break;
        }

        node->endToken = parser->current - 1;
    }

    node->endToken = parser->current - 1;
    return true;
}

static bool
parseExpression(Parser *parser, ASTNode *node) {
    return parseExpressionImpl(parser, node, 0);
}

static bool
tryParseVariableDeclaration(Parser *parser, ASTNode *node) {
    u32 startPosition = getCurrentParserPosition(parser);
    if(!parseType(parser, node)) {
        setCurrentParserPosition(parser, startPosition);
        return false;
    }

    TokenId dataLocation = INVALID_TOKEN_ID;
    if(acceptToken(parser, TokenType_Memory)) {
        dataLocation = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_Storage)) {
        dataLocation = peekLastTokenId(parser);
    } else if(acceptToken(parser, TokenType_Calldata)) {
        dataLocation = peekLastTokenId(parser);
    }

    TokenId name = parseIdentifier(parser);
    if(name == INVALID_TOKEN_ID) {
        setCurrentParserPosition(parser, startPosition);
        return false;
    }

    ASTNode *type = allocateNode(parser);
    *type = *node;

    node->type = ASTNodeType_VariableDeclaration;
    ASTNodeVariableDeclaration *decl = &node->variableDeclarationNode;
    decl->type = type;
    decl->name = name;
    decl->dataLocation = dataLocation;

    return true;
}

static bool
tryParseVariableDeclarationTuple(Parser *parser, ASTNode *node) {
    if(!nextTokenIs(parser, TokenType_LParen)) {
        return false;
    }

    u32 startPosition = getCurrentParserPosition(parser);
    expectToken(parser, TokenType_LParen);

    ASTNodeVariableDeclarationTupleStatement *tuple = &node->variableDeclarationTupleStatementNode;
    if(!acceptToken(parser, TokenType_RParen)) {
        ASTNode *lastDeclaration = 0x0;
        do {
            ASTNode *declaration = allocateNode(parser);

            declaration->startToken = parser->current;
            if(!nextTokenIs(parser, TokenType_Comma) && !nextTokenIs(parser, TokenType_RParen)) {
                if(!tryParseVariableDeclaration(parser, declaration)) {
                    setCurrentParserPosition(parser, startPosition);
                    return false;
                }
            }

            declaration->endToken = parser->current - 1;
            SLL_QUEUE_PUSH(tuple->firstDeclaration, lastDeclaration, declaration);
        } while(acceptToken(parser, TokenType_Comma));

        expectToken(parser, TokenType_RParen);
    }

    expectToken(parser, TokenType_Equal);

    tuple->initialValue = allocateNode(parser);
    parseExpression(parser, tuple->initialValue);

    node->type = ASTNodeType_VariableDeclarationTupleStatement;

    return true;
}

static void
parseYulExpression(Parser *parser, ASTNode *node, YulLexer *lexer) {
    node->startToken = lexer->currentPosition;

    if(acceptYulToken(lexer, YulTokenType_NumberLit)) {
        node->type = ASTNodeType_YulNumberLitExpression;
        node->yulNumberLitExpressionNode.value = peekYulLastToken(lexer);
    } else if(acceptYulToken(lexer, YulTokenType_StringLit)) {
        node->type = ASTNodeType_YulStringLitExpression;
        node->yulStringLitExpressionNode.value = peekYulLastToken(lexer);
    } else if(acceptYulToken(lexer, YulTokenType_HexNumberLit)) {
        node->type = ASTNodeType_YulHexNumberLitExpression;
        node->yulHexNumberLitExpressionNode.value = peekYulLastToken(lexer);
    } else if(acceptYulToken(lexer, YulTokenType_BoolLit)) {
        node->type = ASTNodeType_YulBoolLitExpression;
        node->yulBoolLitExpressionNode.value = peekYulLastToken(lexer);
    } else if(acceptYulToken(lexer, YulTokenType_HexStringLit)) {
        node->type = ASTNodeType_YulHexStringLitExpression;
        node->yulHexStringLitExpressionNode.value = peekYulLastToken(lexer);
    } else if(acceptYulToken(lexer, YulTokenType_Identifier)) {
        TokenId identifier = peekYulLastToken(lexer);

        node->type = ASTNodeType_YulMemberAccessExpression;
        node->yulIdentifierPathExpressionNode.identifiers[0] = identifier;
        node->yulIdentifierPathExpressionNode.count = 1;

        if(acceptYulToken(lexer, YulTokenType_Dot)) {
            node->yulIdentifierPathExpressionNode.identifiers[1] = parseYulIdentifier(lexer);
            node->yulIdentifierPathExpressionNode.count = 2;
        } else if(acceptYulToken(lexer, YulTokenType_LParen)) {
            node->type = ASTNodeType_YulFunctionCallExpression;
            ASTNodeYulFunctionCallExpression *functionCall = &node->yulFunctionCallExpressionNode;
            functionCall->identifier = identifier;

            ASTNode *lastArgument = 0x0;
            if(!acceptYulToken(lexer, YulTokenType_RParen)) {
                do {
                    ASTNode *argument = allocateNode(parser);
                    parseYulExpression(parser, argument, lexer);
                    SLL_QUEUE_PUSH(functionCall->firstArgument, lastArgument, argument);
                } while(acceptYulToken(lexer, YulTokenType_Comma));
                expectYulToken(parser, lexer, YulTokenType_RParen);
            }
        }
    } else {
        reportError(parser, "Unexpected token while parsing Yul expression - %S", peekYulToken(lexer).string);
    }

    node->endToken = lexer->currentPosition - 1;
}

static void
parseYulStatement(Parser *parser, ASTNode *node, YulLexer *lexer) {
    node->startToken = lexer->currentPosition;

    if(acceptYulToken(lexer, YulTokenType_LBrace)) {
        node->type = ASTNodeType_YulBlockStatement;

        ASTNode *lastStatement = 0x0;
        while(!acceptYulToken(lexer, YulTokenType_RBrace)) {
            ASTNode *statement = allocateNode(parser);
            parseYulStatement(parser, statement, lexer);

            SLL_QUEUE_PUSH(node->blockStatementNode.firstStatement, lastStatement, statement);
        }
    } else if(acceptYulToken(lexer, YulTokenType_Let)) {
        node->type = ASTNodeType_YulVariableDeclaration;
        ASTNodeYulVariableDeclaration *declaration = &node->yulVariableDeclarationNode;

        declaration->identifiers.count = 0;
        listPushTokenId(&declaration->identifiers, parseYulIdentifier(lexer), parser->arena);
        while(acceptYulToken(lexer, YulTokenType_Comma)) {
            listPushTokenId(&declaration->identifiers, parseYulIdentifier(lexer), parser->arena);
        }
        if(acceptYulToken(lexer, YulTokenType_ColonEqual)) {
            declaration->value = allocateNode(parser);
            parseYulExpression(parser, declaration->value, lexer);
        }
    } else if(acceptYulToken(lexer, YulTokenType_Identifier)) {
        TokenId identifier = peekYulLastToken(lexer);

        if(acceptYulToken(lexer, YulTokenType_LParen)) {
            node->type = ASTNodeType_YulFunctionCallExpression;
            ASTNodeYulFunctionCallExpression *functionCall = &node->yulFunctionCallExpressionNode;
            functionCall->identifier = identifier;

            if(!acceptYulToken(lexer, YulTokenType_RParen)) {
                ASTNode *lastArgument = 0x0;
                do {
                    ASTNode *argument = allocateNode(parser);
                    parseYulExpression(parser, argument, lexer);
                    SLL_QUEUE_PUSH(functionCall->firstArgument, lastArgument, argument);
                } while(acceptYulToken(lexer, YulTokenType_Comma));
                expectYulToken(parser, lexer, YulTokenType_RParen);
            }
        } else {
            node->type = ASTNodeType_YulVariableAssignment;
            ASTNodeYulVariableAssignment *assignment = &node->yulVariableAssignmentNode;

            ASTNode *path = allocateNode(parser);
            path->type = ASTNodeType_YulMemberAccessExpression;
            path->startToken = lexer->currentPosition - 1;
            path->yulIdentifierPathExpressionNode.count = 1;
            path->yulIdentifierPathExpressionNode.identifiers[0] = identifier;
            if(acceptYulToken(lexer, YulTokenType_Dot)) {
                path->yulIdentifierPathExpressionNode.count++;
                path->yulIdentifierPathExpressionNode.identifiers[1] = parseYulIdentifier(lexer);
            }
            path->endToken = lexer->currentPosition - 1;

            ASTNode *lastPath = 0x0;
            SLL_QUEUE_PUSH(assignment->firstPath, lastPath, path);

            while(acceptYulToken(lexer, YulTokenType_Comma)) {
                ASTNode *path = allocateNode(parser);
                path->type = ASTNodeType_YulMemberAccessExpression;
                path->startToken = lexer->currentPosition;
                path->yulIdentifierPathExpressionNode.count = 1;
                path->yulIdentifierPathExpressionNode.identifiers[0] = parseYulIdentifier(lexer);
                if(acceptYulToken(lexer, YulTokenType_Dot)) {
                    path->yulIdentifierPathExpressionNode.count++;
                    path->yulIdentifierPathExpressionNode.identifiers[1] = parseYulIdentifier(lexer);
                }
                path->endToken = lexer->currentPosition - 1;
                SLL_QUEUE_PUSH(assignment->firstPath, lastPath, path);
            }
            if(acceptYulToken(lexer, YulTokenType_ColonEqual)) {
                assignment->value = allocateNode(parser);
                parseYulExpression(parser, assignment->value, lexer);
            }
        }
    } else if(acceptYulToken(lexer, YulTokenType_If)) {
        node->type = ASTNodeType_YulIfStatement;
        ASTNodeYulIfStatement *ifStatement = &node->yulIfStatementNode;

        ifStatement->expression = allocateNode(parser);
        parseYulExpression(parser, ifStatement->expression, lexer);
        ifStatement->body = allocateNode(parser);
        parseYulStatement(parser, ifStatement->body, lexer);
    } else if(acceptYulToken(lexer, YulTokenType_For)) {
        node->type = ASTNodeType_YulForStatement;
        ASTNodeYulForStatement *forStatement = &node->yulForStatementNode;

        forStatement->variableDeclaration = allocateNode(parser);
        forStatement->condition = allocateNode(parser);
        forStatement->increment = allocateNode(parser);
        forStatement->body = allocateNode(parser);

        parseYulStatement(parser, forStatement->variableDeclaration, lexer);
        parseYulExpression(parser, forStatement->condition, lexer);
        parseYulStatement(parser, forStatement->increment, lexer);
        parseYulStatement(parser, forStatement->body, lexer);
    } else if(acceptYulToken(lexer, YulTokenType_Function)) {
        node->type = ASTNodeType_YulFunctionDefinition;
        ASTNodeYulFunctionDefinition *function = &node->yulFunctionDefinitionNode;

        function->identifier = parseYulIdentifier(lexer);
        expectYulToken(parser, lexer, YulTokenType_LParen);
        if(!acceptYulToken(lexer, YulTokenType_RParen)) {
            do {
                TokenId identifier = parseYulIdentifier(lexer);
                assertError(identifier != INVALID_TOKEN_ID, parser,
                            "Expected identifier in Yul function parameter list, received (%S)", peekYulToken(lexer).string);
                listPushTokenId(&function->parameters, identifier, parser->arena);
            } while(acceptYulToken(lexer, YulTokenType_Comma));
            expectYulToken(parser, lexer, YulTokenType_RParen);
        }

        if(acceptYulToken(lexer, YulTokenType_RightArrow)) {
            do {
                TokenId identifier = parseYulIdentifier(lexer);
                assertError(identifier != INVALID_TOKEN_ID, parser,
                            "Expected identifier in Yul function return parameter list, received (%S)", peekYulToken(lexer).string);
                listPushTokenId(&function->returnParameters, identifier, parser->arena);
            } while(acceptYulToken(lexer, YulTokenType_Comma));
        }

        function->body = allocateNode(parser);
        parseYulStatement(parser, function->body, lexer);
    } else if(acceptYulToken(lexer, YulTokenType_Switch)) {
        node->type = ASTNodeType_YulSwitchStatement;
        ASTNodeYulSwitchStatement *switchStatement = &node->yulSwitchStatementNode;

        switchStatement->expression = allocateNode(parser);
        parseYulExpression(parser, switchStatement->expression, lexer);

        ASTNode *lastCase = 0x0;
        while(acceptYulToken(lexer, YulTokenType_Case)) {
            ASTNode *c = allocateNode(parser);
            c->type = ASTNodeType_YulCaseStatement;
            c->startToken = lexer->currentPosition - 1;

            c->yulCaseNode.literal = allocateNode(parser);
            c->yulCaseNode.block = allocateNode(parser);

            parseYulExpression(parser, c->yulCaseNode.literal, lexer);
            assertError(c->yulCaseNode.literal->type == ASTNodeType_YulNumberLitExpression |
                        c->yulCaseNode.literal->type == ASTNodeType_YulStringLitExpression |
                        c->yulCaseNode.literal->type == ASTNodeType_YulHexNumberLitExpression |
                        c->yulCaseNode.literal->type == ASTNodeType_YulBoolLitExpression |
                        c->yulCaseNode.literal->type == ASTNodeType_YulHexStringLitExpression,
                        parser, "Switch case requires a literal");
            parseYulStatement(parser, c->yulCaseNode.block, lexer);

            c->endToken = lexer->currentPosition - 1;
            SLL_QUEUE_PUSH(switchStatement->firstCase, lastCase, c);
        }

        bool defaultRequired = switchStatement->firstCase == 0;
        bool hasDefault = acceptYulToken(lexer, YulTokenType_Default);

        if(hasDefault) {
            ASTNode *defaultBlock = allocateNode(parser);
            switchStatement->defaultBlock = defaultBlock;
            defaultBlock->type = ASTNodeType_YulCaseStatement;
            defaultBlock->startToken = lexer->currentPosition - 1;
            defaultBlock->yulCaseNode.literal = 0x0;
            defaultBlock->yulCaseNode.block = allocateNode(parser);

            parseYulStatement(parser, defaultBlock->yulCaseNode.block, lexer);

            defaultBlock->endToken = lexer->currentPosition - 1;
        }

        if(defaultRequired && !hasDefault) {
            reportError(parser, "Switch statement requires a default case");
        }
    } else if(acceptYulToken(lexer, YulTokenType_Leave)) {
        node->type = ASTNodeType_YulLeaveStatement;
    } else if(acceptYulToken(lexer, YulTokenType_Break)) {
        node->type = ASTNodeType_YulBreakStatement;
    } else if(acceptYulToken(lexer, YulTokenType_Continue)) {
        node->type = ASTNodeType_YulContinueStatement;
    } else {
        reportError(parser, "Unhandeled Yul statement for token - %S", peekYulToken(lexer).string);
    }

    node->endToken = lexer->currentPosition - 1;
}

static void
parseYulBlock(Parser *parser, ASTNode *node) {
    YulLexer lexer = createYulLexer(parser->tokens, parser->tokenCount, parser->current);
    parseYulStatement(parser, node, &lexer);

    parser->current = lexer.currentPosition;
}

static void
parseStatement(Parser *parser, ASTNode *node) {
    if(acceptToken(parser, TokenType_Return)) {
        node->startToken = parser->current - 1;
        ASTNode *returnStatement = node;
        returnStatement->type = ASTNodeType_ReturnStatement;
        returnStatement->returnStatementNode.expression = 0x0;

        if(!acceptToken(parser, TokenType_Semicolon)) {
            returnStatement->returnStatementNode.expression = allocateNode(parser);
            parseExpression(parser, returnStatement->returnStatementNode.expression);
            expectToken(parser, TokenType_Semicolon);
        }
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_If)) {
        node->startToken = parser->current - 1;
        node->type = ASTNodeType_IfStatement;
        ASTNodeIfStatement *ifStatement = &node->ifStatementNode;
        ifStatement->conditionExpression = allocateNode(parser);

        expectToken(parser, TokenType_LParen);
        parseExpression(parser, ifStatement->conditionExpression);
        expectToken(parser, TokenType_RParen);

        ifStatement->trueStatement = allocateNode(parser);
        parseStatement(parser, ifStatement->trueStatement);

        if(acceptToken(parser, TokenType_Else)) {
            ifStatement->falseStatement = allocateNode(parser);
            parseStatement(parser, ifStatement->falseStatement);
        }
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_LBrace)) {
        node->type = ASTNodeType_BlockStatement;
        node->startToken = parser->current - 1;

        ASTNode *lastStatement = 0x0;
        while(!acceptToken(parser, TokenType_RBrace)) {
            ASTNode *statement = allocateNode(parser);
            parseStatement(parser, statement);

            SLL_QUEUE_PUSH(node->blockStatementNode.firstStatement, lastStatement, statement);
        }
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Unchecked)) {
        node->type = ASTNodeType_UncheckedBlockStatement;
        node->startToken = parser->current - 1;
        ASTNodeUncheckedBlockStatement *statement = &node->uncheckedBlockStatementNode;
        statement->block = allocateNode(parser);
        parseStatement(parser, statement->block);
        assertError(statement->block->type == ASTNodeType_BlockStatement, parser, "A block statement must follow the unchecked keyword");
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_While)) {
        node->type = ASTNodeType_WhileStatement;
        node->startToken = parser->current - 1;
        ASTNodeWhileStatement *statement = &node->whileStatementNode;

        expectToken(parser, TokenType_LParen);
        statement->expression = allocateNode(parser);
        parseExpression(parser, statement->expression);
        expectToken(parser, TokenType_RParen);

        statement->body = allocateNode(parser);
        parseStatement(parser, statement->body);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Do)) {
        node->type = ASTNodeType_DoWhileStatement;
        node->startToken = parser->current - 1;
        ASTNodeWhileStatement *statement = &node->doWhileStatementNode;

        statement->body = allocateNode(parser);
        parseStatement(parser, statement->body);

        expectToken(parser, TokenType_While);
        expectToken(parser, TokenType_LParen);
        statement->expression = allocateNode(parser);
        parseExpression(parser, statement->expression);
        expectToken(parser, TokenType_RParen);
        expectToken(parser, TokenType_Semicolon);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_For)) {
        node->type = ASTNodeType_ForStatement;
        node->startToken = parser->current - 1;
        ASTNodeForStatement *statement = &node->forStatementNode;

        expectToken(parser, TokenType_LParen);

        statement->variableStatement = 0x0;
        if(!acceptToken(parser, TokenType_Semicolon)) {
            statement->variableStatement = allocateNode(parser);
            parseStatement(parser, statement->variableStatement);
            assertError(statement->variableStatement->type == ASTNodeType_ExpressionStatement ||
                        statement->variableStatement->type == ASTNodeType_VariableDeclarationStatement ||
                        statement->variableStatement->type == ASTNodeType_VariableDeclarationTupleStatement,
                        parser, "Expected variable declaration or expression statement in for loop initializer");
        }

        statement->conditionExpression = 0x0;
        if(!acceptToken(parser, TokenType_Semicolon)) {
            statement->conditionExpression = allocateNode(parser);
            parseExpression(parser, statement->conditionExpression);
            expectToken(parser, TokenType_Semicolon);
        }

        statement->incrementExpression = 0x0;
        if(!acceptToken(parser, TokenType_RParen)) {
            statement->incrementExpression = allocateNode(parser);
            parseExpression(parser, statement->incrementExpression);
            acceptToken(parser, TokenType_RParen);
        }

        statement->body = allocateNode(parser);
        parseStatement(parser, statement->body);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Revert)) {
        node->type = ASTNodeType_RevertStatement;
        node->startToken = parser->current - 1;
        ASTNodeRevertStatement *statement = &node->revertStatementNode;

        statement->expression = allocateNode(parser);
        parseExpression(parser, statement->expression);
        expectToken(parser, TokenType_Semicolon);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Break)) {
        node->type = ASTNodeType_BreakStatement;
        node->startToken = parser->current - 1;
        expectToken(parser, TokenType_Semicolon);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Continue)) {
        node->type = ASTNodeType_ContinueStatement;
        node->startToken = parser->current - 1;
        expectToken(parser, TokenType_Semicolon);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Emit)) {
        node->type = ASTNodeType_EmitStatement;
        node->startToken = parser->current - 1;
        node->emitStatementNode.expression = allocateNode(parser);
        parseExpression(parser, node->emitStatementNode.expression);
        assertError(node->emitStatementNode.expression->type == ASTNodeType_FunctionCallExpression,
                    parser, "Expected function call expression after emit keyword");
        expectToken(parser, TokenType_Semicolon);
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Try)) {
        node->type = ASTNodeType_TryStatement;
        node->startToken = parser->current - 1;
        ASTNodeTryStatement *statement = &node->tryStatementNode;

        statement->expression = allocateNode(parser);
        parseExpression(parser, statement->expression);

        statement->firstReturnParameter = MISSING_ELEMENT;
        ASTNode *lastReturnParameter = 0x0;
        if(acceptToken(parser, TokenType_Returns)) {
            statement->firstReturnParameter = 0x0;
            expectToken(parser, TokenType_LParen);

            parseFunctionParameters(parser, &statement->firstReturnParameter, &lastReturnParameter);
            expectToken(parser, TokenType_RParen);
        }

        statement->body = allocateNode(parser);
        parseStatement(parser, statement->body);

        ASTNode *lastCatch = 0x0;
        ASTNode *lastParameter = 0x0;
        while(acceptToken(parser, TokenType_Catch)) {
            ASTNode *catch = allocateNode(parser);
            catch->type = ASTNodeType_CatchStatement;
            catch->startToken = parser->current - 1;
            ASTNodeCatchStatement *catchStatement = &catch->catchStatementNode;

            catchStatement->identifier = INVALID_TOKEN_ID;
            catchStatement->firstParameter = MISSING_ELEMENT;

            if(!acceptToken(parser, TokenType_LBrace)) {
                catchStatement->firstParameter = 0x0;
                if(!acceptToken(parser, TokenType_LParen)) {
                    catchStatement->identifier = parseIdentifier(parser);
                    expectToken(parser, TokenType_LParen);
                }

                parseFunctionParameters(parser, &catchStatement->firstParameter, &lastParameter);
                expectToken(parser, TokenType_RParen);
            } else {
                parser->current -= 1;
            }

            catchStatement->body = allocateNode(parser);
            parseStatement(parser, catchStatement->body);
            catch->endToken = parser->current - 1;

            SLL_QUEUE_PUSH(statement->firstCatch, lastCatch, catch);
        }
        node->endToken = parser->current - 1;
    } else if(acceptToken(parser, TokenType_Assembly)) {
        node->type = ASTNodeType_AssemblyStatement;
        node->startToken = parser->current - 1;
        ASTNodeAssemblyStatement *statement = &node->assemblyStatementNode;
        if(acceptToken(parser, TokenType_StringLit)) {
            statement->isEVMAsm = stringMatch(peekLastTokenString(parser), LIT_TO_STR("evmasm"));
            if(!statement->isEVMAsm) {
                reportError(parser, "Expected (\"evmasm\") as string literal, but got (\"%S\")", peekLastTokenString(parser));
            }
        }

        if(acceptToken(parser, TokenType_LParen)) {
            do {
                expectToken(parser, TokenType_StringLit);
                listPushTokenId(&statement->flags, peekLastTokenId(parser), parser->arena);
            } while(acceptToken(parser, TokenType_Comma));
            expectToken(parser, TokenType_RParen);
        }

        statement->body = allocateNode(parser);
        parseYulBlock(parser, statement->body);
        node->endToken = parser->current - 1;
    } else {
        u32 startToken = parser->current;
        if(tryParseVariableDeclaration(parser, node)) {
            ASTNode *varDeclNode = allocateNode(parser);
            node->endToken = parser->current - 1;
            *varDeclNode = *node;

            node->startToken = startToken;
            node->type = ASTNodeType_VariableDeclarationStatement;
            ASTNodeVariableDeclarationStatement *statement = &node->variableDeclarationStatementNode;
            statement->variableDeclaration = varDeclNode;

            statement->initialValue = 0x0;
            if(acceptToken(parser, TokenType_Equal)) {
                statement->initialValue = allocateNode(parser);
                parseExpression(parser, statement->initialValue);
            }

            expectToken(parser, TokenType_Semicolon);
            node->endToken = parser->current - 1;
        } else if(tryParseVariableDeclarationTuple(parser, node)) {
            node->startToken = startToken;
            expectToken(parser, TokenType_Semicolon);
            node->endToken = parser->current - 1;
        } else {
            node->startToken = parser->current;
            node->type = ASTNodeType_ExpressionStatement;
            node->expressionStatementNode.expression = allocateNode(parser);

            parseExpression(parser, node->expressionStatementNode.expression);
            expectToken(parser, TokenType_Semicolon);
            node->endToken = parser->current - 1;
        }
    }
}

static void
parseBlock(Parser *parser, ASTNode *node) {
    return parseStatement(parser, node);
}

static void
parseConstVariable(Parser *parser, ASTNode *node, ASTNode *type) {
    node->type = ASTNodeType_ConstVariable;
    node->startToken = parser->current - 1;
    ASTNodeConstVariable *constVariable = &node->constVariableNode;

    constVariable->type = type;
    constVariable->identifier = parseIdentifier(parser);
    assertError(constVariable->identifier != INVALID_TOKEN_ID,
                parser, "Constant variable declaration name can only be an identifier");

    expectToken(parser, TokenType_Equal);

    constVariable->expression = allocateNode(parser);
    parseExpression(parser, constVariable->expression);

    expectToken(parser, TokenType_Semicolon);
    node->endToken = parser->current - 1;
}

static void
parseOverrideSpecifierArgs(Parser *parser, ASTNode **first) {
    ASTNode *last = 0x0;
    if(acceptToken(parser, TokenType_LParen)) {
        do {
            ASTNode *argument = allocateNode(parser);
            parseType(parser, argument);
            assertError(argument->type == ASTNodeType_IdentifierPath,
                        parser, "Expected identifier path in override specifier arguments");

            SLL_QUEUE_PUSH(*first, last, argument);
        } while(acceptToken(parser, TokenType_Comma));

        expectToken(parser, TokenType_RParen);
    }
}

static bool
tryParseStateVariableDeclaration(Parser *parser, ASTNode *node) {
    u32 startPosition = getCurrentParserPosition(parser);
    node->startToken = parser->current;
    node->type = ASTNodeType_StateVariableDeclaration;
    ASTNodeConstVariable *decl = &node->constVariableNode;

    decl->type = allocateNode(parser);
    if(!parseType(parser, decl->type)) {
        setCurrentParserPosition(parser, startPosition);
        return false;
    }

    decl->visibility = INVALID_TOKEN_ID;
    decl->mutability = INVALID_TOKEN_ID;
    decl->override = INVALID_TOKEN_ID;
    for(;;) {
        if (acceptToken(parser, TokenType_Internal)) {
            assertError(decl->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            decl->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Private)) {
            assertError(decl->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            decl->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Public)) {
            assertError(decl->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            decl->visibility = peekLastTokenId(parser);
        } else if(acceptToken(parser, TokenType_Constant)) {
            assertError(decl->mutability == INVALID_TOKEN_ID, parser, "Mutability modifier already set");
            decl->mutability = peekLastTokenId(parser);
        } else if(acceptToken(parser, TokenType_Immutable)) {
            assertError(decl->mutability == INVALID_TOKEN_ID, parser, "Mutability modifier already set");
            decl->mutability = peekLastTokenId(parser);
        } else if(acceptToken(parser, TokenType_Override)) {
            assertError(decl->override == INVALID_TOKEN_ID, parser, "Override modifier already set");
            decl->override = peekLastTokenId(parser);
            parseOverrideSpecifierArgs(parser, &decl->firstOverride);
        } else {
            break;
        }
    }

    decl->identifier = parseIdentifier(parser);
    if(decl->identifier == INVALID_TOKEN_ID) {
        setCurrentParserPosition(parser, startPosition);
        return false;
    }

    if(acceptToken(parser, TokenType_Equal)) {
        decl->expression = allocateNode(parser);
        parseExpression(parser, decl->expression);
    }

    if(!acceptToken(parser, TokenType_Semicolon)) {
        setCurrentParserPosition(parser, startPosition);
        return false;
    }

    node->endToken = parser->current - 1;

    return true;
}

static void
parseModifierInvocation(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_ModifierInvocation;
    node->startToken = parser->current - 1;
    ASTNodeModifierInvocation *invocation = &node->modifierInvocationNode;

    invocation->identifier = allocateNode(parser);
    parseIdentifierPath(parser, invocation->identifier);

    invocation->firstArgumentExpression = MISSING_ELEMENT;
    if(acceptToken(parser, TokenType_LParen)) {
        invocation->firstArgumentExpression = 0x0;
        parseCallArgumentList(parser, &invocation->firstArgumentExpression, &invocation->argumentsName);
    }
    node->endToken = parser->current - 1;
}

static void
parseFunction(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_FunctionDefinition;
    node->startToken = parser->current - 1;

    TokenId name = parseIdentifier(parser);
    if(name == INVALID_TOKEN_ID) {
        if(acceptToken(parser, TokenType_Fallback) ||
           acceptToken(parser, TokenType_Receive) ||
           peekLastTokenType(parser) == TokenType_Receive ||
           peekLastTokenType(parser) == TokenType_Fallback) {
            name = peekLastTokenId(parser);
        }
    }
    ASTNodeFunctionDefinition *function = &node->functionDefinitionNode;
    function->name = name;

    ASTNode *lastParameter = 0x0;
    expectToken(parser, TokenType_LParen);
    if(!acceptToken(parser, TokenType_RParen)) {
        parseFunctionParameters(parser, &function->firstParameter, &lastParameter);
        expectToken(parser, TokenType_RParen);
    }

    function->visibility = INVALID_TOKEN_ID;
    function->stateMutability = INVALID_TOKEN_ID;
    function->virtual = INVALID_TOKEN_ID;
    function->override = INVALID_TOKEN_ID;
    ASTNode *lastModifier = 0x0;
    for(;;) {
        if (acceptToken(parser, TokenType_Internal)) {
            assertError(function->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            function->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_External)) {
            assertError(function->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            function->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Private)) {
            assertError(function->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            function->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Public)) {
            assertError(function->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            function->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Pure)) {
            assertError(function->stateMutability == INVALID_TOKEN_ID, parser, "State mutability modifier already set");
            function->stateMutability = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_View)) {
            assertError(function->stateMutability == INVALID_TOKEN_ID, parser, "State mutability modifier already set");
            function->stateMutability = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Payable)) {
            assertError(function->stateMutability == INVALID_TOKEN_ID, parser, "State mutability modifier already set");
            function->stateMutability = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Constant)) {
            assertError(function->stateMutability == INVALID_TOKEN_ID, parser, "State mutability modifier already set");
            function->stateMutability = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Virtual)) {
            assertError(function->virtual == INVALID_TOKEN_ID, parser, "Virtual modifier already set");
            function->virtual = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Override)) {
            assertError(function->override == INVALID_TOKEN_ID, parser, "Override modifier already set");
            function->override = peekLastTokenId(parser);
            parseOverrideSpecifierArgs(parser, &function->firstOverride);
        } else {
            if(parseIdentifier(parser) != INVALID_TOKEN_ID) {
                parser->current -= 1;

                ASTNode *modifier = allocateNode(parser);
                parseModifierInvocation(parser, modifier);

                SLL_QUEUE_PUSH(function->firstModifier, lastModifier, modifier);
                continue;
            }

            break;
        }
    }

    ASTNode *lastReturnParameter = 0x0;
    if(acceptToken(parser, TokenType_Returns)) {
        expectToken(parser, TokenType_LParen);
        parseFunctionParameters(parser, &function->firstReturnParameter, &lastReturnParameter);
        expectToken(parser, TokenType_RParen);
    }

    function->body = 0x0;
    if(!acceptToken(parser, TokenType_Semicolon)) {
        function->body = allocateNode(parser);
        parseBlock(parser, function->body);
    }

    node->endToken = parser->current - 1;
}

static void
parseModifier(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_ModifierDefinition;
    node->startToken = parser->current - 1;
    TokenId name = parseIdentifier(parser);
    assertError(name != INVALID_TOKEN_ID, parser, "Modifier name must be an identifier");
    ASTNodeFunctionDefinition *modifier = &node->functionDefinitionNode;
    modifier->name = name;

    modifier->firstParameter = MISSING_ELEMENT;
    ASTNode *lastParameter = 0x0;
    if(acceptToken(parser, TokenType_LParen)) {
        modifier->firstParameter = 0x0;
        if(!acceptToken(parser, TokenType_RParen)) {
            parseFunctionParameters(parser, &modifier->firstParameter, &lastParameter);
            expectToken(parser, TokenType_RParen);
        }
    }

    modifier->stateMutability = INVALID_TOKEN_ID;
    modifier->virtual = INVALID_TOKEN_ID;
    modifier->override = INVALID_TOKEN_ID;
    for(;;) {
        if (acceptToken(parser, TokenType_Virtual)) {
            assertError(modifier->virtual == INVALID_TOKEN_ID, parser, "Virtual modifier already set");
            modifier->virtual = peekLastTokenId(parser);
        } else if(acceptToken(parser, TokenType_Override)) {
            assertError(modifier->override == INVALID_TOKEN_ID, parser, "Override modifier already set");
            modifier->override = peekLastTokenId(parser);
            parseOverrideSpecifierArgs(parser, &modifier->firstOverride);
        } else {
            break;
        }
    }

    modifier->body = 0x0;
    if(!acceptToken(parser, TokenType_Semicolon)) {
        modifier->body = allocateNode(parser);
        parseBlock(parser, modifier->body);
    }

    node->endToken = parser->current - 1;
}

static void
parseConstructor(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_ConstructorDefinition;
    node->startToken = parser->current - 1;
    ASTNodeConstructorDefinition *constructor = &node->constructorDefinitionNode;

    ASTNode *lastParameter = 0x0;
    expectToken(parser, TokenType_LParen);
    if(!acceptToken(parser, TokenType_RParen)) {
        parseFunctionParameters(parser, &constructor->firstParameter, &lastParameter);
        expectToken(parser, TokenType_RParen);
    }

    constructor->stateMutability = INVALID_TOKEN_ID;
    constructor->visibility = INVALID_TOKEN_ID;
    ASTNode *lastModifier = 0x0;
    for(;;) {
        if (acceptToken(parser, TokenType_Payable)) {
            assertError(constructor->stateMutability == INVALID_TOKEN_ID, parser, "State mutability modifier already set");
            constructor->stateMutability = peekLastTokenId(parser);
        } else if(acceptToken(parser, TokenType_Internal)) {
            assertError(constructor->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            constructor->visibility = peekLastTokenId(parser);
        } else if (acceptToken(parser, TokenType_Public)) {
            assertError(constructor->visibility == INVALID_TOKEN_ID, parser, "Visibility modifier already set");
            constructor->visibility = peekLastTokenId(parser);
        } else {
            if(parseIdentifier(parser) != INVALID_TOKEN_ID) {
                parser->current -= 1;

                ASTNode *modifier = allocateNode(parser);
                parseModifierInvocation(parser, modifier);

                SLL_QUEUE_PUSH(constructor->firstModifier, lastModifier, modifier);
                continue;
            }

            break;
        }
    }

    constructor->body = allocateNode(parser);
    parseBlock(parser, constructor->body);

    node->endToken = parser->current - 1;
}

static void
parseContractBody(Parser *parser, ASTNode **firstElement) {
    ASTNode *lastElement = 0x0;
    expectToken(parser, TokenType_LBrace);
    while(!acceptToken(parser, TokenType_RBrace)) {
        ASTNode *element = allocateNode(parser);

        if(acceptToken(parser, TokenType_Constructor)) {
            parseConstructor(parser, element);
        } else if(acceptToken(parser, TokenType_Function)) {
            parser->current -= 1;
            if(!tryParseStateVariableDeclaration(parser, element)) {
                parser->current += 1;
                memset(element, 0, sizeof(ASTNode));

                parseFunction(parser, element);
            }
        } else if(acceptToken(parser, TokenType_Modifier)) {
            parseModifier(parser, element);
        } else if(acceptToken(parser, TokenType_Fallback)) {
            parseFunction(parser, element);
            element->type = ASTNodeType_FallbackFunction;
        } else if(acceptToken(parser, TokenType_Receive)) {
            parseFunction(parser, element);
            element->type = ASTNodeType_ReceiveFunction;
        } else if(acceptToken(parser, TokenType_Struct)) {
            parseStruct(parser, element);
        } else if(acceptToken(parser, TokenType_Using)) {
            parseUsing(parser, element);
        } else if(acceptToken(parser, TokenType_Enum)) {
            parseEnum(parser, element);
        } else if(acceptToken(parser, TokenType_Type)) {
            parseTypedef(parser, element);
        } else if(acceptToken(parser, TokenType_Event)) {
            parseEvent(parser, element);
        } else if(acceptToken(parser, TokenType_Error)) {
            parser->current -= 1;
            if(!tryParseStateVariableDeclaration(parser, element)) {
                parser->current += 1;
                memset(element, 0, sizeof(ASTNode));

                parseError(parser, element);
            }
        } else if(acceptToken(parser, TokenType_EOF)) {
            break;
        } else {
            assertError(tryParseStateVariableDeclaration(parser, element),
                        parser, "Expected state variable declaration");
        }

        SLL_QUEUE_PUSH(*firstElement, lastElement, element);
    }
}

static void
parseContract(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_ContractDefinition;
    node->startToken = parser->current - 1;
    ASTNodeContractDefinition *contract = &node->contractDefinitionNode;

    contract->name = parseIdentifier(parser);

    ASTNode *lastBaseContract = 0x0;
    if(acceptToken(parser, TokenType_Is)) {
        do {
            ASTNode *baseContract = allocateNode(parser);

            baseContract->type = ASTNodeType_InheritanceSpecifier;
            baseContract->startToken = parser->current;
            ASTNodeInheritanceSpecifier *inheritance = &baseContract->inheritanceSpecifierNode;

            inheritance->identifier = allocateNode(parser);
            parseType(parser, inheritance->identifier);
            assertError(inheritance->identifier->type == ASTNodeType_IdentifierPath,
                        parser, "Expected identifier path in inheritance specifier");

            inheritance->firstArgumentExpression = MISSING_ELEMENT;
            if(acceptToken(parser, TokenType_LParen)) {
                inheritance->firstArgumentExpression = 0x0;
                parseCallArgumentList(parser, &inheritance->firstArgumentExpression, &inheritance->argumentsName);
            }

            baseContract->endToken = parser->current - 1;

            SLL_QUEUE_PUSH(contract->firstBaseContract, lastBaseContract, baseContract);
        } while(acceptToken(parser, TokenType_Comma));
    }

    parseContractBody(parser, &contract->firstElement);

    node->endToken = parser->current - 1;
}

static void
parseInterface(Parser *parser, ASTNode *node) {
    u32 startPosition = parser->current - 1;
    parseContract(parser, node);
    u32 endPosition = parser->current - 1;
    node->type = ASTNodeType_InterfaceDefinition;

    node->startToken = startPosition;
    node->endToken = endPosition;
}

static void
parseAbstractContract(Parser *parser, ASTNode *node) {
    u32 startPosition = parser->current - 1;
    expectToken(parser, TokenType_Contract);
    parseContract(parser, node);
    u32 endPosition = parser->current - 1;

    node->type = ASTNodeType_AbstractContractDefinition;
    node->startToken = startPosition;
    node->endToken = endPosition;
}

static void
parseLibrary(Parser *parser, ASTNode *node) {
    node->type = ASTNodeType_LibraryDefinition;
    node->startToken = parser->current - 1;
    ASTNodeLibraryDefinition *library = &node->libraryDefinitionNode;

    library->name = parseIdentifier(parser);

    parseContractBody(parser, &library->firstElement);
    node->endToken = parser->current - 1;
}

typedef struct ASTNodeSizeEntry {
    String name;
    u32 size;
} ASTNodeSizeEntry;

static void
printASTNodeSizes(Arena *arena) {
    static ASTNodeSizeEntry entries[] = {
        { LIT_TO_STR("ASTNode"), sizeof(ASTNode)},
        { LIT_TO_STR("ASTNodeStruct"), sizeof(ASTNodeStruct)},
        { LIT_TO_STR("ASTNodeBaseType"), sizeof(ASTNodeBaseType)},
        { LIT_TO_STR("ASTNodeIdentifierPath"), sizeof(ASTNodeIdentifierPath)},
        { LIT_TO_STR("ASTNodeMapping"), sizeof(ASTNodeMapping)},
        { LIT_TO_STR("ASTNodeFunctionType"), sizeof(ASTNodeFunctionType)},
        { LIT_TO_STR("ASTNodeArrayType"), sizeof(ASTNodeArrayType)},
        { LIT_TO_STR("ASTNodeError"), sizeof(ASTNodeError)},
        { LIT_TO_STR("ASTNodeEvent"), sizeof(ASTNodeEvent)},
        { LIT_TO_STR("ASTNodeTypedef"), sizeof(ASTNodeTypedef)},
        { LIT_TO_STR("ASTNodeConstVariable"), sizeof(ASTNodeConstVariable)},
        { LIT_TO_STR("ASTNodeNumberLitExpression"), sizeof(ASTNodeNumberLitExpression)},
        { LIT_TO_STR("ASTNodeStringLitExpression"), sizeof(ASTNodeStringLitExpression)},
        { LIT_TO_STR("ASTNodeTokenLitExpression"), sizeof(ASTNodeTokenLitExpression)},
        { LIT_TO_STR("ASTNodeBinaryExpression"), sizeof(ASTNodeBinaryExpression)},
        { LIT_TO_STR("ASTNodeTupleExpression"), sizeof(ASTNodeTupleExpression)},
        { LIT_TO_STR("ASTNodeUnaryExpression"), sizeof(ASTNodeUnaryExpression)},
        { LIT_TO_STR("ASTNodeNewExpression"), sizeof(ASTNodeNewExpression)},
        { LIT_TO_STR("ASTNodeFunctionCallExpression"), sizeof(ASTNodeFunctionCallExpression)},
        { LIT_TO_STR("ASTNodeMemberAccessExpression"), sizeof(ASTNodeMemberAccessExpression)},
        { LIT_TO_STR("ASTNodeArrayAccessExpression"), sizeof(ASTNodeArrayAccessExpression)},
        { LIT_TO_STR("ASTNodeArraySliceExpression"), sizeof(ASTNodeArraySliceExpression)},
        { LIT_TO_STR("ASTNodeTerneryExpression"), sizeof(ASTNodeTerneryExpression)},
        { LIT_TO_STR("ASTNodeNamedParametersExpression"), sizeof(ASTNodeNamedParametersExpression)},
        { LIT_TO_STR("ASTNodeInlineArrayExpression"), sizeof(ASTNodeInlineArrayExpression)},
        { LIT_TO_STR("ASTNodeFunctionDefinition"), sizeof(ASTNodeFunctionDefinition)},
        { LIT_TO_STR("ASTNodeBlockStatement"), sizeof(ASTNodeBlockStatement)},
        { LIT_TO_STR("ASTNodeUncheckedBlockStatement"), sizeof(ASTNodeUncheckedBlockStatement)},
        { LIT_TO_STR("ASTNodeReturnStatement"), sizeof(ASTNodeReturnStatement)},
        { LIT_TO_STR("ASTNodeIfStatement"), sizeof(ASTNodeIfStatement)},
        { LIT_TO_STR("ASTNodeVariableDeclarationStatement"), sizeof(ASTNodeVariableDeclarationStatement)},
        { LIT_TO_STR("ASTNodeVariableDeclaration"), sizeof(ASTNodeVariableDeclaration)},
        { LIT_TO_STR("ASTNodeVariableDeclarationTupleStatement"), sizeof(ASTNodeVariableDeclarationTupleStatement)},
        { LIT_TO_STR("ASTNodeWhileStatement"), sizeof(ASTNodeWhileStatement)},
        { LIT_TO_STR("ASTNodeInheritanceSpecifier"), sizeof(ASTNodeInheritanceSpecifier)},
        { LIT_TO_STR("ASTNodeModifierInvocation"), sizeof(ASTNodeModifierInvocation)},
        { LIT_TO_STR("ASTNodeContractDefinition"), sizeof(ASTNodeContractDefinition)},
        { LIT_TO_STR("ASTNodeLibraryDefinition"), sizeof(ASTNodeLibraryDefinition)},
        { LIT_TO_STR("ASTNodeRevertStatement"), sizeof(ASTNodeRevertStatement)},
        { LIT_TO_STR("ASTNodeForStatement"), sizeof(ASTNodeForStatement)},
        { LIT_TO_STR("ASTNodeEmitStatement"), sizeof(ASTNodeEmitStatement)},
        { LIT_TO_STR("ASTNodeTryStatement"), sizeof(ASTNodeTryStatement)},
        { LIT_TO_STR("ASTNodeCatchStatement"), sizeof(ASTNodeCatchStatement)},
        { LIT_TO_STR("ASTNodeAssemblyStatement"), sizeof(ASTNodeAssemblyStatement)},
        { LIT_TO_STR("ASTNodeConstructorDefinition"), sizeof(ASTNodeConstructorDefinition)},
        { LIT_TO_STR("ASTNodeNameValue"), sizeof(ASTNodeNameValue)},
        { LIT_TO_STR("ASTNodeUsing"), sizeof(ASTNodeUsing)},
        { LIT_TO_STR("ASTNodeBlockStatement"), sizeof(ASTNodeBlockStatement)},
        { LIT_TO_STR("ASTNodeYulVariableDeclaration"), sizeof(ASTNodeYulVariableDeclaration)},
        { LIT_TO_STR("ASTNodeYulNumberLitExpression"), sizeof(ASTNodeYulNumberLitExpression)},
        { LIT_TO_STR("ASTNodeYulIdentifierPathExpression"), sizeof(ASTNodeYulIdentifierPathExpression)},
        { LIT_TO_STR("ASTNodeYulFunctionCallExpression"), sizeof(ASTNodeYulFunctionCallExpression)},
        { LIT_TO_STR("ASTNodeYulVariableAssignment"), sizeof(ASTNodeYulVariableAssignment)},
        { LIT_TO_STR("ASTNodeYulIfStatement"), sizeof(ASTNodeYulIfStatement)},
        { LIT_TO_STR("ASTNodeYulForStatement"), sizeof(ASTNodeYulForStatement)},
        { LIT_TO_STR("ASTNodeYulFunctionDefinition"), sizeof(ASTNodeYulFunctionDefinition)},
        { LIT_TO_STR("ASTNodeYulSwitchStatement"), sizeof(ASTNodeYulSwitchStatement)},
    };

    // Bubble sort by size
    for(u32 i = 0; i < ARRAY_LENGTH(entries); i++) {
        for(u32 j = 0; j < ARRAY_LENGTH(entries) - 1; j++) {
            if(entries[j].size < entries[j + 1].size) {
                ASTNodeSizeEntry temp = entries[j];
                entries[j] = entries[j + 1];
                entries[j + 1] = temp;
            }
        }
    }

    for(u32 i = 0; i < ARRAY_LENGTH(entries); i++) {
        if(entries[i].size > 20) {
            logInfo(arena, "%S %d", entries[i].name, entries[i].size);
        }
    }
}

static const char *
astNodeTypeName(ASTNodeType type) {
    static const char *names[ASTNodeType_Count] = {
        [ASTNodeType_None]                              = "None",
        [ASTNodeType_SourceUnit]                        = "SourceUnit",
        [ASTNodeType_Import]                            = "Import",
        [ASTNodeType_EnumDefinition]                    = "EnumDefinition",
        [ASTNodeType_Struct]                            = "Struct",
        [ASTNodeType_BaseType]                          = "BaseType",
        [ASTNodeType_FunctionType]                      = "FunctionType",
        [ASTNodeType_MappingType]                       = "MappingType",
        [ASTNodeType_IdentifierPath]                    = "IdentifierPath",
        [ASTNodeType_ArrayType]                         = "ArrayType",
        [ASTNodeType_Error]                             = "Error",
        [ASTNodeType_Event]                             = "Event",
        [ASTNodeType_Typedef]                           = "Typedef",
        [ASTNodeType_ConstVariable]                     = "ConstVariable",
        [ASTNodeType_NumberLitExpression]               = "NumberLitExpression",
        [ASTNodeType_StringLitExpression]               = "StringLitExpression",
        [ASTNodeType_BoolLitExpression]                 = "BoolLitExpression",
        [ASTNodeType_IdentifierExpression]              = "IdentifierExpression",
        [ASTNodeType_BinaryExpression]                  = "BinaryExpression",
        [ASTNodeType_TupleExpression]                   = "TupleExpression",
        [ASTNodeType_UnaryExpression]                   = "UnaryExpression",
        [ASTNodeType_FunctionCallExpression]            = "FunctionCallExpression",
        [ASTNodeType_MemberAccessExpression]            = "MemberAccessExpression",
        [ASTNodeType_ArrayAccessExpression]             = "ArrayAccessExpression",
        [ASTNodeType_FunctionDefinition]                = "FunctionDefinition",
        [ASTNodeType_BlockStatement]                    = "BlockStatement",
        [ASTNodeType_ReturnStatement]                   = "ReturnStatement",
        [ASTNodeType_ExpressionStatement]               = "ExpressionStatement",
        [ASTNodeType_IfStatement]                       = "IfStatement",
        [ASTNodeType_VariableDeclarationStatement]      = "VariableDeclarationStatement",
        [ASTNodeType_VariableDeclaration]               = "VariableDeclaration",
        [ASTNodeType_NewExpression]                     = "NewExpression",
        [ASTNodeType_VariableDeclarationTupleStatement] = "VariableDeclarationTupleStatement",
        [ASTNodeType_WhileStatement]                    = "WhileStatement",
        [ASTNodeType_ContractDefinition]                = "ContractDefinition",
        [ASTNodeType_RevertStatement]                   = "RevertStatement",
        [ASTNodeType_StateVariableDeclaration]          = "StateVariableDeclaration",
        [ASTNodeType_LibraryDefinition]                 = "LibraryDefinition",
        [ASTNodeType_TerneryExpression]                 = "TerneryExpression",
        [ASTNodeType_ForStatement]                      = "ForStatement",
        [ASTNodeType_BreakStatement]                    = "BreakStatement",
        [ASTNodeType_ContinueStatement]                 = "ContinueStatement",
        [ASTNodeType_UnaryExpressionPostfix]            = "UnaryExpressionPostfix",
        [ASTNodeType_HexStringLitExpression]            = "HexStringLitExpression",
        [ASTNodeType_ArraySliceExpression]              = "ArraySliceExpression",
        [ASTNodeType_UncheckedBlockStatement]           = "UncheckedBlockStatement",
        [ASTNodeType_ModifierDefinition]                = "ModifierDefinition",
        [ASTNodeType_FallbackFunction]                  = "FallbackFunction",
        [ASTNodeType_ReceiveFunction]                   = "ReceiveFunction",
        [ASTNodeType_EmitStatement]                     = "EmitStatement",
        [ASTNodeType_ConstructorDefinition]             = "ConstructorDefinition",
        [ASTNodeType_NamedParameterExpression]          = "NamedParameterExpression",
        [ASTNodeType_InterfaceDefinition]               = "InterfaceDefinition",
        [ASTNodeType_AbstractContractDefinition]        = "AbstractContractDefinition",
        [ASTNodeType_InheritanceSpecifier]              = "InheritanceSpecifier",
        [ASTNodeType_Pragma]                            = "Pragma",
        [ASTNodeType_ModifierInvocation]                = "ModifierInvocation",
        [ASTNodeType_Using]                             = "Using",
        [ASTNodeType_UnicodeStringLitExpression]        = "UnicodeStringLitExpression",
        [ASTNodeType_InlineArrayExpression]             = "InlineArrayExpression",
        [ASTNodeType_DoWhileStatement]                  = "DoWhileStatement",
        [ASTNodeType_TryStatement]                      = "TryStatement",
        [ASTNodeType_CatchStatement]                    = "CatchStatement",
        [ASTNodeType_AssemblyStatement]                 = "AssemblyStatement",
        [ASTNodeType_YulBlockStatement]                 = "YulBlockStatement",
        [ASTNodeType_YulVariableDeclaration]            = "YulVariableDeclaration",
        [ASTNodeType_YulNumberLitExpression]            = "YulNumberLitExpression",
        [ASTNodeType_YulStringLitExpression]            = "YulStringLitExpression",
        [ASTNodeType_YulHexNumberLitExpression]         = "YulHexNumberLitExpression",
        [ASTNodeType_YulBoolLitExpression]              = "YulBoolLitExpression",
        [ASTNodeType_YulHexStringLitExpression]         = "YulHexStringLitExpression",
        [ASTNodeType_YulMemberAccessExpression]         = "YulMemberAccessExpression",
        [ASTNodeType_YulFunctionCallExpression]         = "YulFunctionCallExpression",
        [ASTNodeType_YulVariableAssignment]             = "YulVariableAssignment",
        [ASTNodeType_YulIfStatement]                    = "YulIfStatement",
        [ASTNodeType_YulForStatement]                   = "YulForStatement",
        [ASTNodeType_YulLeaveStatement]                 = "YulLeaveStatement",
        [ASTNodeType_YulBreakStatement]                 = "YulBreakStatement",
        [ASTNodeType_YulContinueStatement]              = "YulContinueStatement",
        [ASTNodeType_YulFunctionDefinition]             = "YulFunctionDefinition",
        [ASTNodeType_YulSwitchStatement]                = "YulSwitchStatement",
        [ASTNodeType_YulCaseStatement]                  = "YulCaseStatement",
    };

    assert(type < ASTNodeType_Count);
    const char *name = names[type];
    assert(name != 0x0);
    return name;
}

static void
printNodeHistogram(Parser *parser) {
    assert(parser != 0x0);
    assert(parser->nodeCount <= parser->nodeCapacity);

    u32 counts[ASTNodeType_Count] = {0};
    for(u32 i = 0; i < parser->nodeCount; i++) {
        ASTNodeType type = parser->nodes[i].type;
        assert(type < ASTNodeType_Count);
        counts[type] += 1;
    }

    u32 order[ASTNodeType_Count];
    for(u32 i = 0; i < ASTNodeType_Count; i++) {
        order[i] = i;
    }

    // Selection sort by count descending. Bounded, predictable, no recursion.
    for(u32 i = 0; i < ASTNodeType_Count; i++) {
        u32 maxIndex = i;
        for(u32 j = i + 1; j < ASTNodeType_Count; j++) {
            if(counts[order[j]] > counts[order[maxIndex]]) {
                maxIndex = j;
            }
        }
        u32 tmp = order[i];
        order[i] = order[maxIndex];
        order[maxIndex] = tmp;
    }

    u32 total = parser->nodeCount;
    printf("Node histogram (%u nodes total):\n", total);
    for(u32 i = 0; i < ASTNodeType_Count; i++) {
        u32 type = order[i];
        u32 count = counts[type];
        if(count == 0) { break; }
        double percent = total > 0 ? (100.0 * (double)count / (double)total) : 0.0;
        printf("  %-36s %8u  %6.2f%%\n", astNodeTypeName(type), count, percent);
    }
}

static ASTNode
parseSourceUnit(Parser *parser) {
    ASTNode node = { .type = ASTNodeType_SourceUnit };
    ASTNodeSourceUnit *sourceUnit = &node.sourceUnitNode;

    // printASTNodeSizes(parser->arena);

    ASTNode *lastChild = 0x0;
    while(true) {
        ASTNode *child = allocateNode(parser);

        if(acceptToken(parser, TokenType_Pragma)) {
            parsePragma(parser, child);
        } else if(acceptToken(parser, TokenType_Import)) {
            parseImport(parser, child);
        } else if(acceptToken(parser, TokenType_Using)) {
            parseUsing(parser, child);
        } else if(acceptToken(parser, TokenType_Enum)) {
            parseEnum(parser, child);
        } else if(acceptToken(parser, TokenType_Struct)) {
            parseStruct(parser, child);
        } else if(acceptToken(parser, TokenType_Error)) {
            parseError(parser, child);
        } else if(acceptToken(parser, TokenType_Event)) {
            parseEvent(parser, child);
        } else if(acceptToken(parser, TokenType_Type)) {
            parseTypedef(parser, child);
        } else if(acceptToken(parser, TokenType_Function)) {
            parseFunction(parser, child);
        } else if(acceptToken(parser, TokenType_Contract)) {
            parseContract(parser, child);
        } else if(acceptToken(parser, TokenType_Abstract)) {
            parseAbstractContract(parser, child);
        } else if(acceptToken(parser, TokenType_Interface)) {
            parseInterface(parser, child);
        } else if(acceptToken(parser, TokenType_Library)) {
            parseLibrary(parser, child);
        } else if(acceptToken(parser, TokenType_EOF)) {
            node.endToken = MIN(parser->current - 2, parser->tokenCount - 1);
            break;
        } else {
            u32 startToken = parser->current;
            ASTNode *type = allocateNode(parser);
            bool success = parseType(parser, type);
            if(!success) {
                reportError(parser, "Expected type, but got (\"%S\")", peekLastTokenString(parser));
            }
            expectToken(parser, TokenType_Constant);
            parseConstVariable(parser, child, type);
            child->startToken = startToken;
        }

        SLL_QUEUE_PUSH(sourceUnit->firstChild, lastChild, child);
    }

    // printNodeHistogram(parser);

    return node;
}
