typedef struct Writer {
    u8 *data;
    u32 size;
    u32 capacity;

    u32 indentSize;
    u32 indentCount;
    u32 lineSize;
} Writer;

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
    Word trailingWhitespace;
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
setIndent(Writer *w, u32 indentCount) {
    w->indentCount = indentCount;
}

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

static bool
fits(Render *r, Word *words, s32 count, u32 group, u32 remainingWidth) {
    u32 width = 0;
    for (u32 i = 0; i < count && words[i].group >= group; i++) {
        Word *word = &words[i];
        if (word->group < group) {
            break;
        }

        if (word->group > group) {
            u32 nested_start_width = width;
            if (!fits(r, words + i, count - i, group + 1, remainingWidth - width)) {
                return false;
            }
            // Calculate minimum width taken by nested group
            u32 nested_min_width = 0;
            u32 nested_group_words = 0;
            for (u32 j = i; j < count && words[j].group > group; ++j) {
                if (words[j].type == WordType_Text) {
                    nested_min_width += words[j].text.size;
                } else if (words[j].type == WordType_Space || words[j].type == WordType_Line) {
                    nested_min_width += 1;
                }
                nested_group_words++;
            }
            width += nested_min_width;
            if (width > remainingWidth) {
                return false;
            }
            i += nested_group_words - 1;
            continue;
        }

        switch (word->type) {
            case WordType_Text: {
                width += word->text.size;
            } break;
            case WordType_Space:
            case WordType_Line:
            {
                width += 1;
            } break;
            case WordType_Hardline: {
                return false;
            } break;
            case WordType_None: {} break;
            default: break;
        }
        if (width > remainingWidth) {
            return false;
        }
    }
    return true;
}

static bool
debugDocumentEnabled() {
    // Check for env
    return getenv("DEBUG_DOCUMENT") != 0;
}

static void
debugPrintDocument(Word *words, u32 count, u32 group) {
    u32 index = 0;
    printf("G%2d:", group);

    for(u32 i = 0; i < count; i++) {
        Word *w = &words[i];

        if(w->group == group) {
            switch(w->type) {
                case WordType_Text:     { printf("%.*s", (int)w->text.size, w->text.data); } break;
                case WordType_Space:    { printf(" "); } break;
                case WordType_Line:     { printf("·"); } break;
                case WordType_Hardline: { printf("¶"); } break;
                case WordType_Softline: { printf("¬"); } break;
                default: break;
            }
        } else {
            switch(w->type) {
                case WordType_Text: { 
                    for(u32 j = 0; j < w->text.size; j++) {
                        printf(" ");
                    }
                } break;
                case WordType_Space:    { printf(" "); } break;
                case WordType_Line:     { printf(" "); } break;
                case WordType_Hardline: { printf(" "); } break;
                case WordType_Softline: { printf(" "); } break;
                default: break;
            }
        }
    }

    printf("\n");
}

