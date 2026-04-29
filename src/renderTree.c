typedef struct Writer {
    u8 *data;
    u32 size;
    u32 capacity;
    u32 lineSize;
} Writer;

typedef enum WordType: u8 {
    WordType_Text,
    WordType_Line,
    WordType_Space,
    WordType_CommentStartSpace,
    WordType_HardBreak,
    WordType_GroupPush,
    WordType_GroupPop,
    WordType_Count,
} WordType;

typedef struct Word {
    WordType type;

    // NOTE(radomski): This is for GroupPush only. It the number of words that we assume are flat.
    // -1 means consider the entire group as flat.
    // Anything else means:
    //  - considers this many words as flat and sum their widths
    //  - continue considering following words in break mode
    //   - lines, hardbreaks, starting of comments finish the function
    s8 assumedFlatCount;
    s8 nestStep;
    s8 groupStep;

    u32 textSize;
    u8 *text;
} Word;

typedef struct Render {
    u32 indentSize;

    Writer writer;
    TokenizeResult tokens;
    u8 *sourceBaseAddress;

    u32 scratchBufferSize;
    u32 scratchBufferCapacity;
    u8 *scratchBuffer;

    Word *words;
    s32 wordCount;
    u32 wordCapacity;

    u32 trailingCount;
} Render;

static void
commitIndent(Writer *w, u32 spaceCount) {
    if(spaceCount <= 16) {
        u64 *output = (u64 *)(w->data + w->size);
        output[0] = 0x2020202020202020;
        output[1] = 0x2020202020202020;
    } else {
        memset(w->data + w->size, ' ', spaceCount);
    }

    w->size += spaceCount;
    w->lineSize = spaceCount;
}

__attribute__((no_sanitize("address")))
static void
writeString(Writer *w, u8 *str, u32 size, u32 nest) {
    if (size == 0) {
        return;
    }

    if (w->lineSize == 0) {
        commitIndent(w, nest);
    }

    if (size <= 16) {
        u64 *output = (u64 *)(w->data + w->size);
        u64 *input = (u64 *)(str);
        // NOTE(radomski): needs to be like this to subdue compiler's aliasing rules
        u64 v0 = input[0];
        u64 v1 = input[1];
        output[0] = v0;
        output[1] = v1;
    } else {
        memcpy(w->data + w->size, str, size);
    }

    w->size += size;
    w->lineSize += size;
}

static void
finishLine(Writer *w) {
    w->data[w->size++] = '\n';
    w->lineSize = 0;
}

static bool
isWordWhitespaceType(WordType t) {
    return (t == WordType_Space) | (t == WordType_CommentStartSpace) |
           (t == WordType_Line)  | (t == WordType_HardBreak);
}

static void
commitTrailing(Render *r) {
    r->trailingCount = 0;
}

static void
pushTrailing(Render *r, Word w) {
    r->words[r->wordCount++] = w;
    r->trailingCount++;
}

static bool
fits(Word *words, s64 count, s8 assumedFlatCount, u64 remainingWidth) {
    u64 width = 0;
    s64 i = 0;

    s32 groupStack = 0;

    s64 assumedFlatLimit = MIN(count, (s64)((u32)((s32)assumedFlatCount)));

    while (i + 3 < assumedFlatLimit &&
           groupStack >= 0 &&
           width       <= remainingWidth)
    {
        width      += words[i].textSize;
        groupStack += words[i++].groupStep;
        if (groupStack < 0) break;

        width      += words[i].textSize;
        groupStack += words[i++].groupStep;
        if (groupStack < 0) break;

        width      += words[i].textSize;
        groupStack += words[i++].groupStep;
        if (groupStack < 0) break;

        width      += words[i].textSize;
        groupStack += words[i++].groupStep;
        if (groupStack < 0) break;
    }

    for (; i < assumedFlatLimit && groupStack >= 0 && width <= remainingWidth; ++i) {
        width      += words[i].textSize;
        groupStack += words[i].groupStep;
    }

    for (;
         i < count &&
         width <= remainingWidth &&
         words[i].type != WordType_HardBreak &&
         words[i].type != WordType_CommentStartSpace &&
         words[i].type != WordType_Line;
         i++) {
        Word *word = &words[i];
        width += word->textSize;
    }

    return width <= remainingWidth;
}

static void
renderDocument(Render *r) {
    commitTrailing(r);

    Writer *w = &r->writer;
    Word *words = r->words;
    s64 count = r->wordCount;

    u32 nest = 0;
    u8 flatModeStack[64];
    u8 fullyFlatStack[64];
    u8 stackIndex = 0;
    bool flatMode = false;
    u32 nestActiveMask = 0;

    for(s64 i = 0; i < count; i++) {
        Word *word = &words[i];
        WordType type = word->type;

        if (type == WordType_GroupPush) {
            bool isFullyFlat = word->assumedFlatCount == -1;
            bool parentIsFullyFlat = stackIndex > 0 && fullyFlatStack[stackIndex - 1];

            flatModeStack[stackIndex] = flatMode;
            fullyFlatStack[stackIndex] = isFullyFlat;
            stackIndex++;

            // Optimization: if parent group fits and both are fully flat,
            // this group will also fit (skip fits() calculation)
            if (!(flatMode && isFullyFlat && parentIsFullyFlat)) {
                s32 remainingWidth = 120 - MIN(120, MAX(w->lineSize, nest));
                flatMode = fits(words + (i + 1), count - (i + 1), word->assumedFlatCount, remainingWidth);
            }
            nestActiveMask = ((u32)flatMode) - 1;
        } else if(type == WordType_GroupPop) {
            stackIndex--;
            flatMode = flatModeStack[stackIndex];
            nestActiveMask = ((u32)flatMode) - 1;
        }

        if(type == WordType_HardBreak || (!flatMode && type == WordType_Line)) {
            finishLine(w);
        } else {
            writeString(w, word->text, word->textSize, nest);
        }

        nest += word->nestStep & nestActiveMask;
    }
}

static void
insertWordBeforeTrailing(Render *r, Word w) {
    u32 trailing = r->trailingCount;
    if (trailing == 0) {
        r->words[r->wordCount++] = w;
        return;
    }
    s32 insertPos = r->wordCount - trailing;
    for (u32 i = r->wordCount; i > (u32)insertPos; i--) {
        r->words[i] = r->words[i - 1];
    }
    r->words[insertPos] = w;
    r->wordCount++;
}

static void
pushGroup(Render *r) {
    Word w = { .type = WordType_GroupPush, .assumedFlatCount = -1, .groupStep = 1 };
    insertWordBeforeTrailing(r, w);
}

static void
pushGroupAssumedFlat(Render *r, s8 assumedFlatCount) {
    Word w = { .type = WordType_GroupPush, .assumedFlatCount = assumedFlatCount, .groupStep = 1 };
    insertWordBeforeTrailing(r, w);
}

static void
popGroup(Render *r) {
    Word w = { .type = WordType_GroupPop, .groupStep = -1 };
    insertWordBeforeTrailing(r, w);
}

static void
modifyNestAt(Render *r, s32 index, s32 delta) {
    r->words[index].nestStep += delta * r->indentSize;
}

static void
pushNest(Render *r) {
    modifyNestAt(r, r->wordCount - 1 - r->trailingCount, 1);
}

static void
popNest(Render *r) {
    modifyNestAt(r, r->wordCount - 1 - r->trailingCount, -1);
}

static void
pushWord(Render *r, Word w) {
    if (r->trailingCount == 0) {
        r->words[r->wordCount++] = w;
        return;
    }

    Word last = r->words[r->wordCount - 1];
    bool skip =
        (last.type == WordType_HardBreak) &
        isWordWhitespaceType(w.type);

    commitTrailing(r);

    if (!skip) {
        r->words[r->wordCount++] = w;
    }
}

static Word
wordText(String text) {
    return (Word) { .type = WordType_Text, .text = text.data, .textSize = text.size };
}

static Word
wordHardBreak(void) {
    return (Word) { .type = WordType_HardBreak, .text = 0x0, .textSize = 0xffffffff };
}

static Word
wordSpace(void) {
    return (Word) { .type = WordType_Space, .text = (u8 *)" ",  .textSize = 1 };
}

static Word
wordCommentStartSpace(void) {
    return (Word) { .type = WordType_CommentStartSpace, .text = (u8 *)" ",  .textSize = 1 };
}

static Word
wordSoftline(void) {
    return (Word) { .type = WordType_Line, .text = (u8 *)"", .textSize = 0 };
}

static Word
wordLine(void) {
    return (Word) { .type = WordType_Line, .text = (u8 *)" ",  .textSize = 1 };
}

static bool
containsSlash(u32 word) {
    u32 mask = 0x2F2F2F2F;
    u32 cmp  = word ^ mask;
    u32 tmp  = ((cmp - 0x01010101) & ~cmp & 0x80808080);
    return tmp != 0;
}

static void
preserveHardBreaksIntoDocument(Render *r, ASTNode *node) {
    u32 tokenIndex = node->startToken;

    String token = r->tokens.tokenStrings[tokenIndex];
    String previousToken = r->tokens.tokenStrings[tokenIndex - 1];

    String inBetween = {
        .data = previousToken.data + previousToken.size,
        .size = token.data >= inBetween.data ? token.data - inBetween.data : 0,
    };

    u32 i = 0;
    bool hasComment = false;
    for (; i + 3 < inBetween.size && !hasComment; i += 4) {
        hasComment = hasComment | (containsSlash(*((u32 *)(inBetween.data + i))));
    }

    for (; i < inBetween.size && !hasComment; i++) {
        hasComment = hasComment | (inBetween.data[i] == '/');
    }

    if (hasComment) {
        return;
    }

    u32 newlines = 0;
    for(s32 i = inBetween.size - 1; i >= 0 && isWhitespace(inBetween.data[i]); i--) {
        newlines += inBetween.data[i] == '\n';
        if(newlines == 2) {
            pushWord(r, wordHardBreak());
            return;
        }
    }
}

