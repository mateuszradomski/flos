typedef struct Writer {
    u8 *data;
    u32 size;
    u32 capacity;

    u32 indentSize;
    u32 indentCount;
    u32 lineSize;
} Writer;

typedef enum ConnectType {
    NONE,
    SPACE,
    COMMA,
    JUST_COMMA,
    COMMA_SPACE,
    COLON_SPACE,
    SEMICOLON,
    JUST_SEMICOLON,
    DOT,
    NEWLINE,
} ConnectType;

typedef enum WordType {
    WordType_None,
    WordType_Text,
    WordType_Line,
    WordType_Space,
    WordType_Softline,
    WordType_Hardline,
    WordType_Count,
} WordType;

typedef struct Word {
    WordType type;
    u8 group;
    u8 nest;
    union {
        String text;
        TokenId token;
    };
} Word;

typedef enum WordRenderLineType {
    WordRenderLineType_Space,
    WordRenderLineType_Newline,
} WordRenderLineType;


typedef struct Render {
    Writer *writer;
    TokenizeResult tokens;
    u8 *sourceBaseAddress;

    Word *words;
    s32 wordCount;
    u32 wordCapacity;
    u8 group;
    u8 nest;
} Render;

static void
commitIndent(Writer *w) {
    for(u32 i = 0; i < w->indentSize * w->indentCount; i++) {
        w->data[w->size++] = ' ';
        w->lineSize += 1;
    }
}

static void
writeString(Writer *w, String str) {
    String newline = LIT_TO_STR("\n") ;
    if(w->lineSize == 0 && !stringMatch(newline, str)) {
        commitIndent(w);
    }

    memcpy(w->data + w->size, str.data, str.size);
    w->size += str.size;
    w->lineSize += str.size;
}

static void
finishLine(Writer *w) {
    if(w->size > 0 && w->data[w->size - 1] == ' ') {
        w->size -= 1;
    }

    writeString(w, LIT_TO_STR("\n"));
    w->lineSize = 0;
}

static void
pushIndent(Writer *w) {
    w->indentCount += 1;
}

static void
setIndent(Writer *w, u32 indentCount) {
    w->indentCount = indentCount;
}

static void
popIndent(Writer *w) {
    assert(w->indentCount > 0);
    w->indentCount -= 1;
}

static void
popBytes(Writer *w, u32 bytes) {
    w->size -= bytes;
}

static void
preservePresentNewLines(Render *r, ASTNode *node) {
    u32 tokenIndex = node->startToken;

    u32 previousTokenIndex = tokenIndex == 0 ? 0 : tokenIndex - 1;
    String previousToken = r->tokens.tokenStrings[previousTokenIndex];
    String token = r->tokens.tokenStrings[tokenIndex];

    String inBetween = {};
    inBetween.data = previousToken.data + previousToken.size;
    inBetween.size = token.data >= inBetween.data ? token.data - inBetween.data : 0;

    for(u32 i = 0; inBetween.size > 0 && i < inBetween.size - 1; i++) {
        if(inBetween.data[i] == '/' && inBetween.data[i + 1] == '/') {
            return;
        } else if(inBetween.data[i] == '/' && inBetween.data[i + 1] == '*') {
            return;
        }
    }

    u32 newlines = 0;
    for(u32 i = 0; i < inBetween.size; i++) {
        // Skip comment
        if(inBetween.data[i] == '/') {
            if(i + 1 < inBetween.size) {
                u32 isLineComment = inBetween.data[i + 1] == '/';
                u32 isMultilineComment = inBetween.data[i + 1] == '*';
                assert(isLineComment || isMultilineComment);
                i += 2;

                if(isLineComment) {
                    for(; i < inBetween.size && inBetween.data[i] != '\n'; i++) { }
                }

                for(; i < inBetween.size - 1; i++) {
                    if(inBetween.data[i] == '*' && inBetween.data[i + 1] == '/') {
                        i += 2;
                        break;
                    }
                }
            }
        }

        newlines += inBetween.data[i] == '\n';
        if(newlines == 2) {
            finishLine(r->writer);
            return;
        }
    }
}

static void renderExpression(Render *r, ASTNode *node, ConnectType connect);
static void renderType(Render *r, ASTNode *node, ConnectType connect);
static void renderYulExpression(Render *r, ASTNode *node, ConnectType connect);
static void renderParameters(Render *r, ASTNodeListRanged *list, ConnectType connect, u32 connectIsInclusive);
static void renderToken(Render *r, TokenId token, ConnectType connect);


enum CommentType {
    CommentType_None,
    CommentType_SingleLine,
    CommentType_MultiLine,
    CommentType_StarAligned
};

static u32
renderComments(Render *r, u32 startOffset, u32 endOffset) {
    u32 commentCount = 0;
    String input = (String) {
        .data = r->sourceBaseAddress + startOffset,
        .size = endOffset - startOffset,
    };

    if(input.size < 2) {
        return commentCount;
    }

    u32 index = 0;
    while(true) {
        u32 finished = true; 
        for(u32 i = index; i < input.size - 1; i++) {
            String comment = (String) { .data = 0x0, .size = 0 };
            u32 commentStart = i; 
            u32 commentEnd = i;

            u32 commentType = CommentType_None;
            u32 indexSkipSize = 0;
            if(input.data[i] == '/' && input.data[i + 1] == '/') {
                u32 newlineIndex = i;
                while(newlineIndex < input.size && input.data[newlineIndex] != '\n') {
                    newlineIndex += 1;
                }

                indexSkipSize += 1;
                if(input.data[newlineIndex - 1] == '\r') {
                    newlineIndex -= 1;
                    indexSkipSize += 1;
                }

                commentCount += 1;
                comment = (String) {
                    .data = input.data + commentStart,
                    .size = newlineIndex - commentStart
                };
                commentEnd = newlineIndex;
                commentType = CommentType_SingleLine;
            } else if(input.data[i] == '/' && input.data[i + 1] == '*') {
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

                if(input.data[commentEnd] == '\r') {
                    indexSkipSize += 1;
                }

                commentCount += 1;
                comment = (String) {
                    .data = input.data + commentStart,
                    .size = commentEnd - commentStart
                };

                commentType = isStarAligned ? CommentType_StarAligned : CommentType_MultiLine;
            }

            if(comment.size > 0) {
                u32 preceedingNewlines = 0;
                for(u32 j = index; j < commentStart; j++) {
                    preceedingNewlines += input.data[j] == '\n';
                }

                if(index == 0 && preceedingNewlines >= 2) {
                    finishLine(r->writer);
                    finishLine(r->writer);
                } else if(preceedingNewlines > 0) {
                    finishLine(r->writer);
                } else if(index == 0) {
                    writeString(r->writer, LIT_TO_STR(" "));
                } else if(commentType != CommentType_SingleLine) {
                    writeString(r->writer, LIT_TO_STR(" "));
                }

                index = commentEnd + indexSkipSize;
                i = index - 1;

                assert(commentType != CommentType_None);

                // Write comment out
                bool startOfLine = r->writer->lineSize == 0;
                if(startOfLine) {
                    commitIndent(r->writer);
                }

                if(commentType == CommentType_StarAligned && startOfLine) {
                    for(u32 i = 0; i < comment.size; i++) {
                        u8 c = comment.data[i];
                        if(c == '\n') {
                            finishLine(r->writer);
                        } else if(!(isWhitespace(c) && r->writer->lineSize == 0)) {
                            if(r->writer->lineSize == 0) {
                                commitIndent(r->writer);
                                r->writer->data[r->writer->size++] = ' ';
                                r->writer->lineSize += 1;
                            }

                            r->writer->data[r->writer->size++] = c;
                            r->writer->lineSize += 1;
                        }
                    }
                } else {
                    for(u32 i = 0; i < comment.size; i++) {
                        if(comment.data[i] == '\n') {
                            finishLine(r->writer);
                        } else if(comment.data[i] != '\r') {
                            r->writer->data[r->writer->size++] = comment.data[i];
                            r->writer->lineSize += 1;
                        }
                    }
                }

                // Endings
                if(commentType == CommentType_SingleLine) {
                    finishLine(r->writer);
                } else {
                    u8 *head = comment.data + comment.size;
                    while(isWhitespace(*head)) {
                        if(*head == '\n') {
                            finishLine(r->writer);
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
    for(u32 i = index; i < input.size; i++) {
        preceedingNewlines += input.data[i] == '\n';
    }

    if(index != 0 && preceedingNewlines >= 2) {
        finishLine(r->writer);
    } 

    return commentCount;
}

static void
renderConnect(Render *r, ConnectType connect) {
    switch(connect) {
        case NONE: break;
        case SPACE: writeString(r->writer, LIT_TO_STR(" ")); break;
        case COMMA: writeString(r->writer, LIT_TO_STR(",")); finishLine(r->writer); break;
        case JUST_COMMA: writeString(r->writer, LIT_TO_STR(",")); break;
        case COMMA_SPACE: writeString(r->writer, LIT_TO_STR(", ")); break;
        case COLON_SPACE: writeString(r->writer, LIT_TO_STR(": ")); break;
        case SEMICOLON: writeString(r->writer, LIT_TO_STR(";")); finishLine(r->writer); break;
        case JUST_SEMICOLON: writeString(r->writer, LIT_TO_STR(";")); break;
        case DOT: writeString(r->writer, LIT_TO_STR(".")); break;
        case NEWLINE: finishLine(r->writer); break;
    }
}

static void
renderConnectWithComments(Render *r, TokenId token, ConnectType connect) {
    TokenId nextToken = token == r->tokens.count - 1 ? token : token + 1; 
    String next = getTokenString(r->tokens, nextToken);
    String current = getTokenString(r->tokens, token);

    u32 startOffset = (current.data - r->sourceBaseAddress) + current.size;
    u32 endOffset = next.data ? next.data - r->sourceBaseAddress : startOffset;
    u32 writtenComments = renderComments(r, startOffset, endOffset);

    switch(connect) {
        case NONE: break;
        case SPACE: {
            if(writtenComments == 0) {
                writeString(r->writer, LIT_TO_STR(" "));
            }
        } break;
        case COMMA: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Comma) {
                renderToken(r, nextToken, NEWLINE);
            } else if(writtenComments == 0) {
                finishLine(r->writer);
            }
        } break;
        case JUST_COMMA: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Comma) {
                renderToken(r, nextToken, NONE);
            }
        } break;
        case COMMA_SPACE: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Comma) {
                renderToken(r, nextToken, SPACE);
            } else if(writtenComments == 0) {
                writeString(r->writer, LIT_TO_STR(" "));
            }
        } break;
        case COLON_SPACE: writeString(r->writer, LIT_TO_STR(": ")); break;
        case SEMICOLON: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Semicolon) {
                renderToken(r, nextToken, NEWLINE);
            } else if(writtenComments == 0) {
                finishLine(r->writer);
            }
        } break;
        case JUST_SEMICOLON: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Semicolon) {
                renderToken(r, nextToken, NONE);
            }
        } break;
        case DOT: writeString(r->writer, LIT_TO_STR(".")); break;
        case NEWLINE: {
            if(writtenComments == 0) {
                finishLine(r->writer);
            }
        } break;
    }
}