static u32 renderGroup(Render *r, Word *words, s32 count, u32 group);
static u32 processGroupWords(Render *r, Word *words, s32 count, u32 group, WordRenderLineType lineType) {
    u32 processed = 0;
    for (u32 i = 0; i < (u32)count && words[i].group >= group;) {
        Word *word = &words[i];
        if (word->group > group) {
            u32 sub_processed = renderGroup(r, words + i, count - i, group + 1);
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
    u32 baseLineSize = w->lineSize;
    u32 baseWritten = w->size;

    u32 remainingWidth = 120 - w->lineSize;
    if (fits(r, words, count, group, remainingWidth)) {
        return processGroupWords(r, words, count, group, WordRenderLineType_Space);
    }

    w->size = baseWritten;
    w->lineSize = baseLineSize;
    
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
        case WordType_None: {} break;
        default: { assert(false); }
    }
}

static void
renderDocument(Render *r) {
    if(debugDocumentEnabled()) {
        u32 maxGroup = 0;
        for(u32 i = 0; i < r->wordCount; i++) {
            maxGroup = MAX(maxGroup, r->words[i].group);
        }

        for(u32 i = 0; i <= maxGroup; i++) {
            debugPrintDocument(r->words, r->wordCount, i);
        }
    }

    renderGroup(r, r->words, r->wordCount, 0);
    r->wordCount = 0;
}

static void pushWord(Render *r, Word w);

static void
pushGroup(Render *r) {
    r->group += 1;
    pushWord(r, (Word){ .type = WordType_None });
}

static void
popGroup(Render *r) {
    r->group -= 1;
    pushWord(r, (Word){ .type = WordType_None });
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
addNest(Render *r, s32 nest) {
    r->nest += nest;
}

static void
popNestWithLastWord(Render *r) {
    popNest(r);
    r->words[r->wordCount - 1].nest -= 1;
}

static void
pushWord(Render *r, Word w) {
    if(r->trailingWhitespace.type == WordType_Hardline && w.type != WordType_None) {
        r->words[r->wordCount++] = r->trailingWhitespace;
        r->trailingWhitespace = (Word){};
    } else {
        w.group = r->group;
        w.nest = r->nest;

        r->words[r->wordCount++] = w;
    }
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
pushCommentsAfterToken(Render *r, TokenId token) {
    TokenId nextToken = token == r->tokens.count - 1 ? token : token + 1; 
    String next = getTokenString(r->tokens, nextToken);
    String current = getTokenString(r->tokens, token);

    u32 startOffset = (current.data - r->sourceBaseAddress) + current.size;
    u32 endOffset = next.data ? next.data - r->sourceBaseAddress : startOffset;

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
                    commentEnd = newlineIndex + 1;
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

                    cursor = commentEnd;
                    i = cursor - 1;

                    // Write comment out
                    if(commentType == CommentType_SingleLine) {
                        pushWord(r, wordText(stringTrim(comment)));
                        pushWord(r, wordHardline());
                    } else if(commentType == CommentType_StarAligned) {
                        String line = { .data = comment.data, .size = 0 };
                        u32 lineCount = 0;
                        for(u32 i = 0; i < comment.size; i++) {
                            if(comment.data[i] == '\n') {
                                lineCount += 1;
                                if(lineCount > 1) { pushWord(r, wordSpace()); }
                                pushWord(r, wordText(stringTrim(line)));
                                pushWord(r, wordHardline());
                                line = (String){ .data = line.data + line.size + 1, .size = 0 };
                            } else {
                                line.size += 1;
                            }
                        }

                        if(line.size > 0) {
                            if(lineCount > 1) { pushWord(r, wordSpace()); }
                            pushWord(r, wordText(stringTrim(line)));
                        }

                        u8 *head = comment.data + comment.size;
                        while(isWhitespace(*head)) {
                            if(*head == '\n') {
                                pushWord(r, wordHardline());
                                break;
                            }
                            head++;
                        }
                    } else {
                        pushWord(r, wordText(stringTrim(comment)));

                        u8 *head = comment.data + comment.size;
                        while(isWhitespace(*head)) {
                            if(*head == '\n') {
                                pushWord(r, wordHardline());
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
            pushWord(r, wordHardline());
        } 
    }

    if(r->wordCount > 0 && r->words[r->wordCount - 1].type != WordType_Text) {
        r->trailingWhitespace = r->words[r->wordCount - 1];
        r->wordCount -= 1;
    }
}

static void
pushTokenWordOnly(Render *r, TokenId token) {
    Word w = {
        .type = WordType_Text,
        .text = getTokenString(r->tokens, token),
        .group = r->group,
        .nest = r->nest,
    };

    if(r->trailingWhitespace.type == WordType_Hardline) {
        pushWord(r, r->trailingWhitespace);
        r->trailingWhitespace = (Word){};
    }

    pushWord(r, w);
}

static void
pushTokenWord(Render *r, TokenId token) {
    pushTokenWordOnly(r, token);
    pushCommentsAfterToken(r, token);
}

static void
pushTokenAsStringWordOnly(Render *r, TokenId token) {
    String text = getTokenString(r->tokens, token);
    assert(text.data[-1] == '"');
    assert(text.data[text.size] == '"');
    text.data -= 1;
    text.size += 2;

    Word w = {
        .type = WordType_Text,
        .text = text,
        .group = r->group,
        .nest = r->nest,
    };

    pushWord(r, w);
}

static void
pushTokenAsStringWord(Render *r, TokenId token) {
    pushTokenAsStringWordOnly(r, token);
    pushCommentsAfterToken(r, token);
}

static WordType
getConnectingWordType(Render *r, TokenId t1, TokenId t2) {
    return WordType_Space;
}

static void pushExpressionDocument(Render *r, ASTNode *node);

static void
pushCallArgumentListDocument(Render *r, TokenId startingToken, ASTNodeListRanged *expressions, TokenIdList *names) {
    assert(expressions->count != -1);

    pushTokenWord(r, startingToken);
    pushGroup(r);
    pushNest(r);
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
            pushGroup(r);
            TokenId literal = listGetTokenId(names, i);
            pushTokenWord(r, literal);
            pushTokenWord(r, literal + 1);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, &argument->node);
            popGroup(r);

            if(i < expressions->count - 1) {
                pushTokenWord(r, argument->node.endToken + 1);
            }
            pushWord(r, wordLine());
        }
        pushTokenWord(r, expressions->last->node.endToken + 1);
    }
    popNestWithLastWord(r);
    popGroup(r);

    if(expressions->count > 0) {
        TokenId token = expressions->last->node.endToken + 1;
        token += names->count > 0;
        pushTokenWord(r, token);
    } else {
        pushTokenWord(r, startingToken + 1);
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
                pushExpressionDocument(r, node->arrayTypeNode.lengthExpression);
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

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushNest(r);
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
            popNest(r);
            pushTokenWord(r, node->endToken);
            popGroup(r);
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

            pushGroup(r);
            pushExpressionDocument(r, function->expression);
            pushCallArgumentListDocument(r, function->expression->endToken + 1, &function->argumentsExpression, &function->argumentsName);
            popGroup(r);
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
            ASTNodeArraySliceExpression *array = &node->arraySliceExpressionNode;
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[array->expression->endToken + 1]));
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));

            pushExpressionDocument(r, array->expression);
            pushTokenWord(r, array->expression->endToken + 1);
            if(array->leftFenceExpression != 0x0) {
                pushExpressionDocument(r, array->leftFenceExpression);
                pushTokenWord(r, array->leftFenceExpression->endToken + 1);
            } else {
                pushTokenWord(r, array->expression->endToken + 2);
            }
            if(array->rightFenceExpression != 0x0) {
                pushExpressionDocument(r, array->rightFenceExpression);
            }
            pushTokenWord(r, node->endToken);
        } break;
        case ASTNodeType_InlineArrayExpression: {
            ASTNodeInlineArrayExpression *array = &node->inlineArrayExpressionNode;
            assert(stringMatch(LIT_TO_STR("["), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSoftline());
            pushNest(r);

            ASTNodeLink *expression = array->expressions.head;
            for(u32 i = 0; i < array->expressions.count; i++, expression = expression->next) {
                pushExpressionDocument(r, &expression->node);
                if(i < array->expressions.count - 1) {
                    assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[expression->node.endToken + 1]));
                    pushTokenWord(r, expression->node.endToken + 1);
                }
                pushWord(r, wordLine());
            }

            popNest(r);
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