static void
pushCommentsInRange(Render *r, u32 startOffset, u32 endOffset) {
    String input = (String) {
        .data = r->sourceBaseAddress + startOffset,
        .size = endOffset - startOffset,
    };

    if(input.size <= 2) {
        return;
    }

    u32 size = input.size;
    u8 *data = input.data;
    bool hasSlash = false;
    u32 k = 0;
    for (; k + 3 < size; k += 4) {
        hasSlash = containsSlash(*((u32 *)(data + k)));
        if (hasSlash) break;
    }
    if (!hasSlash) {
        for (; k < size; k++) {
            if (data[k] == '/') { hasSlash = true; break; }
        }
    }
    if (!hasSlash) return;

    for(; input.size > 0 && !isWhitespace(input.data[0]) && input.data[0] != '/'; input.data++, input.size--) { }

    if(input.size <= 2) {
        return;
    }

    u32 i = 0;
    while(i < input.size) {
        u32 newlineCount = 0;
        for(; i < input.size && isWhitespace(input.data[i]); i++) {
            newlineCount += input.data[i] == '\n';
        }

        if(i < input.size && input.data[i] != '/' && newlineCount == 0) { break; }

        if(newlineCount > 0) {
            u32 toInsert = MIN(newlineCount, 2);
            for(u32 j = 0; j < toInsert; j++) {
                pushTrailing(r, wordHardBreak());
            }
        } else if(i < input.size) {
            pushTrailing(r, wordCommentStartSpace());
        }

        if(i < input.size && input.data[i] != '/') { break; }

        u32 commentStart = i;
        if(i++ >= input.size) { break; }

        if(input.data[i++] == '/') {
            for(; i < input.size && input.data[i] != '\n'; i += 1) {}

            String comment = {
                .data = input.data + commentStart,
                .size = i - commentStart
            };

            pushTrailing(r, wordText(stringTrim(comment)));
        } else {
            bool isStarAligned = true;
            bool checkStarAlignment = false;

            while(i < input.size - 1 && (input.data[i] != '*' || input.data[i + 1] != '/')) {
                if(input.data[i] == '\n') {
                    checkStarAlignment = true;
                }

                if(checkStarAlignment && !(isWhitespace(input.data[i]) || input.data[i] == '*')) {
                    isStarAligned = false;
                }

                if(checkStarAlignment && input.data[i] == '*') {
                    checkStarAlignment = false;
                }

                i += 1;
            }
            i += 2;

            String comment = {
                .data = input.data + commentStart,
                .size = i - commentStart
            };

            if(isStarAligned) {
                String line = { .data = comment.data, .size = 0 };
                u32 lineCount = 0;
                for(u32 k = 0; k < comment.size; k++) {
                    if(comment.data[k] == '\n') {
                        lineCount += 1;
                        if(lineCount > 1) { pushTrailing(r, wordSpace()); }
                        pushTrailing(r, wordText(stringTrim(line)));
                        pushTrailing(r, wordHardBreak());
                        line = (String){ .data = line.data + line.size + 1, .size = 0 };
                    } else {
                        line.size += 1;
                    }
                }

                if(line.size > 0) {
                    if(lineCount > 1) { pushTrailing(r, wordSpace()); }
                    pushTrailing(r, wordText(stringTrim(line)));
                }
            } else {
                pushTrailing(r, wordText(comment));
            }
        }
    }
}

static void
pushCommentsAfterTokenWithCurrent(Render *r, TokenId token, String current) {
    String next = r->tokens.tokenStrings[token + 1];

    u32 startOffset = (current.data - r->sourceBaseAddress) + current.size;
    u32 endOffset = next.data ? next.data - r->sourceBaseAddress : r->tokens.sourceSize;

    pushCommentsInRange(r, startOffset, endOffset);
}

static void
pushCommentsAfterToken(Render *r, TokenId token) {
    assert(token < r->tokens.count);
    pushCommentsAfterTokenWithCurrent(r, token, r->tokens.tokenStrings[token]);
}

static void
pushTokenWord(Render *r, TokenId token) {
    String text = r->tokens.tokenStrings[token];
    pushWord(r, wordText(text));
    pushCommentsAfterTokenWithCurrent(r, token, text);
}

static void
pushTokenAsStringWord(Render *r, TokenId token) {
    String text = getTokenString(r->tokens, token);

    if(text.data[-1] == '"' && text.data[text.size] == '"') {
        text.data -= 1;
        text.size += 2;
    } else {
        u8 *data = r->scratchBuffer + r->scratchBufferSize;
        u8 *head = data;
        *head++ = '"';
        for (size_t i = 0; i < text.size; ++i) {
            u8 current = text.data[i];
            if (current == '"') {
                *head++ = '\\';
                *head++ = '"';
            } else if (current == '\\' && (i + 1 < text.size) && text.data[i + 1] == '\'') {
                *head++ = '\'';
                i++;
            } else if (current == '\\' && (i + 1 < text.size) && text.data[i + 1] == '"') {
                *head++ = '\\';
                *head++ = '"';
                i++;
            } else {
                *head++ = current;
            }
        }
        *head++ = '"';
        text.data = data;
        text.size = (u32)(head - data);
        r->scratchBufferSize += text.size;
        assert(r->scratchBufferSize <= r->scratchBufferCapacity && "Scratch buffer overflow detected when wrapping string with quotes");
    }

    pushWord(r, wordText(text));
    pushCommentsAfterToken(r, token);
}

static void pushExpressionDocument(Render *r, ASTNode *node);

static void
pushCallArgumentListDocument(Render *r, TokenId startingToken, ASTNode *firstExpression, TokenIdList *names) {
    pushGroup(r);
    pushTokenWord(r, startingToken);
    if(names->count > 0) {
        pushTokenWord(r, listGetTokenId(names, 0) - 1);
    }
    pushNest(r);

    ASTNode *lastExpression = 0x0;
    Word whitespace = names->count == 0 ? wordSoftline() : wordLine();
    if(names->count == 0) {
        if(firstExpression) {
            pushWord(r, whitespace);
        }

        ASTNode *argument = firstExpression;
        for(u32 i = 0; argument != 0x0; i++, argument = argument->next) {
            lastExpression = argument;
            pushGroup(r);
            pushExpressionDocument(r, argument);
            popGroup(r);
            if(argument->next != 0x0) {
                // Add comma and space after each argument except the last one
                pushTokenWord(r, argument->endToken + 1);
                pushWord(r, wordLine());
            }
        }
    } else {
        pushWord(r, whitespace);
        ASTNode *argument = firstExpression;
        for(u32 i = 0; argument != 0x0; i++, argument = argument->next) {
            lastExpression = argument;
            pushGroup(r);
            TokenId literal = listGetTokenId(names, i);
            pushTokenWord(r, literal);
            pushTokenWord(r, literal + 1);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, argument);
            popGroup(r);

            if(argument->next != 0x0) {
                pushTokenWord(r, argument->endToken + 1);
                pushWord(r, wordLine());
            }
        }
    }

    TokenId endToken = 0;
    if(firstExpression != 0x0) {
        TokenId token = lastExpression->endToken + 1;
        token += names->count > 0;
        endToken = token;
        pushWord(r, whitespace);
    } else {
        endToken = startingToken + 1;
    }

    commitTrailing(r);

    if(names->count > 0) {
        popNest(r);
        pushTokenWord(r, lastExpression->endToken + 1);
    } else {
        popNest(r);
    }

    pushTokenWord(r, endToken);
    popGroup(r);

    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[startingToken]));
    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[endToken]));
}

static void pushParametersDocument(Render *r, TokenId openParenToken, ASTNode *first);

static void
pushTypeDocument(Render *r, ASTNode *node) {
    switch(node->type) {
        case ASTNodeType_BaseType: {
            pushTokenWord(r, node->baseTypeNode.typeName);
            if(node->baseTypeNode.payable) {
                pushWord(r, wordSpace());
                pushTokenWord(r, node->endToken);
            }
        } break;
        case ASTNodeType_IdentifierPath: {
            for(u32 i = 0; i < node->identifierPathNode.identifiers.count; i++) {
                TokenId part = listGetTokenId(&node->identifierPathNode.identifiers, i);
                pushTokenWord(r, part);
                if(i != node->identifierPathNode.identifiers.count - 1) {
                    pushTokenWord(r, part + 1);
                }
            }
        } break;
        case ASTNodeType_MappingType: {
            assert(stringMatch(LIT_TO_STR("mapping"), r->tokens.tokenStrings[node->startToken]));
            pushTokenWord(r, node->startToken);

            TokenId openParen = node->startToken + 1;
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParen]));

            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, openParen);
            pushWord(r, wordSoftline());
            pushTypeDocument(r, node->mappingNode.keyType);
            pushWord(r, wordSpace());
            if(node->mappingNode.keyIdentifier != INVALID_TOKEN_ID) {
                pushTokenWord(r, node->mappingNode.keyIdentifier);
                pushWord(r, wordSpace());
            }

            pushTokenWord(r, node->mappingNode.valueType->startToken - 2);
            pushTokenWord(r, node->mappingNode.valueType->startToken - 1);
            pushWord(r, wordSpace());

            pushTypeDocument(r, node->mappingNode.valueType);
            if(node->mappingNode.valueIdentifier != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, node->mappingNode.valueIdentifier);
            }
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_ArrayType: {
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[node->arrayTypeNode.elementType->endToken + 1]));
            pushTypeDocument(r, node->arrayTypeNode.elementType);

            pushTokenWord(r, node->arrayTypeNode.elementType->endToken + 1);
            if(node->arrayTypeNode.lengthExpression != 0x0) {
                pushExpressionDocument(r, node->arrayTypeNode.lengthExpression);
            }
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_FunctionType: {
            assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[node->startToken]));

            pushTokenWord(r, node->startToken);

            pushGroup(r);
            pushParametersDocument(r, node->startToken + 1, node->functionTypeNode.firstParameter);

            if(node->functionTypeNode.visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, node->functionTypeNode.visibility);
            }
            if(node->functionTypeNode.stateMutability != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, node->functionTypeNode.stateMutability);
            }

            if(node->functionTypeNode.firstReturnParameter) {
                pushWord(r, wordSpace());
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[node->functionTypeNode.firstReturnParameter->startToken - 2]));

                pushGroup(r);
                pushTokenWord(r, node->functionTypeNode.firstReturnParameter->startToken - 2);
                pushWord(r, wordSpace());
                pushParametersDocument(r, node->functionTypeNode.firstReturnParameter->startToken - 1, node->functionTypeNode.firstReturnParameter);
                popGroup(r);
            }
            popGroup(r);

            // assert(false);
        } break;
        default: {
            assert(0);
        }
    }
}

static void pushExpressionDocumentAssignment(Render *r, ASTNode *node);

static bool
shouldFlattenBinaryExpression(ASTNode *outerNode, ASTNode *innerNode) {
    if(outerNode->type != ASTNodeType_BinaryExpression || innerNode->type != ASTNodeType_BinaryExpression) {
        return false;
    }

    ASTNodeBinaryExpression *outerExpression = &outerNode->binaryExpressionNode;
    ASTNodeBinaryExpression *innerExpression = &innerNode->binaryExpressionNode;

    TokenType outerOperator = outerExpression->operator;
    TokenType innerOperator = innerExpression->operator;

    u32 outerPrecedence = getBinaryOperatorPrecedence(outerOperator);
    u32 innerPrec = getBinaryOperatorPrecedence(innerOperator);
    if(outerPrecedence != innerPrec) {
        return false;
    }

    if(outerOperator == TokenType_StarStar) {
        return false;
    }

    if(outerOperator == TokenType_EqualEqual && innerOperator == TokenType_EqualEqual) {
        return false;
    }

    return true;
}