static void
renderString(Render *r, String string, ConnectType connect) {
    writeString(r->writer, string);
    renderConnect(r, connect);
}

#define renderCString(r, s, c) renderString(r, LIT_TO_STR((s)), c)

static void
renderToken(Render *r, TokenId token, ConnectType connect) {
    assert(token != INVALID_TOKEN_ID);
    writeString(r->writer, getTokenString(r->tokens, token));
    renderConnectWithComments(r, token, connect);
}

static void
renderTokenChecked(Render *r, TokenId token, String expected, ConnectType connect) {
    assert(stringMatch(expected, r->tokens.tokenStrings[token]));
    renderToken(r, token, connect);
}

static void renderDocumentWord(Render *r, Word *word, WordRenderLineType lineType);

static bool
hasHardline(Word *words, u32 count, u32 group) {
    for (u32 i = 0; i < count; i++) {
        Word *word = &words[i];
        if (word->group < group) {
            break;
        }
        if (word->group == group && word->type == WordType_Hardline) {
            return true;
        }
    }
    return false;
}

static u32 renderGroup(Render *r, Word *words, s32 count, u32 group);
static u32 processGroupWords(Render *r, Word *words, s32 count, u32 group, WordRenderLineType lineType) {
    u32 processed = 0;
    for (u32 i = 0; i < (u32)count && words[i].group >= group;) {
        Word *word = &words[i];
        if (word->group > group) {
            u32 sub_processed = renderGroup(r, words + i, count - i, word->group);
            i += sub_processed;
            processed += sub_processed;
        } else {
            renderDocumentWord(r, word, lineType);
            i++;
            processed++;
        }
    }
    return processed;
}

static u32
renderGroup(Render *r, Word *words, s32 count, u32 group) {
    if (count <= 0) return 0;

    Writer *w = r->writer;

    // Early exit for hardline groups
    if (hasHardline(words, count, group)) {
        return processGroupWords(r, words, count, group, WordRenderLineType_Newline);
    }

    // Try fitting everything on one line
    u32 checkpoint = w->lineSize;
    u32 writtenCheckpoint = w->size;
    u32 processed = processGroupWords(r, words, count, group, WordRenderLineType_Space);

    // If it fits, we're done
    if (w->size - writtenCheckpoint <= 120 - checkpoint) {
        return processed;
    }

    // Rollback and try with newlines
    w->size = writtenCheckpoint;
    w->lineSize = checkpoint;
    
    return processGroupWords(r, words, count, group, WordRenderLineType_Newline);
}

static void
renderDocumentWord(Render *r, Word *word, WordRenderLineType lineType) {
    Writer *w = r->writer;
    setIndent(w, word->nest);

    switch(word->type) {
        case WordType_Text: {
            writeString(r->writer, word->text);
        } break;
        case WordType_Space: {
            writeString(r->writer, LIT_TO_STR(" "));
        } break;
        case WordType_Line: {
            switch (lineType) {
                case WordRenderLineType_Space: {
                    writeString(r->writer, LIT_TO_STR(" "));
                } break;
                case WordRenderLineType_Newline: { finishLine(w); } break;
            }
        } break;
        case WordType_Softline: {
            switch (lineType) {
                case WordRenderLineType_Space: { } break;
                case WordRenderLineType_Newline: { finishLine(w); } break;
            }
        } break;
        case WordType_Hardline: { finishLine(w); } break;
        default: { assert(false); }
    }
}

static void
renderDocument(Render *r) {
    renderGroup(r, r->words, r->wordCount, 0);
    r->wordCount = 0;
}

// TODO(radomski): This shouldn't really exist, I don't know how to handle this case yet
static void
trimGroupRight(Render *r) {
    while(r->words[r->wordCount - 1].group == r->group && r->words[r->wordCount - 1].type == WordType_Hardline) {
        r->wordCount -= 1;
    }
}

static void
pushGroup(Render *r) {
    r->group += 1;
}

static void
popGroup(Render *r) {
    r->group -= 1;
}

static void
pushNest(Render *r) {
    r->nest += 1;
}

static void
popNest(Render *r) {
    r->nest -= 1;
}

static void
popNestWithLastWord(Render *r) {
    popNest(r);
    r->words[r->wordCount - 1].nest -= 1;
}

static void
pushWord(Render *r, Word w) {
    w.group = r->group;
    // TODO(radomski): There should be a single indent counter
    w.nest = r->nest + r->writer->indentCount;

    r->words[r->wordCount++] = w;
}

static Word
wordText(String text) {
    return (Word) { .type = WordType_Text, .text = text };
}

static Word
wordHardline() {
    return (Word) { .type = WordType_Hardline };
}

static Word
wordSoftline() {
    return (Word) { .type = WordType_Softline };
}

static Word
wordSpace() {
    return (Word) { .type = WordType_Space };
}

static Word
wordLine() {
    return (Word) { .type = WordType_Line };
}

static void
parseCommentsIntoWords(Render *r, u32 startOffset, u32 endOffset) {
    String input = (String) {
        .data = r->sourceBaseAddress + startOffset,
        .size = endOffset - startOffset,
    };

    if(input.size >= 2) {
        u32 cursor = 0;

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
                    commentEnd = newlineIndex;
                } else if(input.data[i] == '/' && input.data[i + 1] == '*') {
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
                        pushWord(r, wordHardline());
                        pushWord(r, wordHardline());
                    } else if(preceedingNewlines > 0) {
                        pushWord(r, wordHardline());
                    } else if(cursor == 0) {
                        pushWord(r, wordSpace());
                    } else if(commentType != CommentType_SingleLine) {
                        pushWord(r, wordSpace());
                    }

                    cursor = commentEnd + 1;
                    i = cursor - 1;

                    // Write comment out
                    if(commentType == CommentType_SingleLine) {
                        pushWord(r, wordText(stringTrim(comment)));
                        pushWord(r, wordHardline());
                    } else if(commentType == CommentType_StarAligned) {
                        String line = { .data = comment.data, .size = 0 };
                        for(u32 i = 0; i < comment.size; i++) {
                            if(comment.data[i] == '\n') {
                                pushWord(r, wordText(stringTrim(line)));
                                pushWord(r, wordHardline());
                                line = (String){ .data = line.data + line.size + 1, .size = 0 };
                            } else {
                                line.size += 1;
                            }
                        }

                        if(line.size > 0) {
                            pushWord(r, wordText(stringTrim(line)));
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
            pushWord(r, wordHardline());
        } 
    }
}

static void
preserveHardlinesIntoDocument(Render *r, ASTNode *node) {
    u32 tokenIndex = node->startToken;

    u32 previousTokenIndex = tokenIndex == 0 ? 0 : tokenIndex - 1;
    String previousToken = r->tokens.tokenStrings[previousTokenIndex];
    String token = r->tokens.tokenStrings[tokenIndex];

    String inBetween = {};
    inBetween.data = previousToken.data + previousToken.size;
    inBetween.size = token.data >= inBetween.data ? token.data - inBetween.data : 0;

    for(u32 i = 0; inBetween.size > 0 && i < inBetween.size - 1; i++) {
        if(inBetween.data[i] == '/' && inBetween.data[i + 1] == '/') {
            return;
        } else if(inBetween.data[i] == '/' && inBetween.data[i + 1] == '*') {
            return;
        }
    }

    u32 newlines = 0;
    for(u32 i = 0; i < inBetween.size; i++) {
        // TODO(radomski): I don't think that this is necessary
        if(inBetween.data[i] == '/') {
            assert(false);
            if(i + 1 < inBetween.size) {
                u32 isLineComment = inBetween.data[i + 1] == '/';
                u32 isMultilineComment = inBetween.data[i + 1] == '*';
                assert(isLineComment || isMultilineComment);
                i += 2;

                if(isLineComment) {
                    for(; i < inBetween.size && inBetween.data[i] != '\n'; i++) { }
                }

                for(; i < inBetween.size - 1; i++) {
                    if(inBetween.data[i] == '*' && inBetween.data[i + 1] == '/') {
                        i += 2;
                        break;
                    }
                }
            }
        }

        newlines += inBetween.data[i] == '\n';
        if(newlines == 2) {
            pushWord(r, wordHardline());
            return;
        }
    }
}



