typedef struct Writer {
    u8 *data;
    u32 size;
    u32 capacity;

    u32 indentSize;
    u32 indentCount;
    u32 lineSize;
} Writer;

typedef enum WordType {
    WordType_Text,
    WordType_Line,
    WordType_Space,
    WordType_CommentStartSpace,
    WordType_HardBreak,
    WordType_GroupPush,
    WordType_GroupPop,
    WordType_NestPush,
    WordType_NestPop,
    WordType_Count,
} WordType;

typedef struct Word {
    WordType type;
    String text;
} Word;

typedef struct Render {
    Writer *writer;
    TokenizeResult tokens;
    u8 *sourceBaseAddress;

    u32 scratchBufferSize;
    u32 scratchBufferCapacity;
    u8 *scratchBuffer;

    Word *words;
    s32 wordCount;
    u32 wordCapacity;

    u32 trailingCount;
    Word trailing[8192];
} Render;

static void
commitIndent(Writer *w) {
    u32 spaceCount = w->indentSize * w->indentCount;
    memset(w->data + w->size, ' ', spaceCount);
    w->size += spaceCount;
    w->lineSize = spaceCount;
}

static void
writeString(Writer *w, String str) {
    if(w->lineSize == 0) {
        commitIndent(w);
    }

    memcpy(w->data + w->size, str.data, str.size);
    w->size += str.size;
    w->lineSize += str.size;
}

static void
finishLine(Writer *w) {
    w->data[w->size++] = '\n';
    w->lineSize = 0;
}

static void
setIndent(Writer *w, u32 indentCount) {
    w->indentCount = indentCount;
}

static void
pushTrailing(Render *r, Word w) {
    assert(r->trailingCount < ARRAY_LENGTH(r->trailing));
    r->trailing[r->trailingCount++] = w;
}

static void
flushTrailing(Render *r) {
    for(u32 i = 0; i < r->trailingCount; i++) {
        Word *out = r->words + r->wordCount++;
        *out = r->trailing[i];
    }

    r->trailingCount = 0;
}

static bool
fits(Render *r, Word *words, s32 count, u32 remainingWidth) {
    u32 width = 0;
    s32 i = 0;

    s32 groupStack = 0;

    for (; i < count && groupStack >= 0 && width <= remainingWidth; i++) {
        Word *word = &words[i];

        if(word->type == WordType_GroupPush) {
            groupStack += 1;
        } else if(word->type == WordType_GroupPop) {
            groupStack -= 1;
        }

        width += word->text.size;
    }

    for (;
         i < count &&
         width <= remainingWidth &&
         words[i].type != WordType_HardBreak &&
         words[i].type != WordType_CommentStartSpace &&
         words[i].type != WordType_Line;
         i++) {
        Word *word = &words[i];
        width += word->text.size;
    }

    return width <= remainingWidth;
}

static u32
renderDocumentWord(Render *r, Word *word, u32 nest, bool flatMode) {
    Writer *w = r->writer;
    setIndent(w, nest);

    u32 nestActive = flatMode ? 0 : 1;
    switch(word->type) {
        case WordType_CommentStartSpace:
        case WordType_Space:
        case WordType_Text: { writeString(r->writer, word->text); } break;
        case WordType_Line: {
            if(flatMode) { writeString(r->writer, word->text); }
            else         { finishLine(w); }
        } break;
        case WordType_HardBreak: { finishLine(w); } break;
        case WordType_NestPush:  { nest += nestActive; } break;
        case WordType_NestPop:   { nest -= nestActive; } break;
        default:                 { assert(false); }
    }

    return nest;
}

static u32
renderGroup(Render *r, Word *words, s32 count, u32 nest) {
    if (count <= 0) return 0;

    Writer *w = r->writer;

    u32 startLineSize = w->lineSize;
    bool hasFlatMode = false;
    bool flatMode = false;

    u32 i = 0;

    for(;i < count && words[i].type != WordType_GroupPop; i++) {
        Word *word = &words[i];

        if (word->type == WordType_GroupPush) {
            i += renderGroup(r, words + (i + 1), count - (i + 1), nest);
        } else {
            if(!hasFlatMode) {
                s32 remainingWidth = 120 - MIN(120, MAX(startLineSize, w->indentSize * nest));
                flatMode = fits(r, words, count, remainingWidth);
                hasFlatMode = true;
            }

            nest = renderDocumentWord(r, word, nest, flatMode);
        }
    }

    // NOTE(radomski): Consume that last GroupPop
    return i + 1;
}

static void
renderDocument(Render *r) {
    flushTrailing(r);

    u32 nest = 0;
    renderGroup(r, r->words, r->wordCount, nest);
}

static void
pushGroup(Render *r) {
    Word w = { .type = WordType_GroupPush };
    r->words[r->wordCount++] = w;
}

static void
popGroup(Render *r) {
    Word w = { .type = WordType_GroupPop };
    r->words[r->wordCount++] = w;
}

static void
pushNest(Render *r) {
    Word w = { .type = WordType_NestPush };
    r->words[r->wordCount++] = w;
}

static void
popNest(Render *r) {
    Word w = { .type = WordType_NestPop };
    r->words[r->wordCount++] = w;
}

static void
addNest(Render *r, s32 nest) {
    if(nest == 0) {}
    else if(nest > 0) { pushNest(r); }
    else if(nest < 0) { popNest(r); }
}

static bool
isWordWhitespace(Word w) {
    return (
        w.type == WordType_Space ||
        w.type == WordType_CommentStartSpace ||
        w.type == WordType_Line ||
        w.type == WordType_HardBreak
    );
}

static void
pushWord(Render *r, Word w) {
    bool skipPassedWord =
        r->trailingCount > 0 &&
        r->trailing[r->trailingCount - 1].type == WordType_HardBreak &&
        isWordWhitespace(w);

    flushTrailing(r);
    if(!skipPassedWord) {
        r->words[r->wordCount++] = w;
    }
}