static void
pushBinaryExpressionDocument(Render *r, ASTNode *node, TokenId outerOperator) {
    ASTNodeBinaryExpression *binary = &node->binaryExpressionNode;

    bool onlyTwoElementBinaryExpression =
        outerOperator == TokenType_None &&
        binary->left->type != ASTNodeType_BinaryExpression &&
        binary->right->type != ASTNodeType_BinaryExpression;

    u32 outerPrecedence = getBinaryOperatorPrecedence(outerOperator);
    u32 innerPrec = getBinaryOperatorPrecedence(binary->operator);
    bool startingPrecedence = outerPrecedence == 0;
    bool differingPrecedence = outerPrecedence != innerPrec;
    bool openGroup = differingPrecedence && !startingPrecedence;

    bool outerHigherThanAssignment = outerPrecedence > getBinaryOperatorPrecedence(TokenType_Equal);
    bool notInequality = innerPrec != getBinaryOperatorPrecedence(TokenType_LTick);
    bool notEqualities = innerPrec != getBinaryOperatorPrecedence(TokenType_EqualEqual);
    bool outerNotInequality = outerPrecedence != getBinaryOperatorPrecedence(TokenType_LTick);
    bool outerNotEquality = outerPrecedence != getBinaryOperatorPrecedence(TokenType_EqualEqual);
    bool base = !startingPrecedence && outerHigherThanAssignment && notInequality && notEqualities && outerNotInequality && outerNotEquality;

    bool outerHasLowerPrecedence = outerPrecedence < innerPrec;
    bool outerNotAdd = outerPrecedence != getBinaryOperatorPrecedence(TokenType_Plus);
    bool outerNotMultipliaction = outerPrecedence != getBinaryOperatorPrecedence(TokenType_Star);
    bool operatorsDontMatch = binary->operator != outerOperator;

    // Cares about different things depending on the outer and the inner operator
    bool outerDependentCase = outerNotAdd && (
        (outerNotMultipliaction && outerHasLowerPrecedence && innerPrec < getBinaryOperatorPrecedence(TokenType_StarStar)) ||
        (operatorsDontMatch && innerPrec == getBinaryOperatorPrecedence(TokenType_Star)) ||
        (outerNotMultipliaction && innerPrec == getBinaryOperatorPrecedence(TokenType_StarStar))
    );

    // Doesn't care about what the outer is, always is present as long as base is also true
    bool alwaysPresentCase = innerPrec == getBinaryOperatorPrecedence(TokenType_LeftShift) || (binary->operator == TokenType_Percent);
    bool addParens = base && (outerDependentCase || alwaysPresentCase);

    if(addParens) {
        pushGroup(r);
        pushWord(r, wordText(LIT_TO_STR("(")));
        pushExpressionDocument(r, node);
        pushWord(r, wordText(LIT_TO_STR(")")));
        popGroup(r);
    } else if(openGroup) {
        pushExpressionDocument(r, node);
    } else {
        if(binary->left->type == ASTNodeType_BinaryExpression) {
            pushBinaryExpressionDocument(r, binary->left, binary->operator);
        } else {
            pushGroup(r);
            pushExpressionDocument(r, binary->left);
            popGroup(r);
        }

        pushWord(r, wordSpace());

        if(onlyTwoElementBinaryExpression) { pushGroup(r); }

        pushTokenWord(r, binary->right->startToken - 1);
        pushWord(r, wordLine());

        if(binary->right->type == ASTNodeType_BinaryExpression) {
            pushBinaryExpressionDocument(r, binary->right, binary->operator);
        } else {
            pushExpressionDocument(r, binary->right);
        }

        if(onlyTwoElementBinaryExpression) { popGroup(r); }
    }

}

static void pushExpressionDocumentNoIndent(Render *r, ASTNode *node);
static void pushExpressionDocumentOrMemberChain(Render *r, ASTNode *node);
static void pushExpressionDocumentNoChaining(Render *r, ASTNode *node);

static void
pushExpressionDocument(Render *r, ASTNode *node) {
    switch(node->type) {
        case ASTNodeType_NumberLitExpression: {
            pushTokenWord(r, node->numberLitExpressionNode.value);
            if(node->numberLitExpressionNode.subdenomination != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, node->numberLitExpressionNode.subdenomination);
            }
        } break;
        case ASTNodeType_HexStringLitExpression:
        case ASTNodeType_UnicodeStringLitExpression:
        case ASTNodeType_StringLitExpression: {
            ASTNodeStringLitExpression *expression = &node->stringLitExpressionNode;

            Word w = (node->type == ASTNodeType_HexStringLitExpression) ? wordText(LIT_TO_STR("hex")) :
                     (node->type == ASTNodeType_UnicodeStringLitExpression) ? wordText(LIT_TO_STR("unicode")) :
                     wordText(LIT_TO_STR(""));

            for(u32 i = 0; i < expression->values.count; i++) {
                if(i > 0) {
                    pushWord(r, wordHardBreak());
                }

                pushWord(r, w);
                TokenId literal = listGetTokenId(&expression->values, i);
                pushTokenAsStringWord(r, literal);
            }
        } break;
        case ASTNodeType_BoolLitExpression: {
            pushTokenWord(r, node->boolLitExpressionNode.value);
        } break;
        case ASTNodeType_IdentifierPath:
        case ASTNodeType_ArrayType:
        case ASTNodeType_MappingType: {
            pushTypeDocument(r, node);
        } break;
        case ASTNodeType_IdentifierExpression: {
            pushTokenWord(r, node->identifierExpressionNode.value);
        } break;
        case ASTNodeType_BinaryExpression: {
            pushGroup(r);

            ASTNodeBinaryExpression *binary = &node->binaryExpressionNode;
            if(binary->operator >= TokenType_Equal && binary->operator <= TokenType_PercentEqual) {
                pushExpressionDocument(r, binary->left);
                pushWord(r, wordSpace());
                pushTokenWord(r, binary->right->startToken - 1);
                pushExpressionDocumentAssignment(r, binary->right);
            } else {
                s32 wordStartIndex = r->wordCount - r->trailingCount;

                pushBinaryExpressionDocument(r, node, TokenType_None);

                s32 wordEndIndex = r->wordCount - r->trailingCount;
                s32 groupPushIndex = 0;
                for(s32 i = wordStartIndex; i < wordEndIndex; i++) {
                    if(r->words[i].type == WordType_GroupPush) {
                        groupPushIndex = i;
                        break;
                    }
                }

                s32 groupStack = 0;
                s32 groupPopIndex = groupPushIndex + 1;
                for(; groupPopIndex < wordEndIndex; groupPopIndex++) {
                    groupStack += r->words[groupPopIndex].groupStep;
                    if(groupStack < 0) break;
                }

                assert(groupPushIndex != 0 && r->words[groupPushIndex].type == WordType_GroupPush);
                assert(groupPopIndex != 0  && r->words[groupPopIndex].type  == WordType_GroupPop);

                modifyNestAt(r, groupPopIndex, 1);
                modifyNestAt(r, wordEndIndex - 1, -1);
            }

            popGroup(r);
        } break;
        case ASTNodeType_TupleExpression: {
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));

            ASTNodeTupleExpression *tuple = &node->tupleExpressionNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushNest(r);
            ASTNode *element = tuple->firstElement;

            if(tuple->firstElement != 0x0 && tuple->firstElement->next == 0x0 && element->type != ASTNodeType_None) {
                pushWord(r, wordSoftline());
                pushExpressionDocumentNoIndent(r, element);
                pushWord(r, wordSoftline());
            } else {
                TokenId commaToken = node->startToken;
                for(u32 i = 0; element != 0x0; i++, element = element->next) {
                    if(i == 0) { pushWord(r, wordSoftline()); }
                    bool isNamed = element->type != ASTNodeType_None;
                    if(isNamed) {
                        if(i > 0) { pushWord(r, wordLine()); }
                        pushExpressionDocument(r, element);
                    }

                    if(element->next != 0x0) {
                        commaToken = isNamed ? element->endToken + 1 : commaToken + 1;
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[commaToken]));
                        pushTokenWord(r, commaToken);
                    }

                    if(element->next == 0x0) { pushWord(r, wordSoftline()); }
                }
            }

            commitTrailing(r);
            popNest(r);
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_UnaryExpression: {
            pushTokenWord(r, node->startToken);
            if(node->unaryExpressionNode.operator == TokenType_Delete) {
                pushWord(r, wordSpace());
            }
            pushExpressionDocument(r, node->unaryExpressionNode.subExpression);

            assert(stringMatch(tokenTypeToString(node->unaryExpressionNode.operator), getTokenString(r->tokens, node->startToken)));
        } break;
        case ASTNodeType_UnaryExpressionPostfix: {
            pushExpressionDocument(r, node->unaryExpressionNode.subExpression);
            pushTokenWord(r, node->endToken);
            assert(stringMatch(tokenTypeToString(node->unaryExpressionNode.operator), getTokenString(r->tokens, node->endToken)));
        } break;
        case ASTNodeType_NewExpression: {
            assert(stringMatch(LIT_TO_STR("new"), r->tokens.tokenStrings[node->startToken]));
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSpace());
            pushTypeDocument(r, node->newExpressionNode.typeName);
        } break;
        case ASTNodeType_FunctionCallExpression: {
            pushExpressionDocumentOrMemberChain(r, node);
        } break;
        case ASTNodeType_BaseType: {
            pushTokenWord(r, node->baseTypeNode.typeName);
        } break;
        case ASTNodeType_MemberAccessExpression: {
            ASTNodeMemberAccessExpression *member = &node->memberAccessExpressionNode;
            pushExpressionDocument(r, member->expression);
            commitTrailing(r);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, member->memberName - 1);
            pushTokenWord(r, member->memberName);
            popNest(r);
            popGroup(r);
        } break;
        case ASTNodeType_ArrayAccessExpression: {
            // NOTE(radomski): There should be a way in which you can print things from left to right instead of right to left.
            // This means that the left most array element will break instead of the right most one.
            ASTNodeArrayAccessExpression *array = &node->arrayAccessExpressionNode;
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[array->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));

            pushExpressionDocument(r, array->expression);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, array->expression->endToken + 1);
            pushWord(r, wordSoftline());
            if(array->indexExpression != 0x0) {
                pushExpressionDocument(r, array->indexExpression);
            }
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_ArraySliceExpression: {
            ASTNodeArraySliceExpression *array = &node->arraySliceExpressionNode;
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[array->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));

            pushExpressionDocument(r, array->expression);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, array->expression->endToken + 1);
            pushWord(r, wordSoftline());
            if(array->leftFenceExpression != 0x0) {
                pushGroup(r);
                pushExpressionDocument(r, array->leftFenceExpression);
                popGroup(r);

                pushTokenWord(r, array->leftFenceExpression->endToken + 1);
            } else {
                pushTokenWord(r, array->expression->endToken + 2);
            }
            if(array->rightFenceExpression != 0x0) {
                pushGroup(r);
                pushWord(r, wordSoftline());
                pushExpressionDocument(r, array->rightFenceExpression);
                popGroup(r);
            }
            pushWord(r, wordSoftline());
            popNest(r);
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_InlineArrayExpression: {
            ASTNodeInlineArrayExpression *array = &node->inlineArrayExpressionNode;
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSoftline());
            pushNest(r);

            pushGroup(r);
            ASTNode *expression = array->firstExpression;
            for(u32 i = 0; expression != 0x0; i++, expression = expression->next) {
                pushExpressionDocument(r, expression);
                if(expression->next != 0x0) {
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[expression->endToken + 1]));
                    pushTokenWord(r, expression->endToken + 1);

                    pushWord(r, wordLine());
                }
            }
            popGroup(r);

            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_TerneryExpression: {
            ASTNodeTerneryExpression *ternery = &node->terneryExpressionNode;
            assert(stringMatch(LIT_TO_STR("?"), r->tokens.tokenStrings[ternery->condition->endToken + 1]));

            pushGroup(r);
            pushExpressionDocument(r, ternery->condition);
            popGroup(r);

            pushNest(r);
            pushWord(r, wordLine());

            pushGroup(r);
            pushTokenWord(r, ternery->condition->endToken + 1);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, ternery->trueExpression);
            popGroup(r);

            pushWord(r, wordLine());

            pushGroup(r);
            pushTokenWord(r, ternery->trueExpression->endToken + 1);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, ternery->falseExpression);
            popGroup(r);

            popNest(r);
        } break;
        case ASTNodeType_NamedParameterExpression: {
            ASTNodeNamedParametersExpression *named = &node->namedParametersExpressionNode;

            pushExpressionDocument(r, named->expression);

            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[named->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[node->endToken]));

            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, named->expression->endToken + 1);
            pushWord(r, wordLine());
            ASTNode *expression = named->firstExpression;
            for(u32 i = 0; expression != 0x0; i++, expression = expression->next) {
                assert(stringMatch(LIT_TO_STR(":"), r->tokens.tokenStrings[listGetTokenId(&named->names, i) + 1]));
                pushTokenWord(r, listGetTokenId(&named->names, i));
                pushTokenWord(r, listGetTokenId(&named->names, i) + 1);
                pushWord(r, wordSpace());
                pushExpressionDocument(r, expression);

                if(expression->next != 0x0) {
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[expression->endToken + 1]));
                    pushTokenWord(r, expression->endToken + 1);
                    pushWord(r, wordLine());
                }
            }
            popNest(r);
            pushWord(r, wordLine());
            popGroup(r);
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_None: { break; }
        default: {
            assert(0);
        }
    }
}