static void
pushTokenWordImpl(Render *r, Word w, TokenId token) {
    r->words[r->wordCount++] = w;

    TokenId nextToken = token == r->tokens.count - 1 ? token : token + 1; 
    String next = getTokenString(r->tokens, nextToken);
    String current = getTokenString(r->tokens, token);

    u32 startOffset = (current.data - r->sourceBaseAddress) + current.size;
    u32 endOffset = next.data ? next.data - r->sourceBaseAddress : startOffset;
    parseCommentsIntoWords(r, startOffset, endOffset);
}

static void
pushTokenWord(Render *r, TokenId token) {
    Word w = {
        .type = WordType_Text,
        .text = getTokenString(r->tokens, token),
        .group = r->group,
        // TODO(radomski): There should be a single indent counter
        .nest = r->nest + r->writer->indentCount,
    };

    pushTokenWordImpl(r, w, token);
}

static void
pushTokenAsStringWord(Render *r, TokenId token) {
    String text = getTokenString(r->tokens, token);
    assert(text.data[-1] == '"');
    assert(text.data[text.size] == '"');
    text.data -= 1;
    text.size += 2;

    Word w = {
        .type = WordType_Text,
        .text = text,
        .group = r->group,
        // TODO(radomski): There should be a single indent counter
        .nest = r->nest + r->writer->indentCount,
    };

    pushTokenWordImpl(r, w, token);
}

static WordType
getConnectingWordType(Render *r, TokenId t1, TokenId t2) {
    return WordType_Space;
}

static void pushExpressionDocument(Render *r, ASTNode *node);

static void
pushCallArgumentListDocument(Render *r, TokenId startingToken, ASTNodeListRanged *expressions, TokenIdList *names) {
    if(expressions->count == -1) {
        assert(false);
        return;
    }

    pushTokenWord(r, startingToken);
    if(names->count == 0) {
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
        pushTokenWord(r, listGetTokenId(names, 0) - 1);
        pushWord(r, wordSpace());
        ASTNodeLink *argument = expressions->head;
        assert(names->count == expressions->count);
        for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
            TokenId literal = listGetTokenId(names, i);
            pushTokenWord(r, literal);
            pushTokenWord(r, literal + 1);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, &argument->node);

            if(i < expressions->count - 1) {
                pushTokenWord(r, argument->node.endToken + 1);
                pushWord(r, wordLine());
            }
        }
        pushTokenWord(r, expressions->last->node.endToken + 1);
    }

    if(expressions->count > 0) {
        TokenId token = expressions->last->node.endToken + 1;
        token += names->count > 0;
        pushTokenWord(r, token);
    } else {
        pushTokenWord(r, startingToken + 1);
    }
}

static void
renderCallArgumentList(Render *r, TokenId startingToken, ASTNodeListRanged *expressions, TokenIdList *names, ConnectType connect) {
    if(expressions->count == -1) {
        assert(false);
        return;
    }

    renderTokenChecked(r, startingToken, LIT_TO_STR("("), NONE);
    if(names->count == 0) {
        ASTNodeLink *argument = expressions->head;
        for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
            void renderExpression(Render *r, ASTNode *node, ConnectType connect);
            ConnectType connect = i == expressions->count - 1 ? NONE : COMMA_SPACE;
            renderExpression(r, &argument->node, connect);
        }
    } else {
        renderTokenChecked(r, listGetTokenId(names, 0) - 1, LIT_TO_STR("{"), SPACE);
        ASTNodeLink *argument = expressions->head;
        assert(names->count == expressions->count);
        for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {

            TokenId literal = listGetTokenId(names, i);
            ConnectType connect = i == expressions->count - 1 ? NONE : COMMA_SPACE;
            renderToken(r, literal, NONE);
            renderTokenChecked(r, literal + 1, LIT_TO_STR(":"), SPACE);
            renderExpression(r, &argument->node, connect);
        }
        renderTokenChecked(r, expressions->last->node.endToken + 1, LIT_TO_STR("}"), NONE);
    }

    if(expressions->count > 0) {
        TokenId token = expressions->last->node.endToken + 1;
        token += names->count > 0;
        renderTokenChecked(r, token, LIT_TO_STR(")"), connect);
    } else {
        renderTokenChecked(r, startingToken + 1, LIT_TO_STR(")"), connect);
    }
}

static void
renderExpression(Render *r, ASTNode *node, ConnectType connect) {
    switch(node->type){
        case ASTNodeType_NumberLitExpression: {
            TokenId tokens[2];
            u32 tokenCount = 0;
            tokens[tokenCount++] = node->numberLitExpressionNode.value;
            if(node->numberLitExpressionNode.subdenomination != INVALID_TOKEN_ID) {
                tokens[tokenCount++] = node->numberLitExpressionNode.subdenomination;
            }

            for(u32 i = 0; i < tokenCount; i++) {
                renderToken(r, tokens[i], i == tokenCount - 1 ? connect : SPACE);
            }
        } break;
        case ASTNodeType_HexStringLitExpression:
        case ASTNodeType_UnicodeStringLitExpression:
        case ASTNodeType_StringLitExpression: {
            ASTNodeStringLitExpression *expression = &node->stringLitExpressionNode;
            // TODO(radomski): Escaping

            if(node->type == ASTNodeType_HexStringLitExpression) {
                renderCString(r, "hex", NONE);
            } else if(node->type == ASTNodeType_UnicodeStringLitExpression) {
                renderCString(r, "unicode", NONE);
            }
            writeString(r->writer, LIT_TO_STR("\""));
            for(u32 i = 0; i < expression->values.count; i++) {
                TokenId literal = listGetTokenId(&expression->values, i);
                renderToken(r, literal, NONE);
            }
            writeString(r->writer, LIT_TO_STR("\""));
            renderConnect(r, connect);
        } break;
        case ASTNodeType_BoolLitExpression: {
            renderToken(r, node->boolLitExpressionNode.value, connect);
        } break;
        case ASTNodeType_IdentifierPath:
        case ASTNodeType_ArrayType:
        case ASTNodeType_MappingType: {
            renderType(r, node, connect);
        } break;
        case ASTNodeType_IdentifierExpression: {
            renderToken(r, node->identifierExpressionNode.value, connect);
        } break;
        case ASTNodeType_BinaryExpression: {
            renderExpression(r, node->binaryExpressionNode.left, SPACE);
            renderString(r, tokenTypeToString(node->binaryExpressionNode.operator), SPACE);
            renderExpression(r, node->binaryExpressionNode.right, connect);
        } break;
        case ASTNodeType_TupleExpression: {
            ASTNodeTupleExpression *tuple = &node->tupleExpressionNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("("), NONE);
            ASTNodeLink *element = tuple->elements.head;
            for(u32 i = 0; i < tuple->elements.count; i++, element = element->next) {
                if(element->node.type != ASTNodeType_None) {
                    renderExpression(r, &element->node, NONE);
                }
                if(i < tuple->elements.count - 1) {
                    renderTokenChecked(r, element->node.endToken + 1, LIT_TO_STR(","), SPACE);
                }
            }
            renderTokenChecked(r, node->endToken, LIT_TO_STR(")"), connect);
        } break;
        case ASTNodeType_UnaryExpression: {
            renderString(r, tokenTypeToString(node->unaryExpressionNode.operator), NONE);
            renderExpression(r, node->unaryExpressionNode.subExpression, connect);
        } break;
        case ASTNodeType_UnaryExpressionPostfix: {
            renderExpression(r, node->unaryExpressionNode.subExpression, NONE);
            renderString(r, tokenTypeToString(node->unaryExpressionNode.operator), connect);
        } break;
        case ASTNodeType_NewExpression: {
            assert(stringMatch(LIT_TO_STR("new"), r->tokens.tokenStrings[node->startToken]));
            renderToken(r, node->startToken, SPACE);
            renderType(r, node->newExpressionNode.typeName, connect);
        } break;
        case ASTNodeType_FunctionCallExpression: {
            ASTNodeFunctionCallExpression *function = &node->functionCallExpressionNode;

            renderExpression(r, function->expression, NONE);
            renderCallArgumentList(r, function->expression->endToken + 1, &function->argumentsExpression, &function->argumentsName, connect);
        } break;
        case ASTNodeType_BaseType: {
            renderToken(r, node->baseTypeNode.typeName, connect);
        } break;
        case ASTNodeType_MemberAccessExpression: {
            ASTNodeMemberAccessExpression *member = &node->memberAccessExpressionNode;
            renderExpression(r, member->expression, DOT);
            renderToken(r, member->memberName, connect);
        } break;
        case ASTNodeType_ArrayAccessExpression: {
            ASTNodeArrayAccessExpression *array = &node->arrayAccessExpressionNode;
            renderExpression(r, array->expression, NONE);
            renderTokenChecked(r, array->expression->endToken + 1, LIT_TO_STR("["), NONE);
            if(array->indexExpression != 0x0) {
                renderExpression(r, array->indexExpression, NONE);
            }
            renderTokenChecked(r, node->endToken, LIT_TO_STR("]"), connect);
        } break;
        case ASTNodeType_ArraySliceExpression: {
            ASTNodeArraySliceExpression *array = &node->arraySliceExpressionNode;
            renderExpression(r, array->expression, NONE);
            renderTokenChecked(r, array->expression->endToken + 1, LIT_TO_STR("["), NONE);
            if(array->leftFenceExpression != 0x0) {
                renderExpression(r, array->leftFenceExpression, NONE);
                renderTokenChecked(r, array->leftFenceExpression->endToken + 1, LIT_TO_STR(":"), NONE);
            } else {
                renderTokenChecked(r, array->expression->endToken + 2, LIT_TO_STR(":"), NONE);
            }
            if(array->rightFenceExpression != 0x0) {
                renderExpression(r, array->rightFenceExpression, NONE);
            }
            renderTokenChecked(r, node->endToken, LIT_TO_STR("]"), connect);
        } break;
        case ASTNodeType_InlineArrayExpression: {
            ASTNodeInlineArrayExpression *array = &node->inlineArrayExpressionNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("["), NONE);
            ASTNodeLink *expression = array->expressions.head;
            for(u32 i = 0; i < array->expressions.count; i++, expression = expression->next) {
                ConnectType connect = i == array->expressions.count - 1 ? NONE : COMMA_SPACE;
                renderExpression(r, &expression->node, connect);
            }
            renderTokenChecked(r, node->endToken, LIT_TO_STR("]"), connect);
        } break;
        case ASTNodeType_TerneryExpression: {
            ASTNodeTerneryExpression *ternery = &node->terneryExpressionNode;
            renderExpression(r, ternery->condition, SPACE);
            renderTokenChecked(r, ternery->condition->endToken + 1, LIT_TO_STR("?"), SPACE);
            renderExpression(r, ternery->trueExpression, SPACE);
            renderTokenChecked(r, ternery->trueExpression->endToken + 1, LIT_TO_STR(":"), SPACE);
            renderExpression(r, ternery->falseExpression, connect);
        } break;
        case ASTNodeType_NamedParameterExpression: {
            ASTNodeNamedParametersExpression *named = &node->namedParametersExpressionNode;
            renderExpression(r, named->expression, NONE);

            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[named->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[node->endToken]));

            renderToken(r, named->expression->endToken + 1, SPACE);
            ASTNodeLink *expression = named->expressions.head;
            for(u32 i = 0; i < named->expressions.count; i++, expression = expression->next) {
                ConnectType connect = i == named->expressions.count - 1 ? SPACE : COMMA_SPACE;
                renderToken(r, listGetTokenId(&named->names, i), COLON_SPACE);
                renderExpression(r, &expression->node, connect);
            }
            renderToken(r, node->endToken, connect);
        } break;
        default: {
            assert(0);
        }
    }
}