static Word
wordText(String text) {
    return (Word) { .type = WordType_Text, .text = text };
}

static Word
wordHardBreak(void) {
    return (Word) { .type = WordType_HardBreak, .text = { .data = 0x0, .size = 0xffff } };
}

static Word
wordSpace(void) {
    return (Word) { .type = WordType_Space, .text = LIT_TO_STR(" ") };
}

static Word
wordCommentStartSpace(void) {
    return (Word) { .type = WordType_CommentStartSpace, .text = LIT_TO_STR(" ") };
}

static Word
wordSoftline(void) {
    return (Word) { .type = WordType_Line, .text = LIT_TO_STR("") };
}

static Word
wordLine(void) {
    return (Word) { .type = WordType_Line, .text = LIT_TO_STR(" ") };
}

static void
preserveHardBreaksIntoDocument(Render *r, ASTNode *node) {
    flushTrailing(r);

    u32 tokenIndex = node->startToken;

    String inBetween = { 0 };
    if(tokenIndex == 0) {
        inBetween.data = r->sourceBaseAddress;
        inBetween.size = r->tokens.tokenStrings[tokenIndex].data - r->sourceBaseAddress;
    } else {
        String token = r->tokens.tokenStrings[tokenIndex];
        String previousToken = r->tokens.tokenStrings[tokenIndex - 1];

        inBetween.data = previousToken.data + previousToken.size;
        inBetween.size = token.data >= inBetween.data ? token.data - inBetween.data : 0;
    }

    // TODO(radomski): This shouln't be here
    for(u32 i = 0; inBetween.size > 0 && i < inBetween.size - 1; i++) {
        if(inBetween.data[i] == '/' && inBetween.data[i + 1] == '*') {
            return;
        }
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

enum CommentType {
    CommentType_None,
    CommentType_SingleLine,
    CommentType_MultiLine,
    CommentType_StarAligned
};

static void
pushCommentsInRange(Render *r, u32 startOffset, u32 endOffset) {
    String input = (String) {
        .data = r->sourceBaseAddress + startOffset,
        .size = endOffset - startOffset,
    };

    if(input.size >= 2) {
        u32 cursor = 0;
        u32 previousCommentType = CommentType_None;

        while(true) {
            u32 finished = true;
            for(u32 i = cursor; i < input.size - 1; i++) {
                u32 commentStart = i;
                u32 commentEnd = i;
                String comment = (String) { .data = 0x0, .size = 0 };
                u32 commentType = CommentType_None;

                if(input.data[i] == '/' && input.data[i + 1] == '/') {
                    commentType = CommentType_SingleLine;

                    u32 newlineIndex = i;
                    for(; newlineIndex < input.size && input.data[newlineIndex] != '\n'; newlineIndex += 1) {}
                    comment = (String) {
                        .data = input.data + commentStart,
                        .size = newlineIndex - commentStart
                    };
                    commentEnd = newlineIndex + 1;
                } else if(input.data[i] == '/' && input.data[i + 1] == '*') {
                    commentEnd = i + 2;
                    bool isStarAligned = true;
                    bool checkStarAlignment = false;

                    while(commentEnd < input.size - 1 && (input.data[commentEnd] != '*' || input.data[commentEnd + 1] != '/')) {
                        if(input.data[commentEnd] == '\n') {
                            checkStarAlignment = true;
                        }

                        if(checkStarAlignment && !(isWhitespace(input.data[commentEnd]) || input.data[commentEnd] == '*')) {
                            isStarAligned = false;
                        }

                        if(checkStarAlignment && input.data[commentEnd] == '*') {
                            checkStarAlignment = false;
                        }

                        commentEnd += 1;
                    }
                    commentEnd += 2;

                    commentType = isStarAligned ? CommentType_StarAligned : CommentType_MultiLine;
                    comment = (String) {
                        .data = input.data + commentStart,
                        .size = commentEnd - commentStart
                    };
                }

                if(comment.size > 0) {
                    assert(commentType != CommentType_None);

                    u32 preceedingNewlines = 0;
                    for(u32 j = cursor; j < commentStart; j++) { preceedingNewlines += input.data[j] == '\n'; }

                    if(cursor == 0 && preceedingNewlines >= 2) {
                        pushTrailing(r, wordHardBreak());
                        pushTrailing(r, wordHardBreak());
                    } else if(preceedingNewlines > 0) {
                        pushTrailing(r, wordHardBreak());
                    } else if(cursor == 0) {
                        pushTrailing(r, wordCommentStartSpace());
                    } else if(commentType != CommentType_SingleLine && previousCommentType != CommentType_SingleLine) {
                        pushTrailing(r, wordSpace());
                    }

                    cursor = commentEnd;
                    i = cursor - 1;

                    // Write comment out
                    if(commentType == CommentType_SingleLine) {
                        previousCommentType = commentType;
                        pushTrailing(r, wordText(stringTrim(comment)));
                        pushTrailing(r, wordHardBreak());
                    } else if(commentType == CommentType_StarAligned) {
                        previousCommentType = commentType;
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

                        u8 *head = comment.data + comment.size;
                        while(isWhitespace(*head)) {
                            if(*head == '\n') {
                                pushTrailing(r, wordHardBreak());
                                break;
                            }
                            head++;
                        }
                    } else {
                        previousCommentType = commentType;
                        pushTrailing(r, wordText(stringTrim(comment)));

                        u8 *head = comment.data + comment.size;
                        while(isWhitespace(*head)) {
                            if(*head == '\n') {
                                pushTrailing(r, wordHardBreak());
                                break;
                            }
                            head++;
                        }
                    }

                    finished = false;
                }
            }

            if(finished) {
                break;
            }
        }

        u32 preceedingNewlines = 0;
        for(u32 i = cursor; i < input.size; i++) {
            preceedingNewlines += input.data[i] == '\n';
        }

        if(cursor != 0 && preceedingNewlines >= 2) {
            pushTrailing(r, wordHardBreak());
        }
    }
}

static void
pushCommentsAfterToken(Render *r, TokenId token) {
    String current = getTokenString(r->tokens, token);

    assert(token < r->tokens.count);
    String next = getTokenString(r->tokens, token + 1);

    u32 startOffset = (current.data - r->sourceBaseAddress) + current.size;
    u32 endOffset = next.data ? next.data - r->sourceBaseAddress : r->tokens.sourceSize;

    pushCommentsInRange(r, startOffset, endOffset);
}

static void
pushTokenWord(Render *r, TokenId token) {
    pushWord(r, wordText(getTokenString(r->tokens, token)));
    pushCommentsAfterToken(r, token);
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
pushCallArgumentListDocument(Render *r, TokenId startingToken, ASTNodeListRanged *expressions, TokenIdList *names) {
    assert(expressions->count != -1);

    pushGroup(r);
    pushTokenWord(r, startingToken);
    if(names->count > 0) {
        pushTokenWord(r, listGetTokenId(names, 0) - 1);
    }
    pushNest(r);

    Word whitespace = names->count == 0 ? wordSoftline() : wordLine();
    if(names->count == 0) {
        if(expressions->count > 0) {
            pushWord(r, whitespace);
        }

        ASTNodeLink *argument = expressions->head;
        for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
            pushExpressionDocument(r, &argument->node);
            if(i < expressions->count - 1) {
                // Add comma and space after each argument except the last one
                pushTokenWord(r, argument->node.endToken + 1);
                pushWord(r, wordLine());
            }
        }
    } else {
        pushWord(r, whitespace);
        ASTNodeLink *argument = expressions->head;
        assert(names->count == expressions->count);
        for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
            pushGroup(r);
            TokenId literal = listGetTokenId(names, i);
            pushTokenWord(r, literal);
            pushTokenWord(r, literal + 1);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, &argument->node);
            popGroup(r);

            if(i < expressions->count - 1) {
                pushTokenWord(r, argument->node.endToken + 1);
                pushWord(r, wordLine());
            }
        }
    }

    TokenId endToken = 0;
    if(expressions->count > 0) {
        TokenId token = expressions->last->node.endToken + 1;
        token += names->count > 0;
        endToken = token;
    } else {
        endToken = startingToken + 1;
    }

    pushWord(r, whitespace);
    if(names->count > 0) {
        popNest(r);
        pushTokenWord(r, expressions->last->node.endToken + 1);
    } else {
        popNest(r);
    }

    pushTokenWord(r, endToken);
    popGroup(r);

    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[startingToken]));
    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[endToken]));
}