static ASTNodeType_Enum
getNodeTypeStrippingParens(ASTNode *node) {
    if (node->type == ASTNodeType_TupleExpression) {
        ASTNode *first = node->tupleExpressionNode.firstElement;
        if (first != NULL && first->next == 0x0) {
            return getNodeTypeStrippingParens(first);
        }
    }

    return node->type;
}

static void
pushExpressionDocumentNoIndent(Render *r, ASTNode *node) {
    if(node->type == ASTNodeType_BinaryExpression) {
        pushGroup(r);
        pushBinaryExpressionDocument(r, node, TokenType_None);
        popGroup(r);
    } else {
        pushExpressionDocument(r, node);
    }
}

static void
pushExpressionDocumentNoChainingUntil(Render *r, ASTNode *node, ASTNode *stopAt) {
    if(node == stopAt) { return; }

    switch(node->type) {
        case ASTNodeType_FunctionCallExpression: {
            ASTNodeFunctionCallExpression *function = &node->functionCallExpressionNode;

            pushExpressionDocumentNoChainingUntil(r, function->expression, stopAt);
            pushGroup(r);
            pushCallArgumentListDocument(r, function->expression->endToken + 1, function->firstArgumentExpression, &function->argumentsName);
            popGroup(r);
        } break;
        case ASTNodeType_ArrayAccessExpression: {
            ASTNodeArrayAccessExpression *array = &node->arrayAccessExpressionNode;
            pushExpressionDocumentNoChainingUntil(r, array->expression, stopAt);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, array->expression->endToken + 1);
            pushWord(r, wordSoftline());
            if(array->indexExpression != 0x0) {
                pushExpressionDocument(r, array->indexExpression);
            }
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_MemberAccessExpression: {
            ASTNodeMemberAccessExpression *member = &node->memberAccessExpressionNode;
            pushExpressionDocumentNoChainingUntil(r, member->expression, stopAt);
            commitTrailing(r);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, member->memberName - 1);
            pushTokenWord(r, member->memberName);
            popNest(r);
            popGroup(r);
        } break;
        default: {
            pushExpressionDocument(r, node);
        }
    }
}

static void
pushExpressionDocumentNoChaining(Render *r, ASTNode *node) {
    pushExpressionDocumentNoChainingUntil(r, node, 0x0);
}

static void
pushExpressionDocumentOrMemberChain(Render *r, ASTNode *node) {
    assert(node->type == ASTNodeType_FunctionCallExpression);
    if(node->functionCallExpressionNode.expression->type == ASTNodeType_MemberAccessExpression) {
        u32 groupStackIndex = 0;
        ASTNode *groupStack[128] = { 0 };

        groupStack[groupStackIndex++] = node;

        ASTNode *visitor = node;
        bool wasFnCall = false;
        while(visitor) {
            while(visitor && visitor->type != ASTNodeType_MemberAccessExpression) {
                if(visitor->type == ASTNodeType_FunctionCallExpression) {
                    wasFnCall = true;
                    visitor = visitor->functionCallExpressionNode.expression;
                } else if (visitor->type == ASTNodeType_ArrayAccessExpression) {
                    visitor = visitor->arrayAccessExpressionNode.expression;
                } else {
                    visitor = 0;
                }
            }


            if(!visitor) { break; }

            while(
                visitor->type == ASTNodeType_ArrayAccessExpression ||
                visitor->type == ASTNodeType_MemberAccessExpression
            ) {
                if(visitor->type == ASTNodeType_MemberAccessExpression) {
                    visitor = visitor->memberAccessExpressionNode.expression;
                } else {
                    ASTNode *next = visitor->arrayAccessExpressionNode.expression;
                    if(next->type == ASTNodeType_MemberAccessExpression) {
                        visitor = next;
                    } else {
                        break;
                    }
                }
            }

            if(wasFnCall) {
                assert(groupStackIndex < ARRAY_LENGTH(groupStack));
                groupStack[groupStackIndex++] = visitor;
                wasFnCall = false;
            }
        }

        if(groupStackIndex <= 2) {
            pushExpressionDocumentNoChaining(r, node);
            return;
        }

        ASTNode *firstElement = groupStack[--groupStackIndex];
        {
            ASTNode *head = groupStack[groupStackIndex - 1];

            bool wasFunctionCall = false;
            while(head != firstElement) {
                ASTNode *next = 0x0;

                if(head->type == ASTNodeType_FunctionCallExpression) {
                    next = head->functionCallExpressionNode.expression;
                } else if (head->type == ASTNodeType_ArrayAccessExpression) {
                    next = head->arrayAccessExpressionNode.expression;
                } else if (head->type == ASTNodeType_MemberAccessExpression) {
                    next = head->memberAccessExpressionNode.expression;

                    if(wasFunctionCall) { head = next; break; }
                } else {
                    assert(false);
                }

                wasFunctionCall = head->type == ASTNodeType_FunctionCallExpression;
                head = next;
            }

            if(head == firstElement && !wasFunctionCall) {
                firstElement = groupStack[--groupStackIndex];
            } else {
                firstElement = head;
            }

            pushExpressionDocumentNoChaining(r, firstElement);
        }

        pushNest(r);
        Word whitespace = groupStackIndex >= 3 ? wordHardBreak() : wordSoftline();

        ASTNode *stopAt = firstElement;
        for(s32 i = groupStackIndex - 1; i >= 0; --i) {
            ASTNode *entry = groupStack[i];
            pushWord(r, whitespace);
            pushExpressionDocumentNoChainingUntil(r, entry, stopAt);
            stopAt = entry;
        }

        popNest(r);
    } else {
        pushExpressionDocumentNoChaining(r, node);
    }
}

static void
pushExpressionDocumentAssignment(Render *r, ASTNode *node) {
    if (node->type == ASTNodeType_FunctionCallExpression) {
        pushGroupAssumedFlat(r, 1);
        pushWord(r, wordLine());
        pushNest(r);

        pushGroup(r);
        pushExpressionDocumentOrMemberChain(r, node);
        popGroup(r);

        popNest(r);
        popGroup(r);
    } else if(node->type == ASTNodeType_InlineArrayExpression || node->type == ASTNodeType_TupleExpression) {
        pushWord(r, wordSpace());
        pushExpressionDocumentNoIndent(r, node);
    } else if(node->type == ASTNodeType_TerneryExpression) {
        bool complexCondition = getNodeTypeStrippingParens(node->terneryExpressionNode.condition) == ASTNodeType_BinaryExpression;

        pushGroup(r);
        if(complexCondition) { pushNest(r); }
        pushWord(r, complexCondition ? wordLine() : wordSpace());
        pushExpressionDocumentNoIndent(r, node);
        if(complexCondition) { popNest(r); }
        popGroup(r);
    } else {
        pushGroup(r);
        pushNest(r);
        pushWord(r, wordLine());
        pushExpressionDocumentNoIndent(r, node);
        popNest(r);
        popGroup(r);
    }
}

static void pushYulExpressionDocument(Render *r, ASTNode *node);

static void
pushYulFunctionCallDocument(Render *r, ASTNode *node) {
    ASTNodeYulFunctionCallExpression *function = &node->yulFunctionCallExpressionNode;

    pushTokenWord(r, function->identifier);

    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->identifier + 1]));
    pushTokenWord(r, function->identifier + 1);

    if (function->firstArgument != 0x0) {
        pushGroup(r);
        pushNest(r);
        pushWord(r, wordSoftline());

        ASTNode *argument = function->firstArgument;
        for(u32 i = 0; argument != 0x0; i++, argument = argument->next) {
            pushYulExpressionDocument(r, argument);
            if(argument->next != 0x0) {
                pushTokenWord(r, argument->endToken + 1); // comma
                pushWord(r, wordLine());
            }
        }

        popNest(r);
        pushWord(r, wordSoftline());
        popGroup(r);
    }

    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));
    pushTokenWord(r, node->endToken);
}

static void
pushYulExpressionDocument(Render *r, ASTNode *node) {
    switch(node->type) {
        case ASTNodeType_YulNumberLitExpression: {
            pushTokenWord(r, node->yulNumberLitExpressionNode.value);
        } break;
        case ASTNodeType_YulStringLitExpression: {
            pushTokenAsStringWord(r, node->yulStringLitExpressionNode.value);
        } break;
        case ASTNodeType_YulHexNumberLitExpression: {
            pushTokenWord(r, node->yulHexNumberLitExpressionNode.value);
        } break;
        case ASTNodeType_YulBoolLitExpression: {
            pushTokenWord(r, node->yulBoolLitExpressionNode.value);
        } break;
        case ASTNodeType_YulHexStringLitExpression: {
            pushWord(r, wordText(LIT_TO_STR("hex")));
            pushTokenAsStringWord(r, node->yulHexStringLitExpressionNode.value);
        } break;
        case ASTNodeType_YulMemberAccessExpression: {
            ASTNodeYulIdentifierPathExpression *path = &node->yulIdentifierPathExpressionNode;

            for(u32 i = 0; i < path->count; i++) {
                if(i != 0) {
                    assert(stringMatch(LIT_TO_STR("."), r->tokens.tokenStrings[path->identifiers[i - 1] + 1]));
                    pushTokenWord(r, path->identifiers[i - 1] + 1);
                }

                pushTokenWord(r, path->identifiers[i]);
            }

        } break;
        case ASTNodeType_YulFunctionCallExpression: {
            pushYulFunctionCallDocument(r, node);
        } break;
        default: {
            assert(0);
        }
    }
}

static void pushStatementDocumentOpenedBlock(Render *r, ASTNode *node);