static void
renderType(Render *r, ASTNode *node, ConnectType connect) {
    switch(node->type) {
        case ASTNodeType_BaseType: {
            ConnectType tokenConnect = node->baseTypeNode.payable ? SPACE : connect;
            renderToken(r, node->baseTypeNode.typeName, tokenConnect);
            if(node->baseTypeNode.payable) {
                assert(stringMatch(LIT_TO_STR("payable"), r->tokens.tokenStrings[node->endToken]));
                renderToken(r, node->endToken, connect);
            }
        } break;
        case ASTNodeType_IdentifierPath: {
            for(u32 i = 0; i < node->identifierPathNode.identifiers.count; i++) {
                TokenId part = listGetTokenId(&node->identifierPathNode.identifiers, i);
                renderToken(r, part, i == node->identifierPathNode.identifiers.count - 1 ? connect : DOT);
            }
        } break;
        case ASTNodeType_MappingType: {
            assert(stringMatch(LIT_TO_STR("mapping"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));
            renderToken(r, node->startToken, NONE);
            renderToken(r, node->startToken + 1, NONE);
            renderType(r, node->mappingNode.keyType, SPACE);
            if(node->mappingNode.keyIdentifier != INVALID_TOKEN_ID) {
                renderToken(r, node->mappingNode.keyIdentifier, SPACE);
            }

            renderTokenChecked(r, node->mappingNode.valueType->startToken - 2, LIT_TO_STR("="), NONE);
            renderTokenChecked(r, node->mappingNode.valueType->startToken - 1, LIT_TO_STR(">"), SPACE);

            renderType(r, node->mappingNode.valueType, node->mappingNode.valueIdentifier == INVALID_TOKEN_ID ? NONE : SPACE);
            if(node->mappingNode.valueIdentifier != INVALID_TOKEN_ID) {
                renderToken(r, node->mappingNode.valueIdentifier, NONE);
            }
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));
            renderToken(r, node->endToken, connect);
        } break;
        case ASTNodeType_ArrayType: {
            renderType(r, node->arrayTypeNode.elementType, NONE);

            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[node->arrayTypeNode.elementType->endToken + 1]));
            renderToken(r, node->arrayTypeNode.elementType->endToken + 1, NONE);
            if(node->arrayTypeNode.lengthExpression != 0x0) {
                renderExpression(r, node->arrayTypeNode.lengthExpression, NONE);
            }
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));
            renderToken(r, node->endToken, connect);
        } break;
        case ASTNodeType_FunctionType: {
            assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));

            renderToken(r, node->startToken, SPACE);
            renderToken(r, node->startToken + 1, NONE);
            renderParameters(r, &node->functionTypeNode.parameters, COMMA_SPACE, 0);
            if(node->functionTypeNode.parameters.count > 0) {
                renderTokenChecked(r, node->functionTypeNode.parameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
            } else {
                renderTokenChecked(r, node->startToken + 2, LIT_TO_STR(")"), SPACE);
            }

            if(node->functionTypeNode.visibility != INVALID_TOKEN_ID) {
                renderToken(r, node->functionTypeNode.visibility, SPACE);
            }
            if(node->functionTypeNode.stateMutability != INVALID_TOKEN_ID) {
                renderToken(r, node->functionTypeNode.stateMutability, SPACE);
            }

            if(node->functionTypeNode.returnParameters.count > 0) {
                renderTokenChecked(r, node->functionTypeNode.returnParameters.head->node.endToken - 2, LIT_TO_STR("returns"), SPACE);
                renderTokenChecked(r, node->functionTypeNode.returnParameters.head->node.endToken - 1, LIT_TO_STR("("), NONE);
                renderParameters(r, &node->functionTypeNode.returnParameters, COMMA_SPACE, 0);
                renderTokenChecked(r, node->functionTypeNode.returnParameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
            }

            assert(connect == SPACE);
        } break;
        default: {
            assert(0);
        }
    }
}

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
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));
            pushTokenWord(r, node->startToken);
            pushTokenWord(r, node->startToken + 1);

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
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_ArrayType: {
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[node->arrayTypeNode.elementType->endToken + 1]));
            pushTypeDocument(r, node->arrayTypeNode.elementType);

            pushTokenWord(r, node->arrayTypeNode.elementType->endToken + 1);
            if(node->arrayTypeNode.lengthExpression != 0x0) {
                assert(false);
                // renderExpression(r, node->arrayTypeNode.lengthExpression, NONE);
            }
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_FunctionType: {
            // assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[node->startToken]));
            // assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));

            // renderToken(r, node->startToken, SPACE);
            // renderToken(r, node->startToken + 1, NONE);
            // renderParameters(r, &node->functionTypeNode.parameters, COMMA_SPACE, 0);
            // if(node->functionTypeNode.parameters.count > 0) {
            //     renderTokenChecked(r, node->functionTypeNode.parameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
            // } else {
            //     renderTokenChecked(r, node->startToken + 2, LIT_TO_STR(")"), SPACE);
            // }

            // if(node->functionTypeNode.visibility != INVALID_TOKEN_ID) {
            //     renderToken(r, node->functionTypeNode.visibility, SPACE);
            // }
            // if(node->functionTypeNode.stateMutability != INVALID_TOKEN_ID) {
            //     renderToken(r, node->functionTypeNode.stateMutability, SPACE);
            // }

            // if(node->functionTypeNode.returnParameters.count > 0) {
            //     renderTokenChecked(r, node->functionTypeNode.returnParameters.head->node.endToken - 2, LIT_TO_STR("returns"), SPACE);
            //     renderTokenChecked(r, node->functionTypeNode.returnParameters.head->node.endToken - 1, LIT_TO_STR("("), NONE);
            //     renderParameters(r, &node->functionTypeNode.returnParameters, COMMA_SPACE, 0);
            //     renderTokenChecked(r, node->functionTypeNode.returnParameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
            // }

            // assert(connect == SPACE);
            assert(false);
        } break;
        default: {
            assert(0);
        }
    }
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
            // TODO(radomski): Escaping

            if(node->type == ASTNodeType_HexStringLitExpression) {
                pushWord(r, wordText(LIT_TO_STR("hex")));
            } else if(node->type == ASTNodeType_UnicodeStringLitExpression) {
                pushWord(r, wordText(LIT_TO_STR("unicode")));
            }
            pushWord(r, wordText(LIT_TO_STR("\"")));
            for(u32 i = 0; i < expression->values.count; i++) {
                TokenId literal = listGetTokenId(&expression->values, i);
                pushTokenWord(r, literal);
            }
            pushWord(r, wordText(LIT_TO_STR("\"")));
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
            pushExpressionDocument(r, node->binaryExpressionNode.left);
            pushWord(r, wordSpace());

            // TODO(radomski): Use the token
            pushWord(r, wordText(tokenTypeToString(node->binaryExpressionNode.operator)));
            pushWord(r, wordLine());

            pushExpressionDocument(r, node->binaryExpressionNode.right);
        } break;
        case ASTNodeType_TupleExpression: {
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));

            ASTNodeTupleExpression *tuple = &node->tupleExpressionNode;

            pushTokenWord(r, node->startToken);
            ASTNodeLink *element = tuple->elements.head;
            for(u32 i = 0; i < tuple->elements.count; i++, element = element->next) {
                if(element->node.type != ASTNodeType_None) {
                    pushExpressionDocument(r, &element->node);
                }

                if(i < tuple->elements.count - 1) {
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[element->node.endToken + 1]));
                    pushTokenWord(r, element->node.endToken + 1);
                    pushWord(r, wordLine());
                }
            }
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_UnaryExpression: {
            pushTokenWord(r, node->startToken);
            pushExpressionDocument(r, node->unaryExpressionNode.subExpression);

            assert(stringMatch(tokenTypeToString(node->unaryExpressionNode.operator), getTokenString(r->tokens, node->startToken)));
        } break;
        case ASTNodeType_UnaryExpressionPostfix: {
            pushExpressionDocument(r, node->unaryExpressionNode.subExpression);
            pushTokenWord(r, node->endToken);
            assert(stringMatch(tokenTypeToString(node->unaryExpressionNode.operator), getTokenString(r->tokens, node->endToken)));
        } break;
        case ASTNodeType_NewExpression: {
            // assert(stringMatch(LIT_TO_STR("new"), r->tokens.tokenStrings[node->startToken]));
            // renderToken(r, node->startToken, SPACE);
            // renderType(r, node->newExpressionNode.typeName, connect);
            assert(false);
        } break;
        case ASTNodeType_FunctionCallExpression: {
            ASTNodeFunctionCallExpression *function = &node->functionCallExpressionNode;

            pushExpressionDocument(r, function->expression);
            pushCallArgumentListDocument(r, function->expression->endToken + 1, &function->argumentsExpression, &function->argumentsName);
        } break;
        case ASTNodeType_BaseType: {
            pushTokenWord(r, node->baseTypeNode.typeName);
        } break;
        case ASTNodeType_MemberAccessExpression: {
            ASTNodeMemberAccessExpression *member = &node->memberAccessExpressionNode;
            pushExpressionDocument(r, member->expression);
            pushTokenWord(r, member->memberName - 1);
            pushTokenWord(r, member->memberName);
        } break;
        case ASTNodeType_ArrayAccessExpression: {
            ASTNodeArrayAccessExpression *array = &node->arrayAccessExpressionNode;
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[array->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));

            pushExpressionDocument(r, array->expression);
            pushTokenWord(r, array->expression->endToken + 1);
            if(array->indexExpression != 0x0) {
                pushExpressionDocument(r, array->indexExpression);
            }
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_ArraySliceExpression: {
            // ASTNodeArraySliceExpression *array = &node->arraySliceExpressionNode;
            // renderExpression(r, array->expression, NONE);
            // renderTokenChecked(r, array->expression->endToken + 1, LIT_TO_STR("["), NONE);
            // if(array->leftFenceExpression != 0x0) {
            //     renderExpression(r, array->leftFenceExpression, NONE);
            //     renderTokenChecked(r, array->leftFenceExpression->endToken + 1, LIT_TO_STR(":"), NONE);
            // } else {
            //     renderTokenChecked(r, array->expression->endToken + 2, LIT_TO_STR(":"), NONE);
            // }
            // if(array->rightFenceExpression != 0x0) {
            //     renderExpression(r, array->rightFenceExpression, NONE);
            // }
            // renderTokenChecked(r, node->endToken, LIT_TO_STR("]"), connect);
            assert(false);
        } break;
        case ASTNodeType_InlineArrayExpression: {
            // ASTNodeInlineArrayExpression *array = &node->inlineArrayExpressionNode;

            // renderTokenChecked(r, node->startToken, LIT_TO_STR("["), NONE);
            // ASTNodeLink *expression = array->expressions.head;
            // for(u32 i = 0; i < array->expressions.count; i++, expression = expression->next) {
            //     ConnectType connect = i == array->expressions.count - 1 ? NONE : COMMA_SPACE;
            //     renderExpression(r, &expression->node, connect);
            // }
            // renderTokenChecked(r, node->endToken, LIT_TO_STR("]"), connect);
            assert(false);
        } break;
        case ASTNodeType_TerneryExpression: {
            // ASTNodeTerneryExpression *ternery = &node->terneryExpressionNode;
            // renderExpression(r, ternery->condition, SPACE);
            // renderTokenChecked(r, ternery->condition->endToken + 1, LIT_TO_STR("?"), SPACE);
            // renderExpression(r, ternery->trueExpression, SPACE);
            // renderTokenChecked(r, ternery->trueExpression->endToken + 1, LIT_TO_STR(":"), SPACE);
            // renderExpression(r, ternery->falseExpression, connect);
            assert(false);
        } break;
        case ASTNodeType_NamedParameterExpression: {
            // assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[named->expression->endToken + 1]));
            // assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[node->endToken]));

            // ASTNodeNamedParametersExpression *named = &node->namedParametersExpressionNode;
            // renderExpression(r, named->expression, NONE);

            // renderToken(r, named->expression->endToken + 1, SPACE);
            // ASTNodeLink *expression = named->expressions.head;
            // for(u32 i = 0; i < named->expressions.count; i++, expression = expression->next) {
            //     ConnectType connect = i == named->expressions.count - 1 ? SPACE : COMMA_SPACE;
            //     renderToken(r, listGetTokenId(&named->names, i), COLON_SPACE);
            //     renderExpression(r, &expression->node, connect);
            // }
            // renderToken(r, node->endToken, connect);
            assert(false);
        } break;
        default: {
            assert(0);
        }
    }
}