static void pushParametersDocument(Render *r, ASTNodeListRanged *parameters);

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
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));

            pushTokenWord(r, node->startToken);

            pushGroup(r);
            pushGroup(r);
            pushTokenWord(r, node->startToken + 1);
            pushWord(r, wordSoftline());
            pushNest(r);
            pushParametersDocument(r, &node->functionTypeNode.parameters);
            popNest(r);

            if(node->functionTypeNode.parameters.count > 0) {
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->functionTypeNode.parameters.last->node.endToken + 1]));
                pushTokenWord(r, node->functionTypeNode.parameters.last->node.endToken + 1);
            } else {
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->startToken + 2]));
                pushTokenWord(r, node->startToken + 2);
            }
            popGroup(r);

            if(node->functionTypeNode.visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, node->functionTypeNode.visibility);
            }
            if(node->functionTypeNode.stateMutability != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, node->functionTypeNode.stateMutability);
            }

            if(node->functionTypeNode.returnParameters.count > 0) {
                pushWord(r, wordLine());
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[node->functionTypeNode.returnParameters.head->node.startToken - 2]));
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->functionTypeNode.returnParameters.head->node.startToken - 1]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->functionTypeNode.returnParameters.last->node.endToken + 1]));

                pushGroup(r);
                pushTokenWord(r, node->functionTypeNode.returnParameters.head->node.startToken - 2);
                pushWord(r, wordSpace());
                pushTokenWord(r, node->functionTypeNode.returnParameters.head->node.startToken - 1);
                pushNest(r);
                pushParametersDocument(r, &node->functionTypeNode.returnParameters);
                popNest(r);
                pushTokenWord(r, node->functionTypeNode.returnParameters.last->node.endToken + 1);
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

static u32
getOperatorPrecedence2(TokenType type) {
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
        case TokenType_None: return 0;
        default: assert(false);
    }
}

static Word expressionLinkWord(ASTNode *node);
static s32 expressionNest(ASTNode *node);