static Word
expressionLinkWord(ASTNode *node) {
    switch(node->type){
        case ASTNodeType_InlineArrayExpression: return wordSpace();
        default: return wordLine();
    }
}

static s32
expressionNest(ASTNode *node) {
    switch(node->type){
        case ASTNodeType_InlineArrayExpression: return 0;
        default: return 1;
    }
}


static void
renderYulFunctionCall(Render *r, ASTNode *node) {
    // ASTNodeYulFunctionCallExpression *function = &node->yulFunctionCallExpressionNode;

    // renderToken(r, function->identifier, NONE);
    // assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->identifier + 1]));
    // renderToken(r, function->identifier + 1, NONE);
    // ASTNodeLink *argument = function->arguments.head;
    // for(u32 i = 0; i < function->arguments.count; i++, argument = argument->next) {
    //     ConnectType connect = i == function->arguments.count - 1 ? NONE : COMMA_SPACE;
    //     renderYulExpression(r, &argument->node, connect);
    // }
    // assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->endToken]));
    // renderToken(r, node->endToken, connect);
    assert(false);
}

static void
renderYulExpression(Render *r, ASTNode *node) {
    // switch(node->type) {
    //     case ASTNodeType_YulNumberLitExpression: {
    //         renderToken(r, node->yulNumberLitExpressionNode.value, connect);
    //     } break;
    //     case ASTNodeType_YulStringLitExpression: {
    //         // TODO(radomski): Escaping
    //         writeString(r->writer, LIT_TO_STR("\""));
    //         renderToken(r, node->yulHexStringLitExpressionNode.value, NONE);
    //         writeString(r->writer, LIT_TO_STR("\""));
    //         renderConnect(r, connect);
    //     } break;
    //     case ASTNodeType_YulHexNumberLitExpression: {
    //         renderToken(r, node->yulHexNumberLitExpressionNode.value, connect);
    //     } break;
    //     case ASTNodeType_YulBoolLitExpression: {
    //         renderToken(r, node->yulBoolLitExpressionNode.value, connect);
    //     } break;
    //     case ASTNodeType_YulHexStringLitExpression: {
    //         // TODO(radomski): Escaping
    //         writeString(r->writer, LIT_TO_STR("hex\""));
    //         renderToken(r, node->yulHexStringLitExpressionNode.value, NONE);
    //         writeString(r->writer, LIT_TO_STR("\""));
    //         renderConnect(r, connect);
    //     } break;
    //     case ASTNodeType_YulMemberAccessExpression: {
    //         for(u32 i = 0; i < node->yulIdentifierPathExpressionNode.count; i++) {
    //             ConnectType connection = i == node->yulIdentifierPathExpressionNode.count - 1 ? connect : DOT;
    //             renderToken(r, node->yulIdentifierPathExpressionNode.identifiers[i], connection);
    //         }
    //     } break;
    //     case ASTNodeType_YulFunctionCallExpression: {
    //         renderYulFunctionCall(r, node, connect);
    //     } break;
    //     default: {
    //         javascriptPrintNumber(node->type);
    //         assert(0);
    //     }
    // }
    assert(false);
}