static void
renderVariableDeclaration(Render *r, ASTNode *node, ConnectType connect) {
    ASTNodeVariableDeclaration *decl = &node->variableDeclarationNode;
    renderType(r, decl->type, SPACE);
    if(decl->dataLocation != INVALID_TOKEN_ID) {
        renderToken(r, decl->dataLocation, SPACE);
    }
    renderToken(r, decl->name, connect);
}

static void
renderYulFunctionCall(Render *r, ASTNode *node, ConnectType connect) {
    ASTNodeYulFunctionCallExpression *function = &node->yulFunctionCallExpressionNode;

    renderToken(r, function->identifier, NONE);
    assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->identifier + 1]));
    renderToken(r, function->identifier + 1, NONE);
    ASTNodeLink *argument = function->arguments.head;
    for(u32 i = 0; i < function->arguments.count; i++, argument = argument->next) {
        ConnectType connect = i == function->arguments.count - 1 ? NONE : COMMA_SPACE;
        renderYulExpression(r, &argument->node, connect);
    }
    assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));
    renderToken(r, node->endToken, connect);
}

static void
renderYulExpression(Render *r, ASTNode *node, ConnectType connect) {
    switch(node->type) {
        case ASTNodeType_YulNumberLitExpression: {
            renderToken(r, node->yulNumberLitExpressionNode.value, connect);
        } break;
        case ASTNodeType_YulStringLitExpression: {
            // TODO(radomski): Escaping
            writeString(r->writer, LIT_TO_STR("\""));
            renderToken(r, node->yulHexStringLitExpressionNode.value, NONE);
            writeString(r->writer, LIT_TO_STR("\""));
            renderConnect(r, connect);
        } break;
        case ASTNodeType_YulHexNumberLitExpression: {
            renderToken(r, node->yulHexNumberLitExpressionNode.value, connect);
        } break;
        case ASTNodeType_YulBoolLitExpression: {
            renderToken(r, node->yulBoolLitExpressionNode.value, connect);
        } break;
        case ASTNodeType_YulHexStringLitExpression: {
            // TODO(radomski): Escaping
            writeString(r->writer, LIT_TO_STR("hex\""));
            renderToken(r, node->yulHexStringLitExpressionNode.value, NONE);
            writeString(r->writer, LIT_TO_STR("\""));
            renderConnect(r, connect);
        } break;
        case ASTNodeType_YulMemberAccessExpression: {
            for(u32 i = 0; i < node->yulIdentifierPathExpressionNode.count; i++) {
                ConnectType connection = i == node->yulIdentifierPathExpressionNode.count - 1 ? connect : DOT;
                renderToken(r, node->yulIdentifierPathExpressionNode.identifiers[i], connection);
            }
        } break;
        case ASTNodeType_YulFunctionCallExpression: {
            renderYulFunctionCall(r, node, connect);
        } break;
        default: {
            javascriptPrintNumber(node->type);
            assert(0);
        }
    }
}