static void
pushStatementDocument(Render *r, ASTNode *node) {
    switch(node->type) {
        case ASTNodeType_BlockStatement: {
            ASTNodeBlockStatement *block = &node->blockStatementNode;

            pushTokenWord(r, node->startToken);
            pushNest(r);

            if(block->firstStatement == 0) {
                pushWord(r, wordSpace());
            }

            u32 i = 0;
            for(ASTNode *statement = block->firstStatement; statement != 0x0; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, statement); }
                else { pushWord(r, wordLine()); }

                pushStatementDocument(r, statement);
                pushWord(r, wordHardBreak());
            }

            popNest(r);
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_UncheckedBlockStatement: {
            ASTNodeUncheckedBlockStatement *unchecked = &node->uncheckedBlockStatementNode;
            assert(stringMatch(LIT_TO_STR("unchecked"), r->tokens.tokenStrings[node->startToken]));
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSpace());
            pushStatementDocument(r, unchecked->block);
        } break;
        case ASTNodeType_ReturnStatement: {
            assert(stringMatch(LIT_TO_STR("return"), r->tokens.tokenStrings[node->startToken]));

            pushTokenWord(r, node->startToken);
            if(node->returnStatementNode.expression != 0x0) {
                pushExpressionDocumentAssignment(r, node->returnStatementNode.expression);
            }
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_ExpressionStatement: {
            if(node->expressionStatementNode.expression != 0x0) {
                pushGroup(r);
                pushExpressionDocument(r, node->expressionStatementNode.expression);
                popGroup(r);
                pushTokenWord(r, node->endToken);
            }
        } break;
        case ASTNodeType_IfStatement: {
            assert(stringMatch(LIT_TO_STR("if"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->ifStatementNode.conditionExpression->endToken + 1]));

            pushTokenWord(r, node->startToken);

            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, node->startToken + 1);
            pushWord(r, wordSoftline());
            pushGroup(r);
            pushExpressionDocumentNoIndent(r, node->ifStatementNode.conditionExpression);
            popGroup(r);
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, node->ifStatementNode.conditionExpression->endToken + 1);
            popGroup(r);

            bool trueStatementIsBlock = node->ifStatementNode.trueStatement->type == ASTNodeType_BlockStatement;

            if(!trueStatementIsBlock) { pushGroup(r); pushNest(r); }
            pushWord(r, trueStatementIsBlock ? wordSpace() : wordLine());
            pushStatementDocument(r, node->ifStatementNode.trueStatement);
            if(!trueStatementIsBlock) { popNest(r); popGroup(r); }

            if(node->ifStatementNode.falseStatement) {
                pushWord(r, trueStatementIsBlock ? wordSpace() : wordHardBreak());

                pushTokenWord(r, node->ifStatementNode.falseStatement->startToken - 1);

                // TODO(radomski): These probaly should be something better
                bool falseStatementIsBlock =
                    node->ifStatementNode.falseStatement->type == ASTNodeType_BlockStatement ||
                    node->ifStatementNode.falseStatement->type == ASTNodeType_IfStatement;
                if(!falseStatementIsBlock) { pushGroup(r); pushNest(r); }
                pushWord(r, falseStatementIsBlock ? wordSpace() : wordLine());
                pushStatementDocument(r, node->ifStatementNode.falseStatement);
                if(!falseStatementIsBlock) { popNest(r); popGroup(r); }
            }
        } break;
        case ASTNodeType_VariableDeclarationStatement: {
            ASTNodeVariableDeclarationStatement *statement = &node->variableDeclarationStatementNode;

            pushGroup(r);
            ASTNodeVariableDeclaration *decl = &statement->variableDeclaration->variableDeclarationNode;
            pushTypeDocument(r, decl->type);

            if(decl->dataLocation != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->dataLocation);
            }

            pushWord(r, wordSpace());
            pushTokenWord(r, decl->name);

            if(statement->initialValue != 0x0) {
                pushWord(r, wordSpace());
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[statement->initialValue->startToken - 1]));

                pushTokenWord(r, statement->initialValue->startToken - 1);

                pushExpressionDocumentAssignment(r, statement->initialValue);
            }

            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_VariableDeclarationTupleStatement: {
            ASTNodeVariableDeclarationTupleStatement *statement = &node->variableDeclarationTupleStatementNode;
            assert(stringMatch(LIT_TO_STR(";"), r->tokens.tokenStrings[node->endToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushNest(r);
            pushWord(r, wordSoftline());
            ASTNode *it = statement->firstDeclaration;
            for(u32 i = 0; it != 0x0; i++, it = it->next) {
                if(it->type != ASTNodeType_None) {
                    ASTNodeVariableDeclaration *decl = &it->variableDeclarationNode;

                    pushTypeDocument(r, decl->type);
                    pushWord(r, wordSpace());

                    if(decl->dataLocation != INVALID_TOKEN_ID) {
                        pushTokenWord(r, decl->dataLocation);
                        pushWord(r, wordSpace());
                    }

                    pushTokenWord(r, decl->name);
                    if(it->next != 0x0) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[decl->name + 1]));
                        pushTokenWord(r, decl->name + 1);
                        pushWord(r, wordLine());
                    }
                } else {
                    if(it->next != 0x0) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[it->endToken + 1]));
                        pushTokenWord(r, it->endToken + 1);
                        pushWord(r, wordLine());
                    }
                }
            }
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[statement->initialValue->startToken - 2]));
            assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[statement->initialValue->startToken - 1]));
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, statement->initialValue->startToken - 2);
            pushWord(r, wordSpace());
            pushTokenWord(r, statement->initialValue->startToken - 1);
            popGroup(r);

            pushExpressionDocumentAssignment(r, statement->initialValue);
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_DoWhileStatement: {
            ASTNodeWhileStatement *statement = &node->whileStatementNode;
            assert(stringMatch(LIT_TO_STR("do"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("while"), r->tokens.tokenStrings[statement->expression->startToken - 2]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[statement->expression->startToken - 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[statement->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR(";"), r->tokens.tokenStrings[node->endToken]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);

            if(statement->body->type == ASTNodeType_BlockStatement) {
                pushWord(r, wordSpace());
                pushStatementDocument(r, statement->body);
                pushWord(r, wordSpace());
            } else {
                pushGroup(r);
                pushNest(r);
                pushWord(r, wordLine());
                pushStatementDocument(r, statement->body);
                popNest(r);
                popGroup(r);
                pushWord(r, wordLine());
            }

            pushGroup(r);
            TokenId openParen = statement->expression->startToken - 1;
            TokenId closeParen = statement->expression->endToken + 1;
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParen]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParen]));

            pushTokenWord(r, statement->expression->startToken - 2);
            pushNest(r);
            pushTokenWord(r, openParen);
            pushWord(r, wordSoftline());
            pushExpressionDocumentNoIndent(r, statement->expression);
            pushWord(r, wordSoftline());
            popNest(r);
            pushTokenWord(r, closeParen);
            popGroup(r);

            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_WhileStatement: {
            ASTNodeWhileStatement *statement = &node->whileStatementNode;

            assert(stringMatch(LIT_TO_STR("while"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[statement->expression->endToken + 1]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);

            pushTokenWord(r, node->startToken + 1);
            pushExpressionDocumentNoIndent(r, statement->expression);
            pushTokenWord(r, statement->expression->endToken + 1);

            popGroup(r);

            if(statement->body->type == ASTNodeType_BlockStatement) {
                pushWord(r, wordSpace());
                pushStatementDocument(r, statement->body);
            } else {
                pushGroup(r);
                pushNest(r);
                pushWord(r, wordLine());
                pushStatementDocument(r, statement->body);
                popNest(r);
                popGroup(r);
            }
        } break;
        case ASTNodeType_ForStatement: {
            ASTNodeForStatement *statement = &node->forStatementNode;
            assert(stringMatch(LIT_TO_STR("for"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));

            pushTokenWord(r, node->startToken);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, node->startToken + 1);
            pushGroup(r);
            pushWord(r, wordSoftline());

            TokenId secondSemicolon = node->startToken + 3;
            if(statement->variableStatement != 0x0) {
                pushStatementDocument(r, statement->variableStatement);
                secondSemicolon = statement->variableStatement->endToken + 1;
            } else {
                assert(stringMatch(LIT_TO_STR(";"), r->tokens.tokenStrings[node->startToken + 2]));
                pushTokenWord(r, node->startToken + 2);
            }

            if(statement->conditionExpression != 0x0) {
                pushWord(r, wordLine());
                pushGroup(r);
                pushExpressionDocumentNoIndent(r, statement->conditionExpression);
                popGroup(r);
                secondSemicolon = statement->conditionExpression->endToken + 1;
            }

            assert(stringMatch(LIT_TO_STR(";"), r->tokens.tokenStrings[secondSemicolon]));
            pushTokenWord(r, secondSemicolon);

            if(statement->incrementExpression != 0x0) {
                pushWord(r, wordLine());
                pushGroup(r);
                pushExpressionDocumentNoIndent(r, statement->incrementExpression);
                popGroup(r);
            }

            pushWord(r, wordSoftline());

            popGroup(r);
            popNest(r);
            pushTokenWord(r, statement->body->startToken - 1);
            popGroup(r);

            if(statement->body->type == ASTNodeType_BlockStatement) {
                pushWord(r, wordSpace());
                pushStatementDocument(r, statement->body);
            } else {
                pushGroup(r);
                pushNest(r);
                pushWord(r, wordLine());
                pushStatementDocument(r, statement->body);
                popNest(r);
                popGroup(r);
            }
        } break;
        case ASTNodeType_RevertStatement: {
            assert(stringMatch(LIT_TO_STR("revert"), r->tokens.tokenStrings[node->startToken]));
            ASTNodeRevertStatement *statement = &node->revertStatementNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            if(statement->expression->type != ASTNodeType_TupleExpression) {
                pushWord(r, wordSpace());
            }
            pushExpressionDocument(r, statement->expression);
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_EmitStatement: {
            assert(stringMatch(LIT_TO_STR("emit"), r->tokens.tokenStrings[node->startToken]));
            ASTNodeEmitStatement *statement = &node->emitStatementNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, statement->expression);
            pushTokenWord(r, node->endToken);
            popGroup(r);
        } break;
        case ASTNodeType_TryStatement: {
            pushGroup(r);

            pushGroup(r);
            ASTNodeTryStatement *statement = &node->tryStatementNode;
            assert(stringMatch(LIT_TO_STR("try"), r->tokens.tokenStrings[node->startToken]));
            pushTokenWord(r, node->startToken);

            pushWord(r, wordSpace());
            pushExpressionDocument(r, statement->expression);

            if(statement->firstReturnParameter != MISSING_ELEMENT) {
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[statement->expression->endToken + 1]));
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[statement->expression->endToken + 2]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[statement->body->startToken - 1]));

                pushWord(r, wordSpace());
                pushTokenWord(r, statement->expression->endToken + 1);
                pushWord(r, wordSpace());

                pushParametersDocument(r, statement->expression->endToken + 2, statement->firstReturnParameter);
            }

            pushWord(r, wordSpace());
            popGroup(r);

            pushStatementDocumentOpenedBlock(r, statement->body);

            for(ASTNode *catchNode = statement->firstCatch; catchNode != 0x0; catchNode = catchNode->next) {
                pushWord(r, wordSpace());

                ASTNodeCatchStatement *catch = &catchNode->catchStatementNode;
                assert(stringMatch(LIT_TO_STR("catch"), r->tokens.tokenStrings[catchNode->startToken]));

                pushTokenWord(r, catchNode->startToken);

                u32 openParenToken = catchNode->startToken + 1;
                if(catch->identifier != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, catch->identifier);
                    openParenToken = catch->identifier + 1;
                }

                if(catch->firstParameter != MISSING_ELEMENT) {
                    pushParametersDocument(r, openParenToken, catch->firstParameter);
                }

                pushWord(r, wordSpace());

                pushStatementDocumentOpenedBlock(r, catch->body);
            }

            popGroup(r);
        } break;
        case ASTNodeType_BreakStatement: {
            pushTokenWord(r, node->startToken);
            pushTokenWord(r, node->endToken); // SEMICOLON
        } break;
        case ASTNodeType_ContinueStatement: {
            pushTokenWord(r, node->startToken);
            pushTokenWord(r, node->endToken); // SEMICOLON
        } break;
        case ASTNodeType_AssemblyStatement: {
            ASTNodeAssemblyStatement *statement = &node->assemblyStatementNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken); // assembly
            pushWord(r, wordSpace());
            if(statement->isEVMAsm) {
                pushWord(r, wordText(LIT_TO_STR("\"evmasm\"")));
                pushWord(r, wordSpace());
            }

            if(statement->flags.count > 0) {
                TokenIdList *flags = &statement->flags;
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[listGetTokenId(flags, 0) - 1]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[listGetTokenId(flags, flags->count - 1) + 1]));

                pushTokenWord(r, listGetTokenId(flags, 0) - 1); // (
                for(u32 i = 0; i < flags->count; i++) {
                    TokenId flagToken = listGetTokenId(flags, i);
                    pushTokenAsStringWord(r, flagToken);
                    if(i != flags->count - 1) {
                        pushTokenWord(r, flagToken + 1); // ,
                        pushWord(r, wordSpace());
                    }
                }
                pushTokenWord(r, listGetTokenId(flags, flags->count - 1) + 1); // (
                pushWord(r, wordSpace());
            }

            pushStatementDocument(r, statement->body);
            popGroup(r);
        } break;
        case ASTNodeType_YulBlockStatement: {
            ASTNodeBlockStatement *block = &node->blockStatementNode;
            pushGroup(r);
            pushTokenWord(r, node->startToken); // {
            pushNest(r);

            pushWord(r, wordLine());

            ASTNode *statement = block->firstStatement;
            for(u32 i = 0; statement != 0x0; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, statement); }

                pushStatementDocument(r, statement);
                pushWord(r, i == 0 ? wordLine() : wordHardBreak());
            }

            popNest(r);
            pushTokenWord(r, node->endToken); // }
            popGroup(r);
        } break;
        case ASTNodeType_YulVariableDeclaration: {
            ASTNodeYulVariableDeclaration *statement = &node->yulVariableDeclarationNode;
            assert(stringMatch(LIT_TO_STR("let"), r->tokens.tokenStrings[node->startToken]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSpace());

            for(u32 i = 0; i < statement->identifiers.count; i++) {
                TokenId identifierToken = listGetTokenId(&statement->identifiers, i);
                pushTokenWord(r, identifierToken);
                if (i < statement->identifiers.count - 1) {
                    pushTokenWord(r, identifierToken + 1);
                    pushWord(r, wordSpace());
                }
            }

            if(statement->value != 0x0) {
                assert(stringMatch(LIT_TO_STR(":"), r->tokens.tokenStrings[statement->value->startToken - 2]));
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[statement->value->startToken - 1]));

                pushWord(r, wordSpace());
                pushTokenWord(r, statement->value->startToken - 2);
                pushTokenWord(r, statement->value->startToken - 1);
                pushWord(r, wordSpace());
                pushYulExpressionDocument(r, statement->value);
            }
            popGroup(r);
        } break;
        case ASTNodeType_YulVariableAssignment: {
            ASTNodeYulVariableAssignment *statement = &node->yulVariableAssignmentNode;
            pushGroup(r);

            ASTNode *it = statement->firstPath;
            for(u32 i = 0; it != 0x0; i++, it = it->next) {
                pushYulExpressionDocument(r, it);
                if (it->next != 0x0) {
                    pushTokenWord(r, it->endToken + 1); // ,
                    pushWord(r, wordSpace());
                }
            }

            if(statement->value != 0x0) {
                pushWord(r, wordSpace());
                assert(stringMatch(LIT_TO_STR(":"), r->tokens.tokenStrings[statement->value->startToken - 2]));
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[statement->value->startToken - 1]));
                pushTokenWord(r, statement->value->startToken - 2); // :=
                pushTokenWord(r, statement->value->startToken - 1); // :=
                pushWord(r, wordSpace());
                pushYulExpressionDocument(r, statement->value);
            }
            popGroup(r);
        } break;
        case ASTNodeType_YulFunctionCallExpression: {
            pushYulFunctionCallDocument(r, node);
        } break;
        case ASTNodeType_YulIfStatement: {
            ASTNodeYulIfStatement *statement = &node->yulIfStatementNode;
            pushGroup(r);

            pushTokenWord(r, node->startToken); // if
            pushWord(r, wordSpace());
            pushYulExpressionDocument(r, statement->expression);
            pushWord(r, wordSpace());
            pushStatementDocument(r, statement->body);
            popGroup(r);
        } break;
        case ASTNodeType_YulForStatement: {
            ASTNodeYulForStatement *statement = &node->yulForStatementNode;
            pushGroup(r);

            pushGroup(r);
            pushTokenWord(r, node->startToken); // for
            pushWord(r, wordSpace());
            pushStatementDocument(r, statement->variableDeclaration);
            pushWord(r, wordSpace());
            pushYulExpressionDocument(r, statement->condition);
            pushWord(r, wordSpace());
            pushStatementDocument(r, statement->increment);
            pushWord(r, wordSpace());
            popGroup(r);

            pushStatementDocument(r, statement->body);
            popGroup(r);
        } break;
        case ASTNodeType_YulFunctionDefinition: {
            ASTNodeYulFunctionDefinition *statement = &node->yulFunctionDefinitionNode;
            pushGroup(r);

            pushTokenWord(r, node->startToken); // function
            pushWord(r, wordSpace());
            pushTokenWord(r, statement->identifier); // function name

            if(statement->parameters.count > 0) {
                TokenIdList *parameters = &statement->parameters;
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[listGetTokenId(parameters, 0) - 1]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[listGetTokenId(parameters, parameters->count - 1) + 1]));

                pushGroup(r);
                pushNest(r);
                pushTokenWord(r, listGetTokenId(parameters, 0) - 1);
                pushWord(r, wordSoftline());
                for(u32 i = 0; i < parameters->count; i++) {
                    TokenId parameterToken = listGetTokenId(&statement->parameters, i);
                    pushTokenWord(r, parameterToken);
                    if (i < statement->parameters.count - 1) {
                        pushTokenWord(r, parameterToken + 1);
                        pushWord(r, wordLine());
                    }
                }
                pushWord(r, wordSoftline());
                popNest(r);
                pushTokenWord(r, listGetTokenId(parameters, parameters->count - 1) + 1);
                popGroup(r);
            } else {
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[statement->identifier + 1]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[statement->identifier + 2]));
                pushTokenWord(r, statement->identifier + 1);
                pushTokenWord(r, statement->identifier + 2);
            }

            pushWord(r, wordSpace());

            if(statement->returnParameters.count > 0) {
                TokenIdList *returnParameters = &statement->returnParameters;

                // Assuming the "->" is always two tokens before the return type list
                TokenId arrowToken1 = listGetTokenId(returnParameters, 0) - 2;
                TokenId arrowToken2 = listGetTokenId(returnParameters, 0) - 1;

                assert(stringMatch(LIT_TO_STR("-"), r->tokens.tokenStrings[arrowToken1]));
                assert(stringMatch(LIT_TO_STR(">"), r->tokens.tokenStrings[arrowToken2]));

                pushTokenWord(r, arrowToken1);
                pushTokenWord(r, arrowToken2);

                pushGroup(r);
                pushNest(r);

                pushWord(r, wordLine());
                for(u32 i = 0; i < returnParameters->count; i++) {
                    TokenId returnToken = listGetTokenId(returnParameters, i);
                    pushTokenWord(r, returnToken);
                    if (i < returnParameters->count - 1) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[returnToken + 1]));
                        pushTokenWord(r, returnToken + 1); // ,
                        pushWord(r, wordLine());
                    }
                }
                popNest(r);
                pushWord(r, wordLine());
                popGroup(r);
            }

            pushStatementDocument(r, statement->body);
            popGroup(r);
        } break;
        case ASTNodeType_YulSwitchStatement: {
            ASTNodeYulSwitchStatement *statement = &node->yulSwitchStatementNode;
            pushGroup(r);

            pushTokenWord(r, node->startToken); // switch
            pushWord(r, wordSpace());
            pushYulExpressionDocument(r, statement->expression);

            ASTNode *it = statement->firstCase;
            for(u32 i = 0; it != 0x0; i++, it = it->next) {
                if(i == 0) {
                    pushWord(r, wordHardBreak());
                } else {
                    pushWord(r, wordSpace());
                }

                pushTokenWord(r, it->yulCaseNode.literal->startToken - 1); // case
                pushWord(r, wordSpace());
                pushYulExpressionDocument(r, it->yulCaseNode.literal);
                pushWord(r, wordSpace());

                pushStatementDocumentOpenedBlock(r, it->yulCaseNode.block);
            }

            if(statement->defaultBlock != 0x0) {
                pushWord(r, wordSpace());
                pushTokenWord(r, statement->defaultBlock->yulCaseNode.block->startToken - 1); // default
                pushWord(r, wordSpace());
                pushStatementDocumentOpenedBlock(r, statement->defaultBlock->yulCaseNode.block);
            }
            popGroup(r);
        } break;
        case ASTNodeType_YulLeaveStatement: {
            pushTokenWord(r, node->startToken);
        } break;
        case ASTNodeType_YulBreakStatement: {
            pushTokenWord(r, node->startToken);
        } break;
        case ASTNodeType_YulContinueStatement: {
            pushTokenWord(r, node->startToken);
        } break;
        default: {
            javascriptPrintString("Unreachable, unhandled statement type = ");
            javascriptPrintNumber(node->type);
            assert(0);
        }
    }
}