static void
pushBinaryExpressionDocument(Render *r, ASTNode *node, TokenId outerOperator) {
    ASTNodeBinaryExpression *binary = &node->binaryExpressionNode;

    bool onlyTwoElementBinaryExpression =
        outerOperator == TokenType_None &&
        binary->left->type != ASTNodeType_BinaryExpression &&
        binary->right->type != ASTNodeType_BinaryExpression;

    u32 outerPrecedence = getOperatorPrecedence2(outerOperator);
    u32 innerPrec = getOperatorPrecedence2(binary->operator);
    bool startingPrecedence = outerPrecedence == 0;
    bool differingPrecedence = outerPrecedence != innerPrec;
    bool openGroup = startingPrecedence || differingPrecedence;

    bool outerHigherThanAssignment = outerPrecedence > getOperatorPrecedence2(TokenType_Equal);
    bool notInequality = innerPrec != getOperatorPrecedence2(TokenType_LTick);
    bool notEqualities = innerPrec != getOperatorPrecedence2(TokenType_EqualEqual);
    bool outerNotInequality = outerPrecedence != getOperatorPrecedence2(TokenType_LTick);
    bool outerNotEquality = outerPrecedence != getOperatorPrecedence2(TokenType_EqualEqual);
    bool base = !startingPrecedence && outerHigherThanAssignment && notInequality && notEqualities && outerNotInequality && outerNotEquality;

    bool outerHasLowerPrecedence = outerPrecedence < innerPrec;
    bool outerNotAdd = outerPrecedence != getOperatorPrecedence2(TokenType_Plus);
    bool outerNotMultipliaction = outerPrecedence != getOperatorPrecedence2(TokenType_Star);
    bool operatorsDontMatch = binary->operator != outerOperator;

    // Cares about different things depending on the outer and the inner operator
    bool outerDependentCase = outerNotAdd && (
        outerNotMultipliaction && outerHasLowerPrecedence && innerPrec < getOperatorPrecedence2(TokenType_StarStar) ||
        operatorsDontMatch && innerPrec == getOperatorPrecedence2(TokenType_Star) ||
        outerNotMultipliaction && innerPrec == getOperatorPrecedence2(TokenType_StarStar)
    );

    // Doesn't care about what the outer is, always is present as long as base is also true
    bool alwaysPresentCase = innerPrec == getOperatorPrecedence2(TokenType_LeftShift) || (binary->operator == TokenType_Percent);
    bool addParens = base && (outerDependentCase || alwaysPresentCase);

    if(openGroup || addParens) { pushGroup(r); }
    if(addParens) { pushWord(r, wordText(LIT_TO_STR("("))); }

    if(binary->left->type == ASTNodeType_BinaryExpression) {
        pushBinaryExpressionDocument(r, binary->left, binary->operator);
    } else {
        pushExpressionDocument(r, binary->left);
    }

    pushWord(r, wordSpace());
    pushTokenWord(r, binary->right->startToken - 1);

    if(onlyTwoElementBinaryExpression) { pushGroup(r); }

    if(binary->operator >= TokenType_Equal && binary->operator <= TokenType_PercentEqual) {
        pushWord(r, expressionLinkWord(binary->right));
    } else {
        pushWord(r, wordLine());
    }

    // only indent for the outermost binary expression
    if(outerOperator == TokenType_None && binary->operator >= TokenType_Equal && binary->operator <= TokenType_PercentEqual) {
        addNest(r, expressionNest(binary->right));
    }

    if(binary->right->type == ASTNodeType_BinaryExpression) {
        pushBinaryExpressionDocument(r, binary->right, binary->operator);
    } else {
        pushExpressionDocument(r, binary->right);
    }

    if(outerOperator == TokenType_None && binary->operator >= TokenType_Equal && binary->operator <= TokenType_PercentEqual) {
        addNest(r, -expressionNest(binary->right));
    }

    if(onlyTwoElementBinaryExpression) { popGroup(r); }

    if(addParens) { pushWord(r, wordText(LIT_TO_STR(")"))); }
    if(openGroup || addParens) { popGroup(r); }
}

static void
pushExpressionDocument(Render *r, ASTNode *node) {
    switch(node->type){
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

            pushGroup(r);
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
            popGroup(r);
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
            pushBinaryExpressionDocument(r, node, TokenType_None);
        } break;
        case ASTNodeType_TupleExpression: {
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));

            ASTNodeTupleExpression *tuple = &node->tupleExpressionNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushNest(r);
            pushWord(r, wordSoftline());
            ASTNodeLink *element = tuple->elements.head;
            pushGroup(r);
            for(u32 i = 0; i < tuple->elements.count; i++, element = element->next) {
                bool isNamed = element->node.type != ASTNodeType_None;
                if(isNamed) {
                    if(i > 0) { pushWord(r, wordLine()); }
                    pushExpressionDocument(r, &element->node);
                }

                if(i < tuple->elements.count - 1) {
                    TokenId commaToken = isNamed ? element->node.endToken + 1 : element->next->node.startToken - 1;
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[commaToken]));
                    pushTokenWord(r, commaToken);
                }
            }
            popGroup(r);
            popNest(r);
            pushWord(r, wordSoftline());
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
            ASTNodeFunctionCallExpression *function = &node->functionCallExpressionNode;

            pushExpressionDocument(r, function->expression);
            pushGroup(r);
            pushCallArgumentListDocument(r, function->expression->endToken + 1, &function->argumentsExpression, &function->argumentsName);
            popGroup(r);
        } break;
        case ASTNodeType_BaseType: {
            pushTokenWord(r, node->baseTypeNode.typeName);
        } break;
        case ASTNodeType_MemberAccessExpression: {
            ASTNodeMemberAccessExpression *member = &node->memberAccessExpressionNode;
            pushExpressionDocument(r, member->expression);
            flushTrailing(r); // TODO(radomski): This shouldn't be here?
            pushGroup(r);
            pushNest(r);
            pushWord(r, wordSoftline());
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
                pushExpressionDocument(r, array->leftFenceExpression);
                pushTokenWord(r, array->leftFenceExpression->endToken + 1);
            } else {
                pushTokenWord(r, array->expression->endToken + 2);
            }
            if(array->rightFenceExpression != 0x0) {
                pushExpressionDocument(r, array->rightFenceExpression);
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
            ASTNodeLink *expression = array->expressions.head;
            for(u32 i = 0; i < array->expressions.count; i++, expression = expression->next) {
                pushExpressionDocument(r, &expression->node);
                if(i < array->expressions.count - 1) {
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[expression->node.endToken + 1]));
                    pushTokenWord(r, expression->node.endToken + 1);

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
            popGroup(r);
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
            ASTNodeLink *expression = named->expressions.head;
            for(u32 i = 0; i < named->expressions.count; i++, expression = expression->next) {
                assert(stringMatch(LIT_TO_STR(":"), r->tokens.tokenStrings[listGetTokenId(&named->names, i) + 1]));
                pushTokenWord(r, listGetTokenId(&named->names, i));
                pushTokenWord(r, listGetTokenId(&named->names, i) + 1);
                pushWord(r, wordSpace());
                pushExpressionDocument(r, &expression->node);

                if(i < named->expressions.count - 1) {
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[expression->node.endToken + 1]));
                    pushTokenWord(r, expression->node.endToken + 1);
                    pushWord(r, wordLine());
                }
            }
            popNest(r);
            pushWord(r, wordLine());
            popGroup(r);
            pushTokenWord(r, node->endToken);
        } break;
        default: {
            assert(0);
        }
    }
}