static void
renderStatement(Render *r, ASTNode *node, ConnectType connect) {
    switch(node->type) {
        case ASTNodeType_BlockStatement: {
            pushIndent(r->writer);

            ASTNodeBlockStatement *block = &node->blockStatementNode;
            renderToken(r, node->startToken, block->statements.count > 0 ? NEWLINE : SPACE);

            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) {
                    preservePresentNewLines(r, &statement->node);
                }

                renderStatement(r, &statement->node, NONE);
            }

            popIndent(r->writer);
            renderToken(r, node->endToken, connect);
            return;
        } break;
        case ASTNodeType_UncheckedBlockStatement: {
            ASTNodeUncheckedBlockStatement *unchecked = &node->uncheckedBlockStatementNode;
            assert(stringMatch(LIT_TO_STR("unchecked"), r->tokens.tokenStrings[node->startToken]));
            renderToken(r, node->startToken, SPACE);
            renderStatement(r, unchecked->block, NEWLINE);
        } break;
        case ASTNodeType_ReturnStatement: {
            assert(stringMatch(LIT_TO_STR("return"), r->tokens.tokenStrings[node->startToken]));
            renderToken(r, node->startToken, node->returnStatementNode.expression != 0x0 ? SPACE : SEMICOLON);
            if(node->returnStatementNode.expression != 0x0) {
                renderExpression(r, node->returnStatementNode.expression, SEMICOLON);
            }
        } break;
        case ASTNodeType_ExpressionStatement: {
            if(node->expressionStatementNode.expression != 0x0) {
                renderExpression(r, node->expressionStatementNode.expression, SEMICOLON);
            }
        } break;
        case ASTNodeType_IfStatement: {
            renderTokenChecked(r, node->startToken, LIT_TO_STR("if"), NONE);
            renderTokenChecked(r, node->startToken + 1, LIT_TO_STR("("), NONE);
            renderExpression(r, node->ifStatementNode.conditionExpression, NONE);
            renderTokenChecked(r, node->ifStatementNode.conditionExpression->endToken + 1, LIT_TO_STR(")"), SPACE);
            ConnectType trueConnect = node->ifStatementNode.trueStatement->type == ASTNodeType_BlockStatement ? NEWLINE : NONE;
            renderStatement(r, node->ifStatementNode.trueStatement, node->ifStatementNode.falseStatement ? NONE : trueConnect);
            if(node->ifStatementNode.falseStatement) {
                renderTokenChecked(r, node->ifStatementNode.falseStatement->startToken - 1, LIT_TO_STR("else"), SPACE);
                ConnectType falseConnect = node->ifStatementNode.falseStatement->type == ASTNodeType_BlockStatement ? NEWLINE : NONE;
                renderStatement(r, node->ifStatementNode.falseStatement, falseConnect);
            }
        } break;
        case ASTNodeType_VariableDeclarationStatement: {
            ASTNodeVariableDeclarationStatement *statement = &node->variableDeclarationStatementNode;

            renderVariableDeclaration(r, statement->variableDeclaration, statement->initialValue != 0x0 ? SPACE : SEMICOLON);
            if(statement->initialValue != 0x0) {
                renderTokenChecked(r, statement->initialValue->startToken - 1, LIT_TO_STR("="), SPACE);
                renderExpression(r, statement->initialValue, SEMICOLON);
            }
        } break;
        case ASTNodeType_VariableDeclarationTupleStatement: {
            ASTNodeVariableDeclarationTupleStatement *statement = &node->variableDeclarationTupleStatementNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("("), NONE);
            ASTNodeLink *decl = statement->declarations.head;
            for(u32 i = 0; i < statement->declarations.count; i++, decl = decl->next) {
                if(decl->node.type != ASTNodeType_None) {
                    ConnectType innerConnect = i == statement->declarations.count - 1 ? connect : COMMA_SPACE;
                    renderVariableDeclaration(r, &decl->node, innerConnect);
                }
            }
            renderTokenChecked(r, statement->initialValue->startToken - 2, LIT_TO_STR(")"), SPACE);
            renderTokenChecked(r, statement->initialValue->startToken - 1, LIT_TO_STR("="), SPACE);

            renderExpression(r, statement->initialValue, SEMICOLON);
        } break;
        case ASTNodeType_DoWhileStatement: {
            ASTNodeWhileStatement *statement = &node->whileStatementNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("do"), SPACE);
            renderStatement(r, statement->body, SPACE);
            renderTokenChecked(r, statement->expression->startToken - 2, LIT_TO_STR("while"), NONE);
            renderTokenChecked(r, statement->expression->startToken - 1, LIT_TO_STR("("), NONE);
            renderExpression(r, statement->expression, NONE);
            renderTokenChecked(r, node->endToken - 1, LIT_TO_STR(")"), SEMICOLON);
        } break;
        case ASTNodeType_WhileStatement: {
            ASTNodeWhileStatement *statement = &node->whileStatementNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("while"), NONE);
            renderTokenChecked(r, node->startToken + 1, LIT_TO_STR("("), NONE);
            renderExpression(r, statement->expression, NONE);
            renderTokenChecked(r, statement->expression->endToken + 1, LIT_TO_STR(")"), SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_ForStatement: {
            ASTNodeForStatement *statement = &node->forStatementNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("for"), NONE);
            renderTokenChecked(r, node->startToken + 1, LIT_TO_STR("("), NONE);

            if(statement->variableStatement != 0x0) {
                renderStatement(r, statement->variableStatement, NONE);
            }
            if(statement->conditionExpression != 0x0) {
                renderExpression(r, statement->conditionExpression, NONE);
            }
            renderCString(r, ";", SPACE);
            if(statement->incrementExpression != 0x0) {
                renderExpression(r, statement->incrementExpression, NONE);
            }

            renderTokenChecked(r, statement->body->startToken - 1, LIT_TO_STR(")"), SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_RevertStatement: {
            ASTNodeRevertStatement *statement = &node->revertStatementNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("revert"), SPACE);
            renderExpression(r, statement->expression, SEMICOLON);
        } break;
        case ASTNodeType_EmitStatement: {
            ASTNodeEmitStatement *statement = &node->emitStatementNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("emit"), SPACE);
            renderExpression(r, statement->expression, SEMICOLON);
        } break;
        case ASTNodeType_TryStatement: {
            ASTNodeTryStatement *statement = &node->tryStatementNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("try"), SPACE);

            renderExpression(r, statement->expression, SPACE);
            renderTokenChecked(r, statement->expression->endToken + 1, LIT_TO_STR("returns"), SPACE);
            renderTokenChecked(r, statement->expression->endToken + 2, LIT_TO_STR("("), NONE);
            renderParameters(r, &statement->returnParameters, COMMA_SPACE, 0);
            renderTokenChecked(r, statement->body->startToken - 1, LIT_TO_STR(")"), SPACE);
            renderStatement(r, statement->body, statement->catches.count > 0 ? SPACE : NEWLINE);

            ASTNodeLink *catchLink = statement->catches.head;
            for(u32 i = 0; i < statement->catches.count; i++, catchLink = catchLink->next) {
                renderTokenChecked(r, catchLink->node.startToken, LIT_TO_STR("catch"), SPACE);
                ASTNodeCatchStatement *catch = &catchLink->node.catchStatementNode;

                u32 openParenToken = catchLink->node.startToken + 1;
                if(catch->identifier != INVALID_TOKEN_ID) {
                    renderToken(r, catch->identifier, NONE);
                    openParenToken = catch->identifier + 1;
                }

                if(catch->parameters.count != -1) {
                    renderTokenChecked(r, openParenToken, LIT_TO_STR("("), NONE);
                    renderParameters(r, &catch->parameters, COMMA_SPACE, 0);
                    renderTokenChecked(r, catch->body->startToken - 1, LIT_TO_STR(")"), SPACE);
                }
                renderStatement(r, catch->body, i == statement->catches.count - 1 ? NEWLINE : SPACE);
            }
        } break;
        case ASTNodeType_BreakStatement: {
            renderTokenChecked(r, node->startToken, LIT_TO_STR("break"), SEMICOLON);
        } break;
        case ASTNodeType_ContinueStatement: {
            renderTokenChecked(r, node->startToken, LIT_TO_STR("continue"), SEMICOLON);
        } break;
        case ASTNodeType_AssemblyStatement: {
            ASTNodeAssemblyStatement *statement = &node->assemblyStatementNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("assembly"), SPACE);
            if(statement->isEVMAsm) {
                renderCString(r, "\"evmasm\"", SPACE);
            }

            if(statement->flags.count > 0) {
                renderCString(r, "(", NONE);
                for(u32 i = 0; i < statement->flags.count; i++) {
                    writeString(r->writer, LIT_TO_STR("\""));
                    renderToken(r, listGetTokenId(&statement->flags, i), NONE);
                    writeString(r->writer, LIT_TO_STR("\""));
                    if(i != statement->flags.count - 1) {
                        renderCString(r, ",", SPACE);
                    }
                }
                renderCString(r, ")", SPACE);
            }

            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_YulBlockStatement: {
            ASTNodeBlockStatement *block = &node->blockStatementNode;
            renderToken(r, node->startToken, block->statements.count > 0 ? NEWLINE : SPACE);

            pushIndent(r->writer);
            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) {
                    preservePresentNewLines(r, &statement->node);
                }
                renderStatement(r, &statement->node, NONE);
            }
            popIndent(r->writer);

            renderToken(r, node->endToken, connect);
            return;
        } break;
        case ASTNodeType_YulVariableDeclaration: {
            ASTNodeYulVariableDeclaration *statement = &node->yulVariableDeclarationNode;
            renderTokenChecked(r, node->startToken, LIT_TO_STR("let"), SPACE);

            ConnectType valueConnect = statement->value != 0x0 ? SPACE : NEWLINE;
            for(u32 i = 0; i < statement->identifiers.count; i++) {
                ConnectType connect = i == statement->identifiers.count - 1 ? valueConnect : COMMA_SPACE;
                renderToken(r, listGetTokenId(&statement->identifiers, i), connect);
            }

            if(statement->value != 0x0) {
                renderTokenChecked(r, statement->value->startToken - 2, LIT_TO_STR(":"), NONE);
                renderTokenChecked(r, statement->value->startToken - 1, LIT_TO_STR("="), SPACE);
                renderYulExpression(r, statement->value, NEWLINE);
            }
        } break;
        case ASTNodeType_YulVariableAssignment: {
            ASTNodeYulVariableAssignment *statement = &node->yulVariableAssignmentNode;

            ASTNodeLink *it = statement->paths.head;
            ConnectType valueConnect = statement->value != 0x0 ? SPACE : NEWLINE;
            for(u32 i = 0; i < statement->paths.count; i++, it = it->next) {
                ConnectType connect = i == statement->paths.count - 1 ? valueConnect : COMMA_SPACE;
                renderYulExpression(r, &it->node, connect);
            }

            if(statement->value != 0x0) {
                renderTokenChecked(r, statement->value->startToken - 2, LIT_TO_STR(":"), NONE);
                renderTokenChecked(r, statement->value->startToken - 1, LIT_TO_STR("="), SPACE);
                renderYulExpression(r, statement->value, NEWLINE);
            }
        } break;
        case ASTNodeType_YulFunctionCallExpression: {
            renderYulFunctionCall(r, node, NEWLINE);
        } break;
        case ASTNodeType_YulIfStatement: {
            ASTNodeYulIfStatement *statement = &node->yulIfStatementNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("if"), SPACE);
            renderYulExpression(r, statement->expression, SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_YulForStatement: {
            ASTNodeYulForStatement *statement = &node->yulForStatementNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("for"), SPACE);
            renderStatement(r, statement->variableDeclaration, SPACE);
            renderYulExpression(r, statement->condition, SPACE);
            renderStatement(r, statement->increment, SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_YulFunctionDefinition: {
            ASTNodeYulFunctionDefinition *statement = &node->yulFunctionDefinitionNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("function"), SPACE);
            renderToken(r, statement->identifier, statement->parameters.count > 0 ? NONE : SPACE);
            if(statement->parameters.count > 0) {
                renderCString(r, "(", NONE);
                for(u32 i = 0; i < statement->parameters.count; i++) {
                    ConnectType connect = i == statement->parameters.count - 1 ? NONE : COMMA_SPACE;
                    renderToken(r, listGetTokenId(&statement->parameters, i), connect);
                }
                renderCString(r, ")", SPACE);
            }

            if(statement->returnParameters.count > 0) {
                renderCString(r, "->", SPACE);
                for(u32 i = 0; i < statement->returnParameters.count; i++) {
                    ConnectType connect = i == statement->returnParameters.count - 1 ? SPACE : COMMA_SPACE;
                    renderToken(r, listGetTokenId(&statement->returnParameters, i), connect);
                }
            }

            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_YulSwitchStatement: {
            ASTNodeYulSwitchStatement *statement = &node->yulSwitchStatementNode;

            renderTokenChecked(r, node->startToken, LIT_TO_STR("switch"), SPACE);
            ConnectType expressionConnect = statement->cases.count > 0 || statement->defaultBlock != 0x0 ? NEWLINE : SPACE;
            renderYulExpression(r, statement->expression, expressionConnect);

            ASTNodeLink *it = statement->cases.head;
            for(u32 i = 0; i < statement->cases.count; i++, it = it->next) {
                renderTokenChecked(r, it->node.yulCaseNode.literal->startToken - 1, LIT_TO_STR("case"), SPACE);
                renderYulExpression(r, it->node.yulCaseNode.literal, SPACE);
                renderStatement(r, it->node.yulCaseNode.block, NEWLINE);
            }

            if(statement->defaultBlock != 0x0) {
                renderTokenChecked(r, statement->defaultBlock->yulCaseNode.block->startToken - 1, LIT_TO_STR("default"), SPACE);
                renderStatement(r, statement->defaultBlock->yulCaseNode.block, NEWLINE);
            }
        } break;
        case ASTNodeType_YulLeaveStatement: {
            renderTokenChecked(r, node->startToken, LIT_TO_STR("leave"), NEWLINE);
        } break;
        case ASTNodeType_YulBreakStatement: {
            renderTokenChecked(r, node->startToken, LIT_TO_STR("break"), NEWLINE);
        } break;
        case ASTNodeType_YulContinueStatement: {
            renderTokenChecked(r, node->startToken, LIT_TO_STR("continue"), NEWLINE);
        } break;
        default: {
            javascriptPrintString("Unreachable, unhandled statement type = ");
            javascriptPrintNumber(node->type);
            assert(0);
        }
    }

    renderConnect(r, connect);
}