static void
pushStatementDocumentOpenedBlock(Render *r, ASTNode *node) {
    switch(node->type) {
        case ASTNodeType_BlockStatement: {
            ASTNodeBlockStatement *block = &node->blockStatementNode;

            pushTokenWord(r, node->startToken);
            pushNest(r);

            if(block->firstStatement == 0) {
                pushWord(r, wordHardBreak());
            }

            ASTNode *statement = block->firstStatement;
            for(u32 i = 0; statement != 0x0; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, statement); }
                else { pushWord(r, wordHardBreak()); }

                pushStatementDocument(r, statement);
                pushWord(r, wordHardBreak());
            }

            popNest(r);
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_YulBlockStatement: {
            ASTNodeBlockStatement *block = &node->blockStatementNode;
            pushGroup(r);
            pushTokenWord(r, node->startToken); // {
            pushNest(r);

            ASTNode *statement = block->firstStatement;
            for(u32 i = 0; statement != 0x0; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, statement); }
                else { pushWord(r, wordHardBreak()); }

                pushStatementDocument(r, statement);
                pushWord(r, i == 0 ? wordLine() : wordHardBreak());
            }

            popNest(r);
            pushTokenWord(r, node->endToken); // }
            popGroup(r);
        } break;
        default: {
            pushStatementDocument(r, node);
        }
    }
}

static void
pushInheritanceSpecifierDocument(Render *r, ASTNode *node) {
    ASTNodeInheritanceSpecifier *inheritance = &node->inheritanceSpecifierNode;
    pushTypeDocument(r, inheritance->identifier);
    if(inheritance->firstArgumentExpression != MISSING_ELEMENT) {
        pushCallArgumentListDocument(r, inheritance->identifier->endToken + 1,
                                     inheritance->firstArgumentExpression,
                                     &inheritance->argumentsName);
    }
}

static void
pushOverridesDocument(Render *r, TokenId overrideToken, ASTNode *firstOverride, Word whitespace) {
    if(overrideToken != INVALID_TOKEN_ID) {
        pushWord(r, whitespace);
        assert(stringMatch(LIT_TO_STR("override"), r->tokens.tokenStrings[overrideToken]));
        pushTokenWord(r, overrideToken);
        if(firstOverride) {
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, firstOverride->startToken - 1);
            pushWord(r, wordSoftline());

            ASTNode *lastOverride = firstOverride;
            for(ASTNode *override = firstOverride; override != 0x0; override = override->next) {
                pushTypeDocument(r, override);
                if(override->next != 0x0) {
                    pushTokenWord(r, override->endToken + 1);
                    pushWord(r, wordLine());
                }
                lastOverride = override;
            }

            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, lastOverride->endToken + 1);
            popGroup(r);

            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[firstOverride->startToken - 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[lastOverride->endToken + 1]));
        }
    }
}