static Word
expressionLinkWord(ASTNode *node) {
    switch(node->type){
        case ASTNodeType_InlineArrayExpression: return wordSpace();
        case ASTNodeType_TerneryExpression: {
            bool complexCondition = node->terneryExpressionNode.condition->type == ASTNodeType_BinaryExpression;
            return complexCondition ? wordLine() : wordSpace();
        }
        case ASTNodeType_FunctionCallExpression: return wordSpace();
        case ASTNodeType_TupleExpression: return wordSpace();
        default: return wordLine();
    }
}

static s32
expressionNest(ASTNode *node) {
    switch(node->type){
        case ASTNodeType_InlineArrayExpression: return 0;
        case ASTNodeType_TerneryExpression: {
            bool complexCondition = node->terneryExpressionNode.condition->type == ASTNodeType_BinaryExpression;
            return complexCondition ? 1 : 0;
        }
        case ASTNodeType_FunctionCallExpression: return 0;
        case ASTNodeType_TupleExpression: return 0;
        default: return 1;
    }
}

static void
pushExpressionDocumentLinked(Render *r, ASTNode *node) {
    pushGroup(r);
    addNest(r, expressionNest(node));
    pushWord(r, expressionLinkWord(node));
    pushExpressionDocument(r, node);
    addNest(r, -expressionNest(node));
    popGroup(r);
}

static void pushYulExpressionDocument(Render *r, ASTNode *node);

static void
pushYulFunctionCallDocument(Render *r, ASTNode *node) {
    ASTNodeYulFunctionCallExpression *function = &node->yulFunctionCallExpressionNode;

    pushGroup(r);
    pushTokenWord(r, function->identifier);

    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->identifier + 1]));
    pushTokenWord(r, function->identifier + 1);

    if (function->arguments.count > 0) {
        pushNest(r);
        pushWord(r, wordSoftline());

        ASTNodeLink *argument = function->arguments.head;
        for(u32 i = 0; i < function->arguments.count; i++, argument = argument->next) {
            pushYulExpressionDocument(r, &argument->node);
            if(i < function->arguments.count - 1) {
                pushTokenWord(r, argument->node.endToken + 1); // comma
                pushWord(r, wordLine());
            }
        }

        popNest(r);
        pushWord(r, wordSoftline());
    }

    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));
    pushTokenWord(r, node->endToken);
    popGroup(r);
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

            if(block->statements.count == 0) {
                pushWord(r, wordSpace());
            }

            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, &statement->node); }
                else { pushWord(r, wordLine()); }

                pushStatementDocument(r, &statement->node);
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
                pushExpressionDocumentLinked(r, node->returnStatementNode.expression);
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
            pushExpressionDocument(r, node->ifStatementNode.conditionExpression);
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

                pushExpressionDocumentLinked(r, statement->initialValue);
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
            ASTNodeLink *it = statement->declarations.head;
            for(u32 i = 0; i < statement->declarations.count; i++, it = it->next) {
                if(it->node.type != ASTNodeType_None) {
                    ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;

                    pushTypeDocument(r, decl->type);
                    pushWord(r, wordSpace());

                    if(decl->dataLocation != INVALID_TOKEN_ID) {
                        pushTokenWord(r, decl->dataLocation);
                        pushWord(r, wordSpace());
                    }

                    pushTokenWord(r, decl->name);
                    if(i < statement->declarations.count - 1) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[decl->name + 1]));
                        pushTokenWord(r, decl->name + 1);
                        pushWord(r, wordLine());
                    }
                } else {
                    if(i < statement->declarations.count - 1) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[it->node.endToken + 1]));
                        pushTokenWord(r, it->node.endToken + 1);
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

            pushExpressionDocumentLinked(r, statement->initialValue);
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
            pushWord(r, wordSpace());

            pushStatementDocument(r, statement->body);
            pushWord(r, wordSpace());

            pushGroup(r);
            TokenId openParen = statement->expression->startToken - 1;
            TokenId closeParen = statement->expression->endToken + 1;
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParen]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParen]));

            pushTokenWord(r, statement->expression->startToken - 2);
            pushNest(r);
            pushTokenWord(r, openParen);
            pushWord(r, wordSoftline());
            pushExpressionDocument(r, statement->expression);
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

            pushGroup(r);
            pushTokenWord(r, node->startToken + 1);
            pushExpressionDocument(r, statement->expression);
            pushTokenWord(r, statement->expression->endToken + 1);
            popGroup(r);

            pushWord(r, wordSpace());
            popGroup(r);

            pushStatementDocument(r, statement->body);
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

            if(statement->variableStatement != 0x0) {
                pushStatementDocument(r, statement->variableStatement);
            } else {
                pushWord(r, wordText(LIT_TO_STR(";")));
            }

            if(statement->conditionExpression != 0x0) {
                pushWord(r, wordLine());
                pushGroup(r);
                pushExpressionDocument(r, statement->conditionExpression);
                popGroup(r);
            }

            pushWord(r, wordText(LIT_TO_STR(";")));

            if(statement->incrementExpression != 0x0) {
                pushWord(r, wordLine());
                pushGroup(r);
                pushExpressionDocument(r, statement->incrementExpression);
                popGroup(r);
            }

            pushWord(r, wordSoftline());

            popGroup(r);
            popNest(r);
            pushTokenWord(r, statement->body->startToken - 1);
            popGroup(r);

            pushWord(r, wordSpace());
            pushStatementDocument(r, statement->body);
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

            if(statement->returnParameters.count != -1) {
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[statement->expression->endToken + 1]));
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[statement->expression->endToken + 2]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[statement->body->startToken - 1]));

                pushWord(r, wordSpace());
                pushTokenWord(r, statement->expression->endToken + 1);
                pushWord(r, wordSpace());

                // TODO(radomski): Does the parameters list always gets blown
                // into multiple lines or do we allow to get the parameters on
                // the same line if they don't straddle the line limit
                pushTokenWord(r, statement->expression->endToken + 2);
                pushWord(r, wordSoftline());
                pushNest(r);
                pushParametersDocument(r, &statement->returnParameters);
                popNest(r);
                pushWord(r, wordSoftline());
                pushTokenWord(r, statement->body->startToken - 1);
            }

            pushWord(r, wordSpace());
            popGroup(r);

            pushStatementDocumentOpenedBlock(r, statement->body);

            ASTNodeLink *catchLink = statement->catches.head;
            for(u32 i = 0; i < statement->catches.count; i++, catchLink = catchLink->next) {
                pushWord(r, wordSpace());

                ASTNodeCatchStatement *catch = &catchLink->node.catchStatementNode;
                assert(stringMatch(LIT_TO_STR("catch"), r->tokens.tokenStrings[catchLink->node.startToken]));

                pushTokenWord(r, catchLink->node.startToken);

                u32 openParenToken = catchLink->node.startToken + 1;
                if(catch->identifier != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, catch->identifier);
                    openParenToken = catch->identifier + 1;
                }

                if(catch->parameters.count != -1) {
                    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));
                    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[catch->body->startToken - 1]));
                    pushTokenWord(r, openParenToken);
                    pushParametersDocument(r, &catch->parameters);
                    pushTokenWord(r, catch->body->startToken - 1);
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

            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, &statement->node); }

                pushStatementDocument(r, &statement->node);
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

            ASTNodeLink *it = statement->paths.head;
            for(u32 i = 0; i < statement->paths.count; i++, it = it->next) {
                pushYulExpressionDocument(r, &it->node);
                if (i < statement->paths.count - 1) {
                    pushTokenWord(r, it->node.endToken + 1); // ,
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

            ASTNodeLink *it = statement->cases.head;
            for(u32 i = 0; i < statement->cases.count; i++, it = it->next) {
                if(i == 0) {
                    pushWord(r, wordHardBreak());
                } else {
                    pushWord(r, wordSpace());
                }

                pushTokenWord(r, it->node.yulCaseNode.literal->startToken - 1); // case
                pushWord(r, wordSpace());
                pushYulExpressionDocument(r, it->node.yulCaseNode.literal);
                pushWord(r, wordSpace());

                pushStatementDocumentOpenedBlock(r, it->node.yulCaseNode.block);
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

            if(block->statements.count == 0) {
                pushWord(r, wordHardBreak());
            }

            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, &statement->node); }
                else { pushWord(r, wordHardBreak()); }

                pushStatementDocument(r, &statement->node);
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

            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) { preserveHardBreaksIntoDocument(r, &statement->node); }
                else { pushWord(r, wordHardBreak()); }

                pushStatementDocument(r, &statement->node);
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
    if(inheritance->argumentsExpression.count > 0) {
        pushCallArgumentListDocument(r, inheritance->identifier->endToken + 1,
                                     &inheritance->argumentsExpression, &inheritance->argumentsName);
    }
}