static void
renderParameters(Render *r, ASTNodeListRanged *list, ConnectType connect, u32 connectIsInclusive) {
    ASTNodeLink *it = list->head;
    for(u32 i = 0; i < list->count; i++, it = it->next) {
        if(connectIsInclusive) {
            preservePresentNewLines(r, &it->node);
        }
        ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;
        ConnectType connections[3] = { SPACE, SPACE, SPACE };
        if(!connectIsInclusive && i == list->count - 1) {
            connect = NONE;
        }

        if(decl->name != INVALID_TOKEN_ID) {
            connections[2] = connect;
        } else {
            if(decl->dataLocation != INVALID_TOKEN_ID) {
                connections[1] = connect;
            } else {
                connections[0] = connect;
            }
        }

        renderType(r, decl->type, connections[0]);

        if(decl->dataLocation != INVALID_TOKEN_ID) {
            renderToken(r, decl->dataLocation, connections[1]);
        }

        if(decl->name != INVALID_TOKEN_ID) {
            renderToken(r, decl->name, connections[2]);
        }
    }
}

static void
renderInheritanceSpecifier(Render *r, ASTNode *node, ConnectType connect) {
    ASTNodeInheritanceSpecifier *inheritance = &node->inheritanceSpecifierNode;
    renderType(r, inheritance->identifier, inheritance->argumentsExpression.count > 0 ? NONE : connect);
    if(inheritance->argumentsExpression.count > 0) {
        renderCallArgumentList(r, inheritance->identifier->endToken + 1, &inheritance->argumentsExpression, &inheritance->argumentsName, connect);
    }
}

static void
renderModiferInvocations(Render *r, ASTNodeList *modifiers) {
    if(modifiers && modifiers->count > 0) {
        ASTNodeLink *it = modifiers->head;
        for(u32 i = 0; i < modifiers->count; i++, it = it->next) {
            ASTNodeModifierInvocation *invocation = &it->node.modifierInvocationNode;
            renderType(r, invocation->identifier, invocation->argumentsExpression.count != -1 ? NONE : SPACE);
            if(invocation->argumentsExpression.count != -1) {
                renderCallArgumentList(r, invocation->identifier->endToken + 1, &invocation->argumentsExpression, &invocation->argumentsName, SPACE);
            }
        }
    }
}

static void
renderOverrides(Render *r, TokenId overrideToken, ASTNodeListRanged *overrides) {
    if(overrideToken != INVALID_TOKEN_ID) {
        renderTokenChecked(r, overrideToken, LIT_TO_STR("override"), overrides->count > 0 ? NONE : SPACE);
        if(overrides->count > 0) {
            renderTokenChecked(r, overrides->startToken - 1, LIT_TO_STR("("), NONE);
            ASTNodeLink *override = overrides->head;
            for(u32 i = 0; i < overrides->count; i++, override = override->next) {
                renderType(r, &override->node, i == overrides->count - 1 ? NONE : COMMA_SPACE);
            }
            renderTokenChecked(r, overrides->endToken + 1, LIT_TO_STR(")"), SPACE);
        }
    }
}