static void
pushParametersDocument(Render *r, TokenId openParenToken, ASTNode *first) {
    TokenId closeParenToken = openParenToken + 1;

    pushGroup(r);
    pushTokenWord(r, openParenToken);
    pushNest(r);

    ASTNode *it = first;
    for(u32 i = 0; it != 0x0; i++, it = it->next) {
        if(i == 0) { pushWord(r, wordSoftline()); }
        ASTNodeVariableDeclaration *decl = &it->variableDeclarationNode;
        pushTypeDocument(r, decl->type);

        if(decl->dataLocation != INVALID_TOKEN_ID) {
            pushWord(r, wordSpace());
            pushTokenWord(r, decl->dataLocation);
        }

        if(decl->name != INVALID_TOKEN_ID) {
            pushWord(r, wordSpace());
            pushTokenWord(r, decl->name);
        }

        if(it-> next != 0x0) {
            pushTokenWord(r, it->next->variableDeclarationNode.type->startToken - 1);
            pushWord(r, wordLine());
        } else {
            pushWord(r, wordSoftline()); 
        }

        closeParenToken = it->endToken + 1;
    }

    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));
    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParenToken]));

    popNest(r);
    pushTokenWord(r, closeParenToken);
    popGroup(r);
}

static void
pushModifierInvocations(Render *r, ASTNode *first) {
    for(ASTNode *it = first; it != 0x0; it = it->next) {
        pushGroup(r);

        ASTNodeModifierInvocation *invocation = &it->modifierInvocationNode;
        pushTypeDocument(r, invocation->identifier);
        if(invocation->firstArgumentExpression != MISSING_ELEMENT) {
            TokenIdList *names = &invocation->argumentsName;

            TokenId startingToken = invocation->identifier->endToken + 1;
            pushCallArgumentListDocument(r, startingToken, invocation->firstArgumentExpression, names);
        }
        popGroup(r);

        if(it->next) {
            pushWord(r, wordLine());
        }
    }
}

static void
pushMemberDocument(Render *r, ASTNode *member) {
    switch(member->type) {
        case ASTNodeType_Pragma: {
            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, member->startToken + 1);
            pushWord(r, wordSpace());

            ASTNodePragma *pragma = &member->pragmaNode;
            TokenId firstTokenId = listGetTokenId(&pragma->following, 0);
            TokenId lastTokenId = listGetTokenId(&pragma->following, pragma->following.count - 1);
            String first = getTokenString(r->tokens, firstTokenId);
            String last = getTokenString(r->tokens, lastTokenId);
            String string = {
                .data = first.data,
                .size = (u32)(last.data - first.data) + last.size,
            };

            pushWord(r, wordText(string));
            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_Import: {
            assert(stringMatch(LIT_TO_STR("import"), r->tokens.tokenStrings[member->startToken]));

            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());

            ASTNodeImport *import = &member->importNode;
            if(import->symbols.count > 0) {
                assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 1]));
                pushTokenWord(r, member->startToken + 1);
                pushWord(r, wordLine());

                pushNest(r);
                for(u32 i = 0; i < import->symbols.count; i++) {
                    TokenId symbol = listGetTokenId(&import->symbols, i);
                    TokenId alias = listGetTokenId(&import->symbolAliases, i);

                    pushTokenWord(r, symbol);
                    TokenId lastToken = symbol + 1;

                    if(alias != INVALID_TOKEN_ID) {
                        pushWord(r, wordSpace());
                        pushTokenWord(r, alias - 1);
                        pushWord(r, wordSpace());
                        pushTokenWord(r, alias);
                        lastToken = alias + 1;
                    }

                    bool isLastElement = i == import->symbols.count - 1;
                    if(!isLastElement) {
                        pushTokenWord(r, lastToken);
                    }
                    pushWord(r, wordLine());
                }
                popNest(r);

                assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[import->pathTokenId - 2]));
                assert(stringMatch(LIT_TO_STR("from"), r->tokens.tokenStrings[import->pathTokenId - 1]));
                pushTokenWord(r, import->pathTokenId - 2);
                pushWord(r, wordSpace());
                pushTokenWord(r, import->pathTokenId - 1);
                pushWord(r, wordSpace());
            }

            pushTokenAsStringWord(r, import->pathTokenId);
            if(import->unitAliasTokenId != INVALID_TOKEN_ID) {
                assert(stringMatch(LIT_TO_STR("as"), r->tokens.tokenStrings[import->unitAliasTokenId - 1]));

                pushWord(r, wordSpace());
                pushTokenWord(r, import->unitAliasTokenId - 1);
                pushWord(r, wordSpace());
                pushTokenWord(r, import->unitAliasTokenId);
            }

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_Using: {
            assert(stringMatch(LIT_TO_STR("using"), r->tokens.tokenStrings[member->startToken]));
            ASTNodeUsing *using = &member->usingNode;

            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());

            TokenId forToken = INVALID_TOKEN_ID;
            if(using->onLibrary) {
                assert(using->firstIdentifier->next == 0x0);
                pushTypeDocument(r, using->firstIdentifier);
                pushWord(r, wordSpace());
                forToken = using->firstIdentifier->endToken + 1;
            } else {
                pushTokenWord(r, member->startToken + 1);
                pushWord(r, wordSpace());

                TokenId endToken = member->startToken + 1;

                ASTNode *it = using->firstIdentifier;
                for(u32 i = 0; it != 0x0; i++, it = it->next) {
                    endToken = it->endToken;
                    pushTypeDocument(r, it);

                    u16 operator = listGetU16(&using->operators, i);
                    if(operator) {
                        pushWord(r, wordSpace());
                        pushTokenWord(r, it->endToken + 1);
                        pushWord(r, wordSpace());
                        pushWord(r, wordText(tokenTypeToString(operator)));

                        endToken = it->endToken + 2;
                    }

                    if(it->next != 0x0) {
                        pushTokenWord(r, it->next->startToken - 1);
                    }

                    pushWord(r, wordSpace());
                }
                pushTokenWord(r, endToken + 1);
                pushWord(r, wordSpace());
                forToken = endToken + 2;
            }

            if(using->forType != 0x0) {
                pushTokenWord(r, using->forType->startToken - 1);
                pushWord(r, wordSpace());
                pushTypeDocument(r, using->forType);
            } else {
                pushTokenWord(r, forToken);
                pushWord(r, wordSpace());
                pushTokenWord(r, forToken + 1);
            }

            if(using->global != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, using->global);
            }

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_EnumDefinition: {
            ASTNodeEnum *enumNode = &member->enumNode;

            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, enumNode->nameTokenId);
            pushWord(r, wordSpace());
            pushTokenWord(r, enumNode->nameTokenId + 1);

            pushNest(r);
            pushWord(r, wordLine());
            for(u32 i = 0; i < enumNode->values.count; i++) {
                TokenId value = listGetTokenId(&enumNode->values, i);
                pushTokenWord(r, value);
                if(i != enumNode->values.count - 1) {
                    pushTokenWord(r, value + 1);
                }

                pushWord(r, wordHardBreak());
            }
            popNest(r);

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_Struct: {
            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 2]));
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));

            pushGroup(r);

            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, member->structNode.nameTokenId);
            pushWord(r, wordSpace());
            pushNest(r);
            pushTokenWord(r, member->startToken + 2);
            pushWord(r, wordLine());

            for(ASTNode *it = member->structNode.firstMember; it != 0x0; it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->variableDeclarationNode;
                preserveHardBreaksIntoDocument(r, it);

                pushGroup(r);
                pushTypeDocument(r, decl->type);
                popGroup(r);
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->name);
                pushTokenWord(r, decl->name + 1);
                pushWord(r, wordHardBreak());
            }

            popNest(r);

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_Error: {
            assert(stringMatch(LIT_TO_STR("error"), r->tokens.tokenStrings[member->startToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[member->endToken - 1]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[member->startToken + 2]));

            ASTNodeEvent *event = &member->eventNode;

            pushGroup(r);

            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, event->identifier);
            pushTokenWord(r, member->startToken + 2);

            pushNest(r);
            pushWord(r, wordSoftline());
            ASTNode *it = member->eventNode.firstParameter;
            for(u32 i = 0; it != 0x0; i++, it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->variableDeclarationNode;

                pushTypeDocument(r, decl->type);

                if(decl->dataLocation != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->dataLocation);
                }

                if(decl->name != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->name);
                }

                if(it->next != 0x0) {
                    pushTokenWord(r, it->next->variableDeclarationNode.type->startToken - 1);
                    pushWord(r, wordLine());
                }
            }
            pushWord(r, wordSoftline());
            popNest(r);

            pushTokenWord(r, member->endToken - 1);
            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_Event: {
            assert(stringMatch(LIT_TO_STR("event"), r->tokens.tokenStrings[member->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[member->startToken + 2]));

            ASTNodeEvent *event = &member->eventNode;
            pushGroup(r);

            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, event->identifier);
            pushTokenWord(r, member->startToken + 2);

            pushNest(r);
            pushWord(r, wordSoftline());
            ASTNode *it = member->errorNode.firstParameter;
            for(u32 i = 0; it != 0x0; i++, it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->variableDeclarationNode;

                pushTypeDocument(r, decl->type);

                if(decl->dataLocation != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->dataLocation);
                }

                if(decl->name != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->name);
                }

                if(it->next != 0x0) {
                    pushTokenWord(r, it->next->variableDeclarationNode.type->startToken - 1);
                    pushWord(r, wordLine());
                }
            }
            pushWord(r, wordSoftline());
            popNest(r);

            TokenId closeParam = event->anonymous ? member->endToken - 2 : member->endToken - 1;
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParam]));
            pushTokenWord(r, closeParam);

            if(event->anonymous) {
                pushWord(r, wordSpace());
                pushTokenWord(r, member->endToken - 1);
            }


            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_Typedef: {
            ASTNodeTypedef *typedefNode = &member->typedefNode;

            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, typedefNode->identifier);
            pushWord(r, wordSpace());
            pushTokenWord(r, typedefNode->identifier + 1);
            pushWord(r, wordSpace());
            pushTypeDocument(r, typedefNode->type);

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_ConstVariable: {
            ASTNodeConstVariable *constNode = &member->constVariableNode;
            assert(stringMatch(LIT_TO_STR("constant"), r->tokens.tokenStrings[constNode->identifier - 1]));
            assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[constNode->identifier + 1]));

            pushGroup(r);
            pushTypeDocument(r, constNode->type);
            pushWord(r, wordSpace());

            pushTokenWord(r, constNode->identifier - 1);
            pushWord(r, wordSpace());
            pushTokenWord(r, constNode->identifier);
            pushWord(r, wordSpace());
            pushTokenWord(r, constNode->identifier + 1);

            pushExpressionDocumentAssignment(r, constNode->expression);

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_StateVariableDeclaration: {
            ASTNodeConstVariable *decl = &member->constVariableNode;

            pushGroup(r);
            pushTypeDocument(r, decl->type);

            pushGroup(r);
            if(decl->visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->visibility);
            }
            if(decl->mutability != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->mutability);
            }

            pushOverridesDocument(r, decl->override, decl->firstOverride, wordSpace());

            pushWord(r, wordSpace());
            pushTokenWord(r, decl->identifier);
            popGroup(r);

            if(decl->expression) {
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[decl->identifier + 1]));
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->identifier + 1);

                pushExpressionDocumentAssignment(r, decl->expression);
            }

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_FallbackFunction:
        case ASTNodeType_ReceiveFunction:
        case ASTNodeType_FunctionDefinition: {
            ASTNodeFunctionDefinition *function = &member->functionDefinitionNode;

            pushGroup(r);
            TokenId openParenToken = member->startToken + 1;
            if(member->type == ASTNodeType_FallbackFunction) {
                assert(stringMatch(LIT_TO_STR("fallback"), r->tokens.tokenStrings[member->startToken]));
                pushTokenWord(r, member->startToken);
            } else if(member->type == ASTNodeType_ReceiveFunction) {
                assert(stringMatch(LIT_TO_STR("receive"), r->tokens.tokenStrings[member->startToken]));
                pushTokenWord(r, member->startToken);
            } else {
                assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[member->startToken]));
                pushTokenWord(r, member->startToken);

                if(function->name != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, function->name);
                    openParenToken = function->name + 1;
                }
            }

            pushParametersDocument(r, openParenToken, function->firstParameter);

            pushGroup(r);
            pushNest(r);
            if(function->visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, function->visibility);
            }
            if(function->stateMutability != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, function->stateMutability);
            }

            if(function->virtual != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, function->virtual);
            }

            pushOverridesDocument(r, function->override, function->firstOverride, wordLine());

            if(function->firstModifier) {
                pushWord(r, wordLine());
                pushModifierInvocations(r, function->firstModifier);
            }

            if(function->firstReturnParameter) {
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[function->firstReturnParameter->startToken - 2]));

                pushWord(r, wordLine());
                pushGroup(r);
                pushTokenWord(r, function->firstReturnParameter->startToken - 2);
                pushWord(r, wordSpace());

                pushParametersDocument(r, function->firstReturnParameter->startToken - 1, function->firstReturnParameter);
                popGroup(r);
            }

            if(function->body != 0x0) {
                pushWord(r, wordLine());
                popNest(r);
                popGroup(r);
                pushStatementDocument(r, function->body);
                popGroup(r);
            } else {
                popNest(r);
                pushTokenWord(r, member->endToken);
                popGroup(r);
                popGroup(r);
            }

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_ConstructorDefinition: {
            ASTNodeConstructorDefinition *constructor = &member->constructorDefinitionNode;
            assert(stringMatch(LIT_TO_STR("constructor"), r->tokens.tokenStrings[member->startToken]));

            pushGroup(r);

            pushTokenWord(r, member->startToken);

            TokenId openParenToken = member->startToken + 1;
            pushParametersDocument(r, openParenToken, constructor->firstParameter);

            pushGroup(r);
            pushNest(r);
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));

            if(constructor->firstModifier != 0x0) {
                pushWord(r, wordLine());
                pushModifierInvocations(r, constructor->firstModifier);
            }

            if(constructor->visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, constructor->visibility);
            }

            if(constructor->stateMutability != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, constructor->stateMutability);
            }
            popNest(r);

            if(constructor->body != 0x0) {
                pushWord(r, wordLine());
                popGroup(r);
                popGroup(r);

                pushStatementDocument(r, constructor->body);
                pushWord(r, wordHardBreak());
            } else {
                pushTokenWord(r, member->endToken);
                popGroup(r);
                popGroup(r);

                pushWord(r, wordHardBreak());
            }
        } break;
        case ASTNodeType_ModifierDefinition: {
            ASTNodeFunctionDefinition *modifier = &member->functionDefinitionNode;
            assert(stringMatch(LIT_TO_STR("modifier"), r->tokens.tokenStrings[member->startToken]));

            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, modifier->name);

            if(modifier->firstParameter != MISSING_ELEMENT) {
                TokenId openParenToken = modifier->name + 1;
                pushParametersDocument(r, openParenToken, modifier->firstParameter);
            }

            if(modifier->virtual != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, modifier->virtual);
            }

            pushOverridesDocument(r, modifier->override, modifier->firstOverride, wordLine());

            popGroup(r);
            if(modifier->body != 0x0) {
                pushWord(r, wordSpace());
                pushStatementDocument(r, modifier->body);
            } else {
                pushTokenWord(r, member->endToken);
            }

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_LibraryDefinition:
        case ASTNodeType_ContractDefinition:
        case ASTNodeType_InterfaceDefinition:
        case ASTNodeType_AbstractContractDefinition: {
            ASTNodeContractDefinition *contract = &member->contractDefinitionNode;

            pushGroup(r);
            pushGroup(r);
            pushTokenWord(r, member->startToken);
            switch(member->type) {
                case ASTNodeType_LibraryDefinition: {
                    assert(stringMatch(LIT_TO_STR("library"), r->tokens.tokenStrings[member->startToken]));
                } break;
                case ASTNodeType_InterfaceDefinition: {
                    assert(stringMatch(LIT_TO_STR("interface"), r->tokens.tokenStrings[member->startToken]));
                } break;
                case ASTNodeType_ContractDefinition: {
                    assert(stringMatch(LIT_TO_STR("contract"), r->tokens.tokenStrings[member->startToken]));
                } break;
                case ASTNodeType_AbstractContractDefinition: {
                    assert(stringMatch(LIT_TO_STR("abstract"), r->tokens.tokenStrings[member->startToken]));
                    assert(stringMatch(LIT_TO_STR("contract"), r->tokens.tokenStrings[member->startToken + 1]));
                    pushWord(r, wordSpace());
                    pushTokenWord(r, member->startToken + 1);
                }
            }
            pushWord(r, wordSpace());

            pushTokenWord(r, contract->name);
            pushWord(r, wordSpace());
            popGroup(r);

            if(contract->firstBaseContract != 0x0) {
                pushGroup(r);
                pushNest(r);
                assert(stringMatch(LIT_TO_STR("is"), r->tokens.tokenStrings[contract->name + 1]));
                pushTokenWord(r, contract->name + 1);
                pushWord(r, wordLine());

                for(
                    ASTNode *baseContract = contract->firstBaseContract;
                    baseContract != 0x0;
                    baseContract = baseContract->next
                ) {
                    pushInheritanceSpecifierDocument(r, baseContract);
                    if(baseContract->next != 0x0) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[baseContract->endToken + 1]));
                        pushTokenWord(r, baseContract->endToken + 1);
                    }
                    pushWord(r, wordLine());
                }
                popNest(r);
                popGroup(r);
            }

            pushGroup(r);
            u32 openParenToken = contract->firstElement != 0x0
                ? contract->firstElement->startToken - 1
                : member->endToken - 1;

            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[openParenToken]));
            pushTokenWord(r, openParenToken);
            pushNest(r);
            pushWord(r, wordLine());

            ASTNode *element = contract->firstElement;
            for(u32 i = 0; element != 0x0; i++, element = element->next) {
                if(i > 0) {
                    preserveHardBreaksIntoDocument(r, element);
                }
                pushMemberDocument(r, element);
            }
            popNest(r);

            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));
            pushTokenWord(r, member->endToken);
            popGroup(r);
            popGroup(r);
            pushWord(r, wordHardBreak());
        } break;
        default: {
            assert(0);
        } break;
    }
}