static void
pushStatementDocument(Render *r, ASTNode *node) {
    switch(node->type) {
        case ASTNodeType_BlockStatement: {
            ASTNodeBlockStatement *block = &node->blockStatementNode;

            pushGroup(r);
            pushTokenWordOnly(r, node->startToken);
            pushNest(r);

            pushCommentsAfterToken(r, node->startToken);
            pushWord(r, wordLine());

            ASTNodeLink *statement = block->statements.head;
            for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
                if(i != 0) { preserveHardlinesIntoDocument(r, &statement->node); }

                pushStatementDocument(r, &statement->node);
            }

            popNest(r);
            pushTokenWordOnly(r, node->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, node->endToken);
        } break;
        case ASTNodeType_UncheckedBlockStatement: {
            //ASTNodeUncheckedBlockStatement *unchecked = &node->uncheckedBlockStatementNode;
            //assert(stringMatch(LIT_TO_STR("unchecked"), r->tokens.tokenStrings[node->startToken]));
            //renderToken(r, node->startToken, SPACE);
            //renderStatement(r, unchecked->block, NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_ReturnStatement: {
            assert(stringMatch(LIT_TO_STR("return"), r->tokens.tokenStrings[node->startToken]));

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            if(node->returnStatementNode.expression != 0x0) {
                pushWord(r, wordSpace());
                pushExpressionDocument(r, node->returnStatementNode.expression);
            }
            pushTokenWord(r, node->endToken);
            popGroup(r);

            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_ExpressionStatement: {
            if(node->expressionStatementNode.expression != 0x0) {
                pushGroup(r);
                pushExpressionDocument(r, node->expressionStatementNode.expression);
                popGroup(r);
                pushTokenWordOnly(r, node->endToken);

                pushCommentsAfterToken(r, node->endToken);
                pushWord(r, wordHardline());
            }
        } break;
        case ASTNodeType_IfStatement: {
            assert(stringMatch(LIT_TO_STR("if"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[node->ifStatementNode.conditionExpression->endToken + 1]));
            
            pushGroup(r);
            pushTokenWord(r, node->startToken);

            pushGroup(r);
            pushTokenWord(r, node->startToken + 1);
            pushExpressionDocument(r, node->ifStatementNode.conditionExpression);
            pushTokenWord(r, node->ifStatementNode.conditionExpression->endToken + 1);
            popGroup(r);

            pushWord(r, wordSpace());
            popGroup(r);

            pushStatementDocument(r, node->ifStatementNode.trueStatement);
            if(node->ifStatementNode.falseStatement) {
                // TODO(radomski): I don't like this, because it's another
                // solution that requies looking forward instead of
                // consolidating whitespace. Maybe there is a better solution.
                if(node->ifStatementNode.trueStatement->type == ASTNodeType_BlockStatement) {
                    pushWord(r, wordSpace());
                }

                pushTokenWord(r, node->ifStatementNode.falseStatement->startToken - 1);
                pushWord(r, wordSpace());
                pushStatementDocument(r, node->ifStatementNode.falseStatement);
            }
            pushWord(r, wordHardline());
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
                pushWord(r, wordSpace()); // TODO(radomski): Line?
                pushExpressionDocument(r, statement->initialValue);
            }

            pushTokenWord(r, node->endToken);
            popGroup(r);
            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_VariableDeclarationTupleStatement: {
            //ASTNodeVariableDeclarationTupleStatement *statement = &node->variableDeclarationTupleStatementNode;
            //
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("("), NONE);
            //ASTNodeLink *decl = statement->declarations.head;
            //for(u32 i = 0; i < statement->declarations.count; i++, decl = decl->next) {
            //if(decl->node.type != ASTNodeType_None) {
            //ConnectType innerConnect = i == statement->declarations.count - 1 ? connect : COMMA_SPACE;
            //renderVariableDeclaration(r, &decl->node, innerConnect);
            //}
            //}
            //renderTokenChecked(r, statement->initialValue->startToken - 2, LIT_TO_STR(")"), SPACE);
            //renderTokenChecked(r, statement->initialValue->startToken - 1, LIT_TO_STR("="), SPACE);
            //
            //renderExpression(r, statement->initialValue, SEMICOLON);
            assert(false);
        } break;
        case ASTNodeType_DoWhileStatement: {
            //ASTNodeWhileStatement *statement = &node->whileStatementNode;
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("do"), SPACE);
            //renderStatement(r, statement->body, SPACE);
            //renderTokenChecked(r, statement->expression->startToken - 2, LIT_TO_STR("while"), NONE);
            //renderTokenChecked(r, statement->expression->startToken - 1, LIT_TO_STR("("), NONE);
            //renderExpression(r, statement->expression, NONE);
            //renderTokenChecked(r, node->endToken - 1, LIT_TO_STR(")"), SEMICOLON);
            assert(false);
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
            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_ForStatement: {
            ASTNodeForStatement *statement = &node->forStatementNode;
            assert(stringMatch(LIT_TO_STR("for"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));

            pushTokenWord(r, node->startToken);
            pushGroup(r);
            pushNest(r);
            pushTokenWord(r, node->startToken + 1);
            pushWord(r, wordSoftline());
            
            if(statement->variableStatement != 0x0) {
                pushGroup(r);
                pushStatementDocument(r, statement->variableStatement);
                popGroup(r);
            }

            if(statement->conditionExpression != 0x0) {
                // TODO(radomski): This is a hack to make the condition expression
                pushGroup(r);
                pushGroup(r);
                pushExpressionDocument(r, statement->conditionExpression);
                popGroup(r);
                popGroup(r);
            }

            pushWord(r, wordText(LIT_TO_STR(";")));
            pushWord(r, wordLine());

            if(statement->incrementExpression != 0x0) {
                pushGroup(r);
                pushExpressionDocument(r, statement->incrementExpression);
                popGroup(r);
            }

            pushWord(r, wordSoftline());
            
            popNest(r);
            pushTokenWord(r, statement->body->startToken - 1);
            popGroup(r);

            pushWord(r, wordSpace());
            pushStatementDocument(r, statement->body);

            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_RevertStatement: {
            assert(stringMatch(LIT_TO_STR("revert"), r->tokens.tokenStrings[node->startToken]));
            ASTNodeRevertStatement *statement = &node->revertStatementNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, statement->expression);
            pushTokenWord(r, node->endToken);
            popGroup(r);

            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_EmitStatement: {
            assert(stringMatch(LIT_TO_STR("emit"), r->tokens.tokenStrings[node->startToken]));
            ASTNodeEmitStatement *statement = &node->emitStatementNode;

            pushGroup(r);
            pushTokenWord(r, node->startToken);
            pushWord(r, wordSpace());
            pushExpressionDocument(r, statement->expression);
            pushTokenWordOnly(r, node->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, node->endToken);
            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_TryStatement: {
            //ASTNodeTryStatement *statement = &node->tryStatementNode;
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("try"), SPACE);
            //
            //renderExpression(r, statement->expression, SPACE);
            //renderTokenChecked(r, statement->expression->endToken + 1, LIT_TO_STR("returns"), SPACE);
            //renderTokenChecked(r, statement->expression->endToken + 2, LIT_TO_STR("("), NONE);
            //renderParameters(r, &statement->returnParameters, COMMA_SPACE, 0);
            //renderTokenChecked(r, statement->body->startToken - 1, LIT_TO_STR(")"), SPACE);
            //renderStatement(r, statement->body, statement->catches.count > 0 ? SPACE : NEWLINE);
            //
            //ASTNodeLink *catchLink = statement->catches.head;
            //for(u32 i = 0; i < statement->catches.count; i++, catchLink = catchLink->next) {
            //renderTokenChecked(r, catchLink->node.startToken, LIT_TO_STR("catch"), SPACE);
            //ASTNodeCatchStatement *catch = &catchLink->node.catchStatementNode;
            //
            //u32 openParenToken = catchLink->node.startToken + 1;
            //if(catch->identifier != INVALID_TOKEN_ID) {
            //renderToken(r, catch->identifier, NONE);
            //openParenToken = catch->identifier + 1;
            //}
            //
            //if(catch->parameters.count != -1) {
            //renderTokenChecked(r, openParenToken, LIT_TO_STR("("), NONE);
            //renderParameters(r, &catch->parameters, COMMA_SPACE, 0);
            //renderTokenChecked(r, catch->body->startToken - 1, LIT_TO_STR(")"), SPACE);
            //}
            //renderStatement(r, catch->body, i == statement->catches.count - 1 ? NEWLINE : SPACE);
            //}
            assert(false);
        } break;
        case ASTNodeType_BreakStatement: {
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("break"), SEMICOLON);
            assert(false);
        } break;
        case ASTNodeType_ContinueStatement: {
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("continue"), SEMICOLON);
            assert(false);
        } break;
        case ASTNodeType_AssemblyStatement: {
            //ASTNodeAssemblyStatement *statement = &node->assemblyStatementNode;
            //
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("assembly"), SPACE);
            //if(statement->isEVMAsm) {
            //renderCString(r, "\"evmasm\"", SPACE);
            //}
            //
            //if(statement->flags.count > 0) {
            //renderCString(r, "(", NONE);
            //for(u32 i = 0; i < statement->flags.count; i++) {
            //writeString(r->writer, LIT_TO_STR("\""));
            //renderToken(r, listGetTokenId(&statement->flags, i), NONE);
            //writeString(r->writer, LIT_TO_STR("\""));
            //if(i != statement->flags.count - 1) {
            //renderCString(r, ",", SPACE);
            //}
            //}
            //renderCString(r, ")", SPACE);
            //}
            //
            //renderStatement(r, statement->body, NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulBlockStatement: {
            //ASTNodeBlockStatement *block = &node->blockStatementNode;
            //renderToken(r, node->startToken, block->statements.count > 0 ? NEWLINE : SPACE);
            //
            //pushIndent(r->writer);
            //ASTNodeLink *statement = block->statements.head;
            //for(u32 i = 0; i < block->statements.count; i++, statement = statement->next) {
            //if(i != 0) {
            //preservePresentNewLines(r, &statement->node);
            //}
            //renderStatement(r, &statement->node, NONE);
            //}
            //popIndent(r->writer);
            //
            //renderToken(r, node->endToken, connect);
            //return;
            assert(false);
        } break;
        case ASTNodeType_YulVariableDeclaration: {
            //ASTNodeYulVariableDeclaration *statement = &node->yulVariableDeclarationNode;
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("let"), SPACE);
            //
            //ConnectType valueConnect = statement->value != 0x0 ? SPACE : NEWLINE;
            //for(u32 i = 0; i < statement->identifiers.count; i++) {
            //ConnectType connect = i == statement->identifiers.count - 1 ? valueConnect : COMMA_SPACE;
            //renderToken(r, listGetTokenId(&statement->identifiers, i), connect);
            //}
            //
            //if(statement->value != 0x0) {
            //renderTokenChecked(r, statement->value->startToken - 2, LIT_TO_STR(":"), NONE);
            //renderTokenChecked(r, statement->value->startToken - 1, LIT_TO_STR("="), SPACE);
            //renderYulExpression(r, statement->value, NEWLINE);
            //}
            assert(false);
        } break;
        case ASTNodeType_YulVariableAssignment: {
            //ASTNodeYulVariableAssignment *statement = &node->yulVariableAssignmentNode;
            //
            //ASTNodeLink *it = statement->paths.head;
            //ConnectType valueConnect = statement->value != 0x0 ? SPACE : NEWLINE;
            //for(u32 i = 0; i < statement->paths.count; i++, it = it->next) {
            //ConnectType connect = i == statement->paths.count - 1 ? valueConnect : COMMA_SPACE;
            //renderYulExpression(r, &it->node, connect);
            //}
            //
            //if(statement->value != 0x0) {
            //renderTokenChecked(r, statement->value->startToken - 2, LIT_TO_STR(":"), NONE);
            //renderTokenChecked(r, statement->value->startToken - 1, LIT_TO_STR("="), SPACE);
            //renderYulExpression(r, statement->value, NEWLINE);
            //}
            assert(false);
        } break;
        case ASTNodeType_YulFunctionCallExpression: {
            //renderYulFunctionCall(r, node, NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulIfStatement: {
            //ASTNodeYulIfStatement *statement = &node->yulIfStatementNode;
            //
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("if"), SPACE);
            //renderYulExpression(r, statement->expression, SPACE);
            //renderStatement(r, statement->body, NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulForStatement: {
            //ASTNodeYulForStatement *statement = &node->yulForStatementNode;
            //
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("for"), SPACE);
            //renderStatement(r, statement->variableDeclaration, SPACE);
            //renderYulExpression(r, statement->condition, SPACE);
            //renderStatement(r, statement->increment, SPACE);
            //renderStatement(r, statement->body, NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulFunctionDefinition: {
            //ASTNodeYulFunctionDefinition *statement = &node->yulFunctionDefinitionNode;
            //
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("function"), SPACE);
            //renderToken(r, statement->identifier, statement->parameters.count > 0 ? NONE : SPACE);
            //if(statement->parameters.count > 0) {
            //renderCString(r, "(", NONE);
            //for(u32 i = 0; i < statement->parameters.count; i++) {
            //ConnectType connect = i == statement->parameters.count - 1 ? NONE : COMMA_SPACE;
            //renderToken(r, listGetTokenId(&statement->parameters, i), connect);
            //}
            //renderCString(r, ")", SPACE);
            //}
            //
            //if(statement->returnParameters.count > 0) {
            //renderCString(r, "->", SPACE);
            //for(u32 i = 0; i < statement->returnParameters.count; i++) {
            //ConnectType connect = i == statement->returnParameters.count - 1 ? SPACE : COMMA_SPACE;
            //renderToken(r, listGetTokenId(&statement->returnParameters, i), connect);
            //}
            //}
            //
            //renderStatement(r, statement->body, NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulSwitchStatement: {
            //ASTNodeYulSwitchStatement *statement = &node->yulSwitchStatementNode;
            //
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("switch"), SPACE);
            //ConnectType expressionConnect = statement->cases.count > 0 || statement->defaultBlock != 0x0 ? NEWLINE : SPACE;
            //renderYulExpression(r, statement->expression, expressionConnect);
            //
            //ASTNodeLink *it = statement->cases.head;
            //for(u32 i = 0; i < statement->cases.count; i++, it = it->next) {
            //renderTokenChecked(r, it->node.yulCaseNode.literal->startToken - 1, LIT_TO_STR("case"), SPACE);
            //renderYulExpression(r, it->node.yulCaseNode.literal, SPACE);
            //renderStatement(r, it->node.yulCaseNode.block, NEWLINE);
            //}
            //
            //if(statement->defaultBlock != 0x0) {
            //renderTokenChecked(r, statement->defaultBlock->yulCaseNode.block->startToken - 1, LIT_TO_STR("default"), SPACE);
            //renderStatement(r, statement->defaultBlock->yulCaseNode.block, NEWLINE);
            //}
            assert(false);
        } break;
        case ASTNodeType_YulLeaveStatement: {
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("leave"), NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulBreakStatement: {
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("break"), NEWLINE);
            assert(false);
        } break;
        case ASTNodeType_YulContinueStatement: {
            //renderTokenChecked(r, node->startToken, LIT_TO_STR("continue"), NEWLINE);
            assert(false);
        } break;
        default: {
            javascriptPrintString("Unreachable, unhandled statement type = ");
            javascriptPrintNumber(node->type);
            assert(0);
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
        assert(stringMatch(LIT_TO_STR("override"), r->tokens.tokenStrings[overrideToken]));
        pushTokenWord(r, overrideToken);
        if(overrides->count > 0) {
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[overrides->startToken - 1]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[overrides->endToken + 1]));

            pushWord(r, wordSpace());
            pushTokenWord(r, overrides->startToken - 1);

            ASTNodeLink *override = overrides->head;
            for(u32 i = 0; i < overrides->count; i++, override = override->next) {
                pushTypeDocument(r, &override->node);
                if(i != overrides->count - 1) {
                    pushTokenWord(r, override->node.endToken + 1);
                    pushWord(r, wordSpace());
                }
            }

            pushTokenWord(r, overrides->endToken + 1);
            pushWord(r, wordSpace());
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

            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[startingToken]));
            pushTokenWord(r, startingToken);

            if(names->count == 0) {
                ASTNodeLink *argument = expressions->head;
                for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
                    pushExpressionDocument(r, &argument->node);
                    if(i != expressions->count - 1) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[argument->node.endToken + 1]));
                        pushTokenWord(r, argument->node.endToken + 1);
                        pushWord(r, wordLine());
                    }
                }
            } else {
                ASTNodeLink *argument = expressions->head;
                assert(names->count == expressions->count);
                assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[listGetTokenId(names, 0) - 1]));
                assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[expressions->last->node.endToken + 1]));

                pushTokenWord(r, listGetTokenId(names, 0) - 1);
                pushWord(r, wordSpace());
                for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
                    TokenId literal = listGetTokenId(names, i);
                    assert(stringMatch(LIT_TO_STR(":"), r->tokens.tokenStrings[literal - 1]));

                    pushTokenWord(r, literal);
                    pushTokenWord(r, literal + 1);
                    pushWord(r, wordSpace());
                    pushExpressionDocument(r, &argument->node);
                    if(i != expressions->count - 1) {
                        assert(stringMatch(LIT_TO_STR(","), r->tokens.tokenStrings[argument->node.endToken + 1]));
                        pushTokenWord(r, argument->node.endToken + 1);
                        pushWord(r, wordLine());
                    }
                }
                pushTokenWord(r, expressions->last->node.endToken + 1);
            }

            if(expressions->count > 0) {
                TokenId token = expressions->last->node.endToken + 1;
                token += names->count > 0;
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[token]));
                pushTokenWord(r, token);
            } else {
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[startingToken + 1]));
                pushTokenWord(r, startingToken + 1);
            }
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
            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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

                pushWord(r, wordHardline());
            }
            popNestWithLastWord(r);

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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
            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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
            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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
            pushWord(r, expressionLinkWord(constNode->expression));

            pushGroup(r);
            addNest(r, expressionNest(constNode->expression));
            pushExpressionDocument(r, constNode->expression);
            popGroup(r);
            addNest(r, -expressionNest(constNode->expression));

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
        } break;
        case ASTNodeType_StateVariableDeclaration: {
            ASTNodeConstVariable *decl = &member->constVariableNode;

            pushGroup(r);
            pushTypeDocument(r, decl->type);
            pushWord(r, wordSpace());

            if(decl->visibility != INVALID_TOKEN_ID) {
                pushTokenWord(r, decl->visibility);
                pushWord(r, wordSpace());
            }
            if(decl->mutability != INVALID_TOKEN_ID) {
                pushTokenWord(r, decl->mutability);
                pushWord(r, wordSpace());
            }

            pushTokenWord(r, decl->identifier);

            if(decl->expression) {
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[decl->identifier + 1]));
                pushWord(r, wordSpace());
                pushTokenWord(r, decl->identifier + 1);
                pushWord(r, expressionLinkWord(decl->expression));

                pushGroup(r);
                addNest(r, expressionNest(decl->expression));
                pushExpressionDocument(r, decl->expression);
                popGroup(r);
                addNest(r, -expressionNest(decl->expression));
            }

            pushTokenWordOnly(r, member->endToken);
            popGroup(r);

            pushCommentsAfterToken(r, member->endToken);
            pushWord(r, wordHardline());
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
                pushWord(r, wordSpace());
            } else if(member->type == ASTNodeType_ReceiveFunction) {
                assert(stringMatch(LIT_TO_STR("receive"), r->tokens.tokenStrings[member->startToken]));
                pushTokenWord(r, member->startToken);
                pushWord(r, wordSpace());
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

            pushTokenWord(r, openParenToken);
            pushParametersDocument(r, &function->parameters);
            pushTokenWord(r, closeParenToken);

            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[openParenToken]));
            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[closeParenToken]));

            if(function->visibility != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, function->visibility);
            }
            if(function->stateMutability != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, function->stateMutability);
            }

            if(function->virtual != INVALID_TOKEN_ID) {
                pushWord(r, wordSpace());
                pushTokenWord(r, function->virtual);
            }

            pushOverridesDocument(r, function->override, function->overrides);

            if(function->returnParameters) {
                assert(stringMatch(LIT_TO_STR("returns"), r->tokens.tokenStrings[function->returnParameters->startToken - 2]));
                assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[function->returnParameters->startToken - 1]));
                assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[function->returnParameters->endToken + 1]));

                pushWord(r, wordSpace());
                pushTokenWord(r, function->returnParameters->startToken - 2);
                pushWord(r, wordSpace());
                pushTokenWord(r, function->returnParameters->startToken - 1);
                pushParametersDocument(r, function->returnParameters);
                pushTokenWord(r, function->returnParameters->endToken + 1);
            }

            if(function->modifiers && function->modifiers->count > 0) {
                pushWord(r, wordSpace());
                pushModifierInvocations(r, function->modifiers);
            }

            if(function->body != 0x0) {
                pushWord(r, wordLine());
                popGroup(r);
                pushStatementDocument(r, function->body);
                pushWord(r, wordHardline());
            } else {
                pushTokenWordOnly(r, member->endToken);
                popGroup(r);

                pushCommentsAfterToken(r, member->endToken);
                pushWord(r, wordHardline());
            }
        } break;
        case ASTNodeType_ConstructorDefinition: {
            ASTNodeConstructorDefinition *constructor = &member->constructorDefinitionNode;
            assert(stringMatch(LIT_TO_STR("constructor"), r->tokens.tokenStrings[member->startToken]));

            pushGroup(r);

            pushGroup(r);
            pushTokenWord(r, member->startToken);

            pushNest(r);
            TokenId openParenToken = member->startToken + 1;
            TokenId closeParenToken = constructor->parameters.count > 0
                ? constructor->parameters.last->node.endToken + 1
                : openParenToken + 1;

            pushTokenWord(r, openParenToken);
            pushParametersDocument(r, &constructor->parameters);
            pushTokenWord(r, closeParenToken);
            popGroup(r);

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

                pushStatementDocument(r, constructor->body);
                pushWord(r, wordHardline());
            } else {
                pushTokenWordOnly(r, member->endToken);
                popGroup(r);

                pushCommentsAfterToken(r, member->endToken);
                pushWord(r, wordHardline());
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

            popGroup(r);
            if(modifier->body != 0x0) {
                pushWord(r, wordSpace());
                pushStatementDocument(r, modifier->body);
                pushWord(r, wordHardline());
            } else {
                pushTokenWordOnly(r, member->endToken);

                pushCommentsAfterToken(r, member->endToken);
                pushWord(r, wordHardline());
            }
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
                    pushTokenWord(r, member->startToken);
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
            pushTokenWordOnly(r, openParenToken);
            pushNest(r);
            pushCommentsAfterToken(r, openParenToken);
            pushWord(r, wordLine());

            ASTNodeLink *element = contract->elements.head;
            for(u32 i = 0; i < contract->elements.count; i++, element = element->next) {
                if(i > 0) {
                    preserveHardlinesIntoDocument(r, &element->node);
                }
                pushMemberDocument(r, &element->node);
            }
            popNest(r);

            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));
            pushTokenWord(r, member->endToken);
            popGroup(r);
            popGroup(r);
            pushWord(r, wordHardline());
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
        preserveHardlinesIntoDocument(r, &child->node);
        pushMemberDocument(r, &child->node);
    }
    renderDocument(r);
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