static void
renderMember(Render *r, ASTNode *member) {
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
            pushWord(r, wordHardline());

            renderDocument(r);
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
                popNestWithLastWord(r);

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
            pushWord(r, wordHardline());

            renderDocument(r);
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
            pushWord(r, wordHardline());

            renderDocument(r);
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


                // TODO(radomski): This shouldn't exist, we should understand
                // that there was a line comment that ended the line and no
                // line ending is necessary
                if(r->words[r->wordCount - 1].type != WordType_Hardline) {
                    pushWord(r, wordHardline());
                }
            }
            popNestWithLastWord(r);

            pushTokenWord(r, member->endToken);
            popGroup(r);
            pushWord(r, wordHardline());

            renderDocument(r);
        } break;
        case ASTNodeType_Struct: {
            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 2]));
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));

            pushGroup(r);

            pushTokenWord(r, member->startToken);
            pushWord(r, wordSpace());
            pushTokenWord(r, member->structNode.nameTokenId);
            pushWord(r, wordSpace());
            pushTokenWord(r, member->startToken + 2);
            pushNest(r);
            pushWord(r, wordLine());

            ASTNodeListRanged *list = &member->structNode.members;
            ASTNodeLink *it = list->head;
            for(u32 i = 0; i < list->count; i++, it = it->next) {
                ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;
                preserveHardlinesIntoDocument(r, &it->node);

                pushTypeDocument(r, decl->type);
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->name);
                pushTokenWord(r, decl->name + 1);
                pushWord(r, wordHardline());
            }

            popNestWithLastWord(r);

            pushTokenWord(r, member->endToken);

            popGroup(r);
            pushWord(r, wordHardline());
            renderDocument(r);
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
            popNestWithLastWord(r);

            pushTokenWord(r, member->endToken - 1);
            popGroup(r);
            pushTokenWord(r, member->endToken);
            trimGroupRight(r); // TODO(radomski): Remove

            pushWord(r, wordHardline());
            renderDocument(r);
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
            popNestWithLastWord(r);

            pushTokenWord(r, member->endToken - 1);
            popGroup(r);
            pushTokenWord(r, member->endToken);
            trimGroupRight(r); // TODO(radomski): Remove

            pushWord(r, wordHardline());
            renderDocument(r);

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

            trimGroupRight(r); // TODO(radomski): Remove

            popGroup(r);
            pushWord(r, wordHardline());

            renderDocument(r);
        } break;
        case ASTNodeType_ConstVariable: {
            ASTNodeConstVariable *constNode = &member->constVariableNode;
            assert(stringMatch(LIT_TO_STR("constant"), r->tokens.tokenStrings[constNode->identifier - 1]));
            assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[constNode->identifier + 1]));

            //renderType(r, constNode->type, SPACE);
            //renderToken(r, constNode->identifier - 1, SPACE);
            //renderToken(r, constNode->identifier, SPACE);
            //renderToken(r, constNode->identifier + 1, SPACE);
            //renderExpression(r, constNode->expression, SEMICOLON);

            pushGroup(r);
            pushTypeDocument(r, constNode->type);
            pushWord(r, wordSpace());

            pushTokenWord(r, constNode->identifier - 1);
            pushWord(r, wordSpace());
            pushTokenWord(r, constNode->identifier);
            pushWord(r, wordSpace());
            pushTokenWord(r, constNode->identifier + 1);
            pushWord(r, wordSpace());

            pushExpressionDocument(r, constNode->expression);
            pushTokenWord(r, member->endToken);

            trimGroupRight(r); // TODO(radomski): Remove

            popGroup(r);
            pushWord(r, wordHardline());

            renderDocument(r);
        } break;
        case ASTNodeType_StateVariableDeclaration: {
            ASTNodeConstVariable *decl = &member->constVariableNode;

            renderType(r, decl->type, SPACE);
            if(decl->visibility != INVALID_TOKEN_ID) {
                renderToken(r, decl->visibility, SPACE);
            }
            if(decl->mutability != INVALID_TOKEN_ID) {
                renderToken(r, decl->mutability, SPACE);
            }

            renderOverrides(r, decl->override, &decl->overrides);
            renderToken(r, decl->identifier, decl->expression ? SPACE : SEMICOLON);

            if(decl->expression) {
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[decl->identifier + 1]));
                renderToken(r, decl->identifier + 1, SPACE);
                renderExpression(r, decl->expression, SEMICOLON);
            }
        } break;
        case ASTNodeType_FallbackFunction:
        case ASTNodeType_ReceiveFunction:
        case ASTNodeType_FunctionDefinition: {
            TokenId openParenToken = member->startToken + 1;
            ASTNodeFunctionDefinition *function = &member->functionDefinitionNode;
            if(member->type == ASTNodeType_FallbackFunction) {
                assert(stringMatch(LIT_TO_STR("fallback"), r->tokens.tokenStrings[member->startToken]));
                renderToken(r, member->startToken, SPACE);
            } else if(member->type == ASTNodeType_ReceiveFunction) {
                assert(stringMatch(LIT_TO_STR("receive"), r->tokens.tokenStrings[member->startToken]));
                renderToken(r, member->startToken, SPACE);
            } else {
                assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[member->startToken]));
                if(function->name != INVALID_TOKEN_ID) {
                    renderToken(r, member->startToken, SPACE);
                    renderToken(r, function->name, NONE);
                    openParenToken = function->name + 1;
                } else {
                    renderToken(r, member->startToken, NONE);
                }
            }

            renderTokenChecked(r, openParenToken, LIT_TO_STR("("), NONE);
            renderParameters(r, &member->functionDefinitionNode.parameters, COMMA_SPACE, 0);
            if(member->functionDefinitionNode.parameters.count > 0) {
                renderTokenChecked(r, member->functionDefinitionNode.parameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
            } else {
                renderTokenChecked(r, openParenToken + 1, LIT_TO_STR(")"), SPACE);
            }

            if(function->visibility != INVALID_TOKEN_ID) {
                renderToken(r, function->visibility, SPACE);
            }
            if(function->stateMutability != INVALID_TOKEN_ID) {
                renderToken(r, function->stateMutability, SPACE);
            }

            if(function->virtual != INVALID_TOKEN_ID) {
                renderToken(r, function->virtual, SPACE);
            }

            renderOverrides(r, function->override, function->overrides);
            if(function->returnParameters) {
                renderTokenChecked(r, function->returnParameters->startToken - 2, LIT_TO_STR("returns"), SPACE);
                renderTokenChecked(r, function->returnParameters->startToken - 1, LIT_TO_STR("("), NONE);
                renderParameters(r, function->returnParameters, COMMA_SPACE, 0);
                renderTokenChecked(r, function->returnParameters->endToken + 1, LIT_TO_STR(")"), SPACE);
            }

            renderModiferInvocations(r, function->modifiers);

            if(function->body != 0x0) {
                renderStatement(r, function->body, NEWLINE);
            } else {
                popBytes(r->writer, 1);
                renderConnect(r, SEMICOLON);
            }
        } break;
        case ASTNodeType_ConstructorDefinition: {
            assert(stringMatch(LIT_TO_STR("constructor"), r->tokens.tokenStrings[member->startToken]));
            renderToken(r, member->startToken, NONE);
            ASTNodeConstructorDefinition *constructor = &member->constructorDefinitionNode;

            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[member->startToken + 1]));
            renderToken(r, member->startToken + 1, NONE);
            renderParameters(r, &constructor->parameters, COMMA_SPACE, 0);
            if(constructor->parameters.count > 0) {
                renderTokenChecked(r, constructor->parameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
            } else {
                renderTokenChecked(r, member->startToken + 2, LIT_TO_STR(")"), SPACE);
            }

            renderModiferInvocations(r, &constructor->modifiers);

            if(constructor->visibility != INVALID_TOKEN_ID) {
                renderToken(r, constructor->visibility, SPACE);
            }
            if(constructor->stateMutability != INVALID_TOKEN_ID) {
                renderToken(r, constructor->stateMutability, SPACE);
            }

            renderStatement(r, constructor->body, NEWLINE);
        } break;
        case ASTNodeType_ModifierDefinition: {
            assert(stringMatch(LIT_TO_STR("modifier"), r->tokens.tokenStrings[member->startToken]));
            renderToken(r, member->startToken, SPACE);
            ASTNodeFunctionDefinition *function = &member->functionDefinitionNode;
            renderToken(r, function->name, function->parameters.count != -1 ? NONE : SPACE);

            if(function->parameters.count != -1) {
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->name + 1]));
                renderToken(r, function->name + 1, NONE);
                renderParameters(r, &function->parameters, COMMA_SPACE, 0);
                if(function->parameters.count > 0) {
                    renderTokenChecked(r, function->parameters.last->node.endToken + 1, LIT_TO_STR(")"), SPACE);
                } else {
                    renderTokenChecked(r, function->name + 2, LIT_TO_STR(")"), SPACE);
                }
            }

            if(function->virtual != INVALID_TOKEN_ID) {
                renderToken(r, function->virtual, SPACE);
            }

            renderOverrides(r, function->override, function->overrides);
            if(function->body != 0x0) {
                renderStatement(r, function->body, NEWLINE);
            } else {
                popBytes(r->writer, 1);
                renderConnect(r, SEMICOLON);
            }
        } break;
        case ASTNodeType_LibraryDefinition:
        case ASTNodeType_ContractDefinition:
        case ASTNodeType_InterfaceDefinition:
        case ASTNodeType_AbstractContractDefinition: {
            switch(member->type) {
                case ASTNodeType_LibraryDefinition: {
                    assert(stringMatch(LIT_TO_STR("library"), r->tokens.tokenStrings[member->startToken]));
                    renderToken(r, member->startToken, SPACE);
                } break;
                case ASTNodeType_InterfaceDefinition: {
                    assert(stringMatch(LIT_TO_STR("interface"), r->tokens.tokenStrings[member->startToken]));
                    renderToken(r, member->startToken, SPACE);
                } break;
                case ASTNodeType_ContractDefinition: {
                    assert(stringMatch(LIT_TO_STR("contract"), r->tokens.tokenStrings[member->startToken]));
                    renderToken(r, member->startToken, SPACE);
                } break;
                case ASTNodeType_AbstractContractDefinition: {
                    assert(stringMatch(LIT_TO_STR("abstract"), r->tokens.tokenStrings[member->startToken]));
                    assert(stringMatch(LIT_TO_STR("contract"), r->tokens.tokenStrings[member->startToken + 1]));
                    renderToken(r, member->startToken, SPACE);
                    renderToken(r, member->startToken + 1, SPACE);
                }
            }
            ASTNodeContractDefinition *contract = &member->contractDefinitionNode;

            renderToken(r, contract->name, SPACE);
            if(contract->baseContracts.count > 0) {
                assert(stringMatch(LIT_TO_STR("is"), r->tokens.tokenStrings[contract->name + 1]));
                renderToken(r, contract->name + 1, SPACE);
            }
            ASTNodeLink *baseContract = contract->baseContracts.head;
            for(u32 i = 0; i < contract->baseContracts.count; i++, baseContract = baseContract->next) {
                ConnectType connect = i == contract->baseContracts.count - 1 ? SPACE : COMMA_SPACE;
                renderInheritanceSpecifier(r, &baseContract->node, connect);
            }

            pushIndent(r->writer);
            u32 openParenToken = contract->elements.count > 0 ? contract->elements.head->node.startToken - 1 : member->endToken - 1;
            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[openParenToken]));
            renderToken(r, openParenToken, contract->elements.count > 0 ? NEWLINE : SPACE);

            ASTNodeLink *element = contract->elements.head;
            for(u32 i = 0; i < contract->elements.count; i++, element = element->next) {
                if(i > 0) {
                    preservePresentNewLines(r, &element->node);
                }
                renderMember(r, &element->node);
            }

            popIndent(r->writer);
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));
            renderToken(r, member->endToken, NEWLINE);
        } break;
        default: {
            assert(0);
        } break;
    }
}

static void
renderSourceUnit(Render *r, ASTNode *tree) {
    ASTNodeLink *child = tree->children.head;
    for(u32 i = 0; i < tree->children.count; i++, child = child->next) {
        preservePresentNewLines(r, &child->node);
        renderMember(r, &child->node);
    }
}

static String
renderTree(Arena *arena, ASTNode tree, String originalSource, TokenizeResult tokens) {
    Writer writer = {
        .data = arrayPush(arena, u8, originalSource.size * 4),
        .size = 0,
        .capacity = originalSource.size * 4 ,
        .indentSize = 4,
    };

    Render render = {
        .writer = &writer,
        .tokens = tokens,
        .sourceBaseAddress = originalSource.data,

        // NOTE(radomski): This makes read and writes to (-1) possible
        .words = arrayPush(arena, Word, 131072) + 1,
        .wordCount = 0,
        .wordCapacity = 131071,
    };

    u32 startOfTokens = tokens.tokenStrings[tree.startToken].data - originalSource.data;
    renderComments(&render, 0, startOfTokens);
    renderSourceUnit(&render, &tree);
    return (String){ .data = writer.data, .size = writer.size };
}