static void
pushOverridesDocument(Render *r, TokenId overrideToken, ASTNodeListRanged *overrides) {
    if(overrideToken != INVALID_TOKEN_ID) {
        pushWord(r, wordLine());
        assert(stringMatch(LIT_TO_STR("override"), r->tokens.tokenStrings[overrideToken]));
        pushTokenWord(r, overrideToken);
        if(overrides->count > 0) {
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[overrides->startToken - 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[overrides->endToken + 1]));

            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, overrides->startToken - 1);
            pushWord(r, wordSoftline());

            ASTNodeLink *override = overrides->head;
            for(u32 i = 0; i < overrides->count; i++, override = override->next) {
                pushTypeDocument(r, &override->node);
                if(i != overrides->count - 1) {
                    pushTokenWord(r, override->node.endToken + 1);
                    pushWord(r, wordLine());
                }
            }

            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, overrides->endToken + 1);
            popGroup(r);
        }
    }
}

static void
pushParametersDocument(Render *r, ASTNodeListRanged *list) {
    ASTNodeLink *it = list->head;
    for(u32 i = 0; i < list->count; i++, it = it->next) {
        ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;
        pushTypeDocument(r, decl->type);

        if(decl->dataLocation != INVALID_TOKEN_ID) {
            pushWord(r, wordSpace());
            pushTokenWord(r, decl->dataLocation);
        }

        if(decl->name != INVALID_TOKEN_ID) {
            pushWord(r, wordSpace());
            pushTokenWord(r, decl->name);
        }

        if(i != list->count - 1) {
            pushTokenWord(r, it->next->node.variableDeclarationNode.type->startToken - 1);
            pushWord(r, wordLine());
        }
    }
}