static String
wordTypeToString(WordType type) {
    switch(type) {
        case WordType_Text: return LIT_TO_STR("Text");
        case WordType_Line: return LIT_TO_STR("Line");
        case WordType_Space: return LIT_TO_STR("Space");
        case WordType_CommentStartSpace: return LIT_TO_STR("CommentStartSpace");
        case WordType_HardBreak: return LIT_TO_STR("HardBreak");
        case WordType_GroupPush: return LIT_TO_STR("GroupPush");
        case WordType_GroupPop: return LIT_TO_STR("GroupPop");
        case WordType_Count: return LIT_TO_STR("Count");
    }
}

static String
stringEscape(Arena *arena, String string) {
    String escaped = stringPush(arena, string.size * 2);
    u32 newSize = 0;
    for(u32 i = 0; i < string.size; i++) {
        char c = string.data[i];
        if(c == '"' || c == '\\') {
            escaped.data[newSize] = '\\';
            newSize += 1;
        }
        escaped.data[newSize] = c;
        newSize += 1;
    }

    escaped.size = newSize;
    return escaped;
}

static void
openDebugElement(ByteConcatenator *c, String label) {
    String prefixStart = LIT_TO_STR("{\"text\": \"");
    String prefixEnd = LIT_TO_STR("\",");
    byteConcatenatorPushString(c, prefixStart);

    String labelEscaped = stringEscape(c->arena, label);
    byteConcatenatorPushString(c, labelEscaped);

    byteConcatenatorPushString(c, prefixEnd);
}

static void
openDebugElementChildren(ByteConcatenator *c) {
    String prefix = LIT_TO_STR("\"children\": [");
    byteConcatenatorPushString(c, prefix);
}

static void
closeDebugElementChildren(ByteConcatenator *c) {
    String suffix = LIT_TO_STR("],");
    byteConcatenatorPushString(c, suffix);
}

static void
closeDebugElement(ByteConcatenator *c) {
    String suffix = LIT_TO_STR("},");
    byteConcatenatorPushString(c, suffix);
}

static void
dumpDocument(Render *r) {
    const char *flag = getenv("DDOC");
    if(flag == NULL) {
        return;
    }

    FILE *output = fopen("document.csv", "wb");
    fprintf(output, "Type,Text Size,Text\n");
    for(s32 i = 0; i < r->wordCount; i++) {
        Word *word = &r->words[i];
        fprintf(output, "%.*s,%u,%.*s\n",
                (int)wordTypeToString(word->type).size, wordTypeToString(word->type).data,
                word->textSize,
                (int)word->textSize, word->text);
    }
    fclose(output);
}

static Render
createRender(Arena *arena, String originalSource, TokenizeResult tokens) {
    u32 scratchBufferCapacity = MAX(64 * Kilobyte, (originalSource.size / 5));
    u32 wordCount = (originalSource.size / 4) * 3;
    Render render = {
        .indentSize = 4,
        .writer = {
            .data = arrayPush(arena, u8, originalSource.size * 3),
            .size = 0,
            .capacity = originalSource.size * 4 ,
        },
        .tokens = tokens,
        .sourceBaseAddress = originalSource.data,

        // NOTE(radomski): This makes read and writes to (-1) possible
        .words = arrayPush(arena, Word, wordCount + 1) + 1,
        .wordCount = 0,
        .wordCapacity = wordCount,

        .scratchBufferCapacity = scratchBufferCapacity,
        .scratchBufferSize = 0,
        .scratchBuffer = arrayPush(arena, u8, scratchBufferCapacity),

        .trailingCount = 0,
    };

    return render;
}

static void
buildDocument(Render *r, ASTNode *tree, String originalSource, TokenizeResult tokens) {
    u32 startOfTokens = tokens.tokenStrings[tree->startToken].data - originalSource.data;

    pushGroup(r);
    pushCommentsInRange(r, 0, startOfTokens);

    ASTNodeSourceUnit *sourceUnit = &tree->sourceUnitNode;
    ASTNode *child = sourceUnit->firstChild;
    for(u32 i = 0; child != 0x0; i++, child = child->next) {
        if(i != 0) preserveHardBreaksIntoDocument(r, child);
        pushMemberDocument(r, child);
    }
    popGroup(r);
}