static void
pushModifierInvocations(Render *r, ASTNodeList *modifiers) {
    assert(modifiers && modifiers->count > 0);
    ASTNodeLink *it = modifiers->head;
    for(u32 i = 0; i < modifiers->count; i++, it = it->next) {
        pushGroup(r);

        ASTNodeModifierInvocation *invocation = &it->node.modifierInvocationNode;
        pushTypeDocument(r, invocation->identifier);
        if(invocation->argumentsExpression.count != -1) {
            ASTNodeListRanged *expressions = &invocation->argumentsExpression;
            TokenIdList *names = &invocation->argumentsName;

            assert(expressions->count != -1);
            TokenId startingToken = invocation->identifier->endToken + 1;
            pushCallArgumentListDocument(r, startingToken, expressions, names);
        }
        popGroup(r);

        if(i != modifiers->count - 1) {
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

            if(member->symbols.count > 0) {
                assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 1]));
                pushTokenWord(r, member->startToken + 1);
                pushWord(r, wordLine());

                pushNest(r);
                for(u32 i = 0; i < member->symbols.count; i++) {
                    TokenId symbol = listGetTokenId(&member->symbols, i);
                    TokenId alias = listGetTokenId(&member->symbolAliases, i);

                    pushTokenWord(r, symbol);
                    TokenId lastToken = symbol + 1;

                    if(alias != INVALID_TOKEN_ID) {
                        pushWord(r, wordSpace());
                        pushTokenWord(r, alias - 1);
                        pushWord(r, wordSpace());
                        pushTokenWord(r, alias);
                        lastToken = alias + 1;
                    }

                    bool isLastElement = i == member->symbols.count - 1;
                    if(!isLastElement) {
                        pushTokenWord(r, lastToken);
                    }
                    pushWord(r, wordLine());
                }
                popNest(r);

                assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->pathTokenId - 2]));
                assert(stringMatch(LIT_TO_STR("from"), r->tokens.tokenStrings[member->pathTokenId - 1]));
                pushTokenWord(r, member->pathTokenId - 2);
                pushWord(r, wordSpace());
                pushTokenWord(r, member->pathTokenId - 1);
                pushWord(r, wordSpace());
            }

            pushTokenAsStringWord(r, member->pathTokenId);
            if(member->unitAliasTokenId != INVALID_TOKEN_ID) {
                assert(stringMatch(LIT_TO_STR("as"), r->tokens.tokenStrings[member->unitAliasTokenId - 1]));

                pushWord(r, wordSpace());
                pushTokenWord(r, member->unitAliasTokenId - 1);
                pushWord(r, wordSpace());
                pushTokenWord(r, member->unitAliasTokenId);
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
                assert(using->identifiers.count == 1);
                pushTypeDocument(r, &using->identifiers.head->node);
                pushWord(r, wordSpace());
                forToken = using->identifiers.head->node.endToken + 1;
            } else {
                pushTokenWord(r, using->identifiers.startToken - 1);
                pushWord(r, wordSpace());

                ASTNodeLink *it = using->identifiers.head;
                for(u32 i = 0; i < using->identifiers.count; i++, it = it->next) {
                    pushTypeDocument(r, &it->node);

                    u16 operator = listGetU16(&using->operators, i);
                    if(operator) {
                        pushWord(r, wordSpace());
                        pushTokenWord(r, it->node.endToken + 1);
                        pushWord(r, wordSpace());
                        pushWord(r, wordText(tokenTypeToString(operator)));
                    }

                    if(i != using->identifiers.count - 1) {
                        pushTokenWord(r, it->next->node.startToken - 1);
                    }

                    pushWord(r, wordSpace());
                }
                pushTokenWord(r, using->identifiers.endToken + 1);
                pushWord(r, wordSpace());
                forToken = using->identifiers.endToken + 2;
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
            pushGroup(r);
            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, member->nameTokenId);
            pushWord(r, wordSpace());
            pushTokenWord(r, member->nameTokenId + 1);

            pushNest(r);
            pushWord(r, wordLine());
            for(u32 i = 0; i < member->values.count; i++) {
                TokenId value = listGetTokenId(&member->values, i);
                pushTokenWord(r, value);
                if(i != member->values.count - 1) {
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

            ASTNodeListRanged *list = &member->structNode.members;
            ASTNodeLink *it = list->head;
            for(u32 i = 0; i < list->count; i++, it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;
                preserveHardBreaksIntoDocument(r, &it->node);

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
            ASTNodeLink *it = member->errorNode.parameters.head;
            for(u32 i = 0; i < member->errorNode.parameters.count; i++, it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;

                pushTypeDocument(r, decl->type);

                if(decl->dataLocation != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->dataLocation);
                }

                if(decl->name != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->name);
                }

                if(i != member->errorNode.parameters.count - 1) {
                    pushTokenWord(r, it->next->node.variableDeclarationNode.type->startToken - 1);
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
            ASTNodeLink *it = member->errorNode.parameters.head;
            for(u32 i = 0; i < member->errorNode.parameters.count; i++, it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;

                pushTypeDocument(r, decl->type);

                if(decl->dataLocation != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->dataLocation);
                }

                if(decl->name != INVALID_TOKEN_ID) {
                    pushWord(r, wordSpace());
                    pushTokenWord(r, decl->name);
                }

                if(i != member->errorNode.parameters.count - 1) {
                    pushTokenWord(r, it->next->node.variableDeclarationNode.type->startToken - 1);
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

            pushExpressionDocumentLinked(r, constNode->expression);

            pushTokenWord(r, member->endToken);
            popGroup(r);

            pushWord(r, wordHardBreak());
        } break;
        case ASTNodeType_StateVariableDeclaration: {
            ASTNodeConstVariable *decl = &member->constVariableNode;

            pushGroup(r);
            pushTypeDocument(r, decl->type);

            pushGroup(r);
            pushNest(r);
            if(decl->visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, decl->visibility);
            }
            if(decl->mutability != INVALID_TOKEN_ID) {
                pushWord(r, wordLine());
                pushTokenWord(r, decl->mutability);
            }

            pushOverridesDocument(r, decl->override, &decl->overrides);

            pushWord(r, wordSpace());
            pushTokenWord(r, decl->identifier);
            popNest(r);
            popGroup(r);

            if(decl->expression) {

                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[decl->identifier + 1]));
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->identifier + 1);

                pushExpressionDocumentLinked(r, decl->expression);
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

            u32 closeParenToken = function->parameters.count > 0
                ? function->parameters.last->node.endToken + 1
                : openParenToken + 1;

            pushGroup(r);
            pushTokenWord(r, openParenToken);
            pushWord(r, wordSoftline());
            pushNest(r);
            pushParametersDocument(r, &function->parameters);
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, closeParenToken);
            popGroup(r);

            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParenToken]));

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

            pushOverridesDocument(r, function->override, function->overrides);

            if(function->modifiers && function->modifiers->count > 0) {
                pushWord(r, wordLine());
                pushModifierInvocations(r, function->modifiers);
            }

            if(function->returnParameters) {
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[function->returnParameters->startToken - 2]));
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->returnParameters->startToken - 1]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[function->returnParameters->endToken + 1]));

                pushWord(r, wordLine());
                pushGroup(r);
                pushTokenWord(r, function->returnParameters->startToken - 2);
                pushWord(r, wordSpace());
                pushTokenWord(r, function->returnParameters->startToken - 1);
                pushWord(r, wordSoftline());
                pushNest(r);
                pushParametersDocument(r, function->returnParameters);
                popNest(r);
                pushWord(r, wordSoftline());
                pushTokenWord(r, function->returnParameters->endToken + 1);
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
            TokenId closeParenToken = constructor->parameters.count > 0
                ? constructor->parameters.last->node.endToken + 1
                : openParenToken + 1;

            pushGroup(r);
            pushTokenWord(r, openParenToken);
            pushWord(r, wordSoftline());
            pushNest(r);
            pushParametersDocument(r, &constructor->parameters);
            popNest(r);
            pushWord(r, wordSoftline());
            pushTokenWord(r, closeParenToken);
            popGroup(r);

            pushGroup(r);
            pushNest(r);
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParenToken]));

            if(constructor->modifiers.count > 0) {
                pushWord(r, wordLine());
                pushModifierInvocations(r, &constructor->modifiers);
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

            if(modifier->parameters.count != -1) {
                TokenId openParenToken = modifier->name + 1;
                TokenId closeParenToken = modifier->parameters.count > 0
                    ? modifier->parameters.last->node.endToken + 1
                    : openParenToken + 1;

                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParenToken]));

                pushTokenWord(r, openParenToken);
                pushParametersDocument(r, &modifier->parameters);
                pushTokenWord(r, closeParenToken);
            }

            if(modifier->virtual != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, modifier->virtual);
            }

            pushOverridesDocument(r, modifier->override, modifier->overrides);

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

            if(contract->baseContracts.count > 0) {
                pushGroup(r);
                pushNest(r);
                assert(stringMatch(LIT_TO_STR("is"), r->tokens.tokenStrings[contract->name + 1]));
                pushTokenWord(r, contract->name + 1);
                pushWord(r, wordLine());

                ASTNodeLink *baseContract = contract->baseContracts.head;
                for(u32 i = 0; i < contract->baseContracts.count; i++, baseContract = baseContract->next) {
                    pushInheritanceSpecifierDocument(r, &baseContract->node);
                    if(i != contract->baseContracts.count - 1) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[baseContract->node.endToken + 1]));
                        pushTokenWord(r, baseContract->node.endToken + 1);
                    }
                    pushWord(r, wordLine());
                }
                popNest(r);
                popGroup(r);
            }

            pushGroup(r);
            u32 openParenToken = contract->elements.count > 0
                ? contract->elements.head->node.startToken - 1
                : member->endToken - 1;

            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[openParenToken]));
            pushTokenWord(r, openParenToken);
            pushNest(r);
            pushWord(r, wordLine());

            ASTNodeLink *element = contract->elements.head;
            for(u32 i = 0; i < contract->elements.count; i++, element = element->next) {
                if(i > 0) {
                    preserveHardBreaksIntoDocument(r, &element->node);
                }
                pushMemberDocument(r, &element->node);
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

static void
renderSourceUnit(Render *r, ASTNode *tree) {
    gCyclesTable[Measurement_BuildDoc] = -readCPUTimer();
    pushGroup(r);
    ASTNodeLink *child = tree->children.head;
    for(u32 i = 0; i < tree->children.count; i++, child = child->next) {
        preserveHardBreaksIntoDocument(r, &child->node);
        pushMemberDocument(r, &child->node);
    }
    popGroup(r);
    gCyclesTable[Measurement_BuildDoc] += readCPUTimer();

    gCyclesTable[Measurement_RenderDoc] = -readCPUTimer();
    renderDocument(r);
    gCyclesTable[Measurement_RenderDoc] += readCPUTimer();
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
        case WordType_NestPush: return LIT_TO_STR("NestPush");
        case WordType_NestPop: return LIT_TO_STR("NestPop");
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
dumpDocument(Render *r, Arena *arena) {
    const char *flag = getenv("DDOC");
    if(flag == NULL) {
        return;
    }

    FILE *output = fopen("document.csv", "wb");
    fprintf(output, "Type,Text Size,Text\n");
    for(u32 i = 0; i < r->wordCount; i++) {
        Word *word = &r->words[i];
        fprintf(output, "%.*s,%zu,%.*s\n",
                (int)wordTypeToString(word->type).size, wordTypeToString(word->type).data,
                word->text.size,
                (int)word->text.size, word->text.data);
    }
    fclose(output);
}

static String
renderTree(Arena *arena, ASTNode tree, String originalSource, TokenizeResult tokens) {
    Writer writer = {
        .data = arrayPush(arena, u8, originalSource.size * 4),
        .size = 0,
        .capacity = originalSource.size * 4 ,
        .indentSize = 4,
    };

    u32 scratchBufferCapacity = 8 * Megabyte;
    Render render = {
        .writer = &writer,
        .tokens = tokens,
        .sourceBaseAddress = originalSource.data,

        // NOTE(radomski): This makes read and writes to (-1) possible
        .words = arrayPush(arena, Word, 131072) + 1,
        .wordCount = 0,
        .wordCapacity = 131071,

        .scratchBufferCapacity = scratchBufferCapacity,
        .scratchBufferSize = 0,
        .scratchBuffer = arrayPush(arena, u8, scratchBufferCapacity),
    };

    u32 startOfTokens = tokens.tokenStrings[tree.startToken].data - originalSource.data;
    pushCommentsInRange(&render, 0, startOfTokens);
    renderSourceUnit(&render, &tree);

    dumpDocument(&render, arena);

    return (String){ .data = writer.data, .size = writer.size };
}
