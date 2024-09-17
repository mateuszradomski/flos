typedef struct Writer {
	u8 *data;
	u32 size;
	u32 capacity;

	u32 indentSize;
	u32 indentCount;
	u32 lineSize;
} Writer;

typedef struct Render {
	Writer *writer;
	TokenizeResult tokens;
    u8 *sourceBaseAddress;
} Render;

static void
write(Writer *w, String str) {
	if(w->lineSize == 0) {
		for(u32 i = 0; i < w->indentSize * w->indentCount; i++) {
			w->data[w->size++] = ' ';
			w->lineSize += 1;
		}
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

	write(w, LIT_TO_STR("\n"));
	w->lineSize = 0;
}

static void
pushIndent(Writer *w) {
	w->indentCount += 1;
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

    for(u32 i = 0; i < inBetween.size - 1; i++) {
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

typedef enum ConnectType {
	NONE,
	SPACE,
	COMMA,
	COMMA_SPACE,
    COLON_SPACE,
    SEMICOLON,
	DOT,
	NEWLINE,
} ConnectType;

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

            if(input.data[i] == '/' && input.data[i + 1] == '/') {
                u32 newlineIndex = i;
                while(newlineIndex < input.size && input.data[newlineIndex] != '\n') {
                    newlineIndex += 1;
                }

                if(input.data[newlineIndex - 1] == '\r') {
                    newlineIndex -= 1;
                }

                commentCount += 1;
                comment = (String) {
                    .data = input.data + commentStart,
                    .size = newlineIndex - commentStart
                };
                commentEnd = newlineIndex;
            } else if(input.data[i] == '/' && input.data[i + 1] == '*') {
                while(commentEnd < input.size - 1 && (input.data[commentEnd] != '*' || input.data[commentEnd + 1] != '/')) {
                    commentEnd += 1;
                }
                commentEnd += 2;

                commentCount += 1;
                comment = (String) {
                    .data = input.data + commentStart,
                    .size = commentEnd - commentStart
                };
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
                    write(r->writer, LIT_TO_STR(" "));
                }

                index = commentEnd + 1;
                i = index;

                write(r->writer, comment);
                finished = false;
            }
        }

        if(finished) {
            break;
        }
    }

    u32 preceedingNewlines = 0;
    for(u32 i = index - 1; i < input.size; i++) {
        preceedingNewlines += input.data[i] == '\n';
    }

    if(preceedingNewlines >= 2) {
        finishLine(r->writer);
    } 

    return commentCount;
}

static void
renderConnect(Render *r, ConnectType connect) {
    switch(connect) {
        case NONE: break;
        case SPACE: write(r->writer, LIT_TO_STR(" ")); break;
        case COMMA: write(r->writer, LIT_TO_STR(",")); finishLine(r->writer); break;
        case COMMA_SPACE: write(r->writer, LIT_TO_STR(", ")); break;
        case COLON_SPACE: write(r->writer, LIT_TO_STR(": ")); break;
        case SEMICOLON: write(r->writer, LIT_TO_STR(";")); finishLine(r->writer); break;
        case DOT: write(r->writer, LIT_TO_STR(".")); break;
        case NEWLINE: finishLine(r->writer); break;
    }
}

static void renderToken(Render *r, TokenId token, ConnectType connect);

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
        case SPACE: write(r->writer, LIT_TO_STR(" ")); break;
        case COMMA: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Comma) {
                renderToken(r, nextToken, NEWLINE);
            } else if(writtenComments == 0) {
                finishLine(r->writer);
            }
        } break;
        case COMMA_SPACE: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Comma) {
                renderToken(r, nextToken, SPACE);
            } else if(writtenComments == 0) {
                write(r->writer, LIT_TO_STR(" "));
            }
        } break;
        case COLON_SPACE: write(r->writer, LIT_TO_STR(": ")); break;
        case SEMICOLON: {
            if(r->tokens.tokenTypes[nextToken] == TokenType_Semicolon) {
                renderToken(r, nextToken, NEWLINE);
            } else if(writtenComments == 0) {
                finishLine(r->writer);
            }
        } break;
        case DOT: write(r->writer, LIT_TO_STR(".")); break;
        case NEWLINE: finishLine(r->writer); break;
    }
}

static void
renderString(Render *r, String string, ConnectType connect) {
    write(r->writer, string);
    renderConnect(r, connect);
}

#define renderCString(r, s, c) renderString(r, LIT_TO_STR((s)), c)

static void
renderVisibility(Render *r, u32 visibility) {
    switch(visibility) {
        case 1: {
            renderCString(r, "internal", SPACE);
        } break;
        case 2: {
            renderCString(r, "external", SPACE);
        } break;
        case 3: {
            renderCString(r, "private", SPACE);
        } break;
        case 4: {
            renderCString(r, "public", SPACE);
        } break;
        default: { }
    }
}

static void
renderStateMutability(Render *r, u32 stateMutability) {
    switch(stateMutability) {
        case 1: {
            renderCString(r, "pure", SPACE);
        } break;
        case 2: {
            renderCString(r, "view", SPACE);
        } break;
        case 3: {
            renderCString(r, "payable", SPACE);
        } break;
        default: { }
    }
}

static void
renderToken(Render *r, TokenId token, ConnectType connect) {
	assert(token != INVALID_TOKEN_ID);
    write(r->writer, getTokenString(r->tokens, token));
    renderConnectWithComments(r, token, connect);
}

static void
renderTokenAsString(Render *r, TokenId token, ConnectType connect) {
	write(r->writer, LIT_TO_STR("\""));
	renderToken(r, token, NONE);
	write(r->writer, LIT_TO_STR("\""));
	renderConnect(r, connect);
}

static void
renderCallArgumentList(Render *r, ASTNodeList *expressions, TokenIdList *names) {
    if(expressions->count == -1) {
    	assert(false);
        return;
    }

    renderCString(r, "(", NONE);
    if(names->count == 0) {
	    ASTNodeLink *argument = expressions->head;
	    for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
	    	void renderExpression(Render *r, ASTNode *node, ConnectType connect);
	    	ConnectType connect = i == expressions->count - 1 ? NONE : COMMA_SPACE;
	        renderExpression(r, &argument->node, connect);
	    }
    } else {
	    renderCString(r, "{", SPACE);
    	ASTNodeLink *argument = expressions->head;
    	assert(names->count == expressions->count);
    	for(u32 i = 0; i < expressions->count; i++, argument = argument->next) {
    		void renderExpression(Render *r, ASTNode *node, ConnectType connect);

    		TokenId literal = listGetTokenId(names, i);
    		ConnectType connect = i == expressions->count - 1 ? NONE : COMMA_SPACE;
    		renderToken(r, literal, NONE);
    		renderCString(r, ":", SPACE);
    		renderExpression(r, &argument->node, connect);
    	}
        renderCString(r, "}", NONE);
    }
    renderCString(r, ")", NONE);
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
            write(r->writer, LIT_TO_STR("\""));
            for(u32 i = 0; i < expression->values.count; i++) {
                TokenId literal = listGetTokenId(&expression->values, i);
                renderToken(r, literal, NONE);
            }
            write(r->writer, LIT_TO_STR("\""));
            renderConnect(r, connect);
        } break;
        case ASTNodeType_BoolLitExpression: {
        	renderToken(r, node->boolLitExpressionNode.value, connect);
        } break;
        case ASTNodeType_IdentifierPath:
        case ASTNodeType_ArrayType:
        case ASTNodeType_MappingType: {
        	void renderType(Render *r, ASTNode *node, ConnectType connect);
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

            renderCString(r, "(", NONE);
            ASTNodeLink *element = tuple->elements.head;
            for(u32 i = 0; i < tuple->elements.count; i++, element = element->next) {
            	ConnectType connect = i == tuple->elements.count - 1 ? NONE : COMMA_SPACE;
                if(element->node.type != ASTNodeType_None) {
                    renderExpression(r, &element->node, NONE);
                }
                renderConnect(r, connect);
            }
            renderCString(r, ")", connect);
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
        	void renderType(Render *r, ASTNode *node, ConnectType connect);
            assert(stringMatch(LIT_TO_STR("new"), r->tokens.tokenStrings[node->startToken]));
            renderToken(r, node->startToken, SPACE);
        	renderType(r, node->newExpressionNode.typeName, connect);
        } break;
        case ASTNodeType_FunctionCallExpression: {
            ASTNodeFunctionCallExpression *function = &node->functionCallExpressionNode;

            renderExpression(r, function->expression, NONE);
            renderCallArgumentList(r, &function->argumentsExpression, &function->argumentsName);
            renderConnect(r, connect);
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
            renderCString(r, "[", NONE);
            if(array->indexExpression != 0x0) {
                renderExpression(r, array->indexExpression, NONE);
            }
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));
            renderToken(r, node->endToken, connect);
        } break;
        case ASTNodeType_ArraySliceExpression: {
            ASTNodeArraySliceExpression *array = &node->arraySliceExpressionNode;
            renderExpression(r, array->expression, NONE);
            renderCString(r, "[", NONE);
            if(array->leftFenceExpression != 0x0) {
                renderExpression(r, array->leftFenceExpression, NONE);
            }
            renderCString(r, ":", NONE);
            if(array->rightFenceExpression != 0x0) {
                renderExpression(r, array->rightFenceExpression, NONE);
            }
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));
            renderToken(r, node->endToken, connect);
        } break;
        case ASTNodeType_InlineArrayExpression: {
            ASTNodeInlineArrayExpression *array = &node->inlineArrayExpressionNode;

            renderCString(r, "[", NONE);
            ASTNodeLink *expression = array->expressions.head;
            for(u32 i = 0; i < array->expressions.count; i++, expression = expression->next) {
                ConnectType connect = i == array->expressions.count - 1 ? NONE : COMMA_SPACE;
                renderExpression(r, &expression->node, connect);
            }
            assert(stringMatch(LIT_TO_STR("]"), r->tokens.tokenStrings[node->endToken]));
            renderToken(r, node->endToken, connect);
        } break;
        case ASTNodeType_TerneryExpression: {
            ASTNodeTerneryExpression *ternery = &node->terneryExpressionNode;
            renderExpression(r, ternery->condition, SPACE);
            renderCString(r, "?", SPACE);
            renderExpression(r, ternery->trueExpression, SPACE);
            renderCString(r, ":", SPACE);
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

	    	renderCString(r, "=>", SPACE);

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
            void renderParameters(Render *r, ASTNodeList *list, ConnectType connect, u32 connectIsInclusive);
            assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[node->startToken]));
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[node->startToken + 1]));

            renderToken(r, node->startToken, SPACE);
            renderToken(r, node->startToken + 1, NONE);
            renderParameters(r, &node->functionTypeNode.parameters, COMMA_SPACE, 0);
            renderCString(r, ")", SPACE);
            renderVisibility(r, node->functionTypeNode.visibility);
            renderStateMutability(r, node->functionTypeNode.stateMutability);

            if(node->functionTypeNode.returnParameters.count > 0) {
                renderCString(r, "returns (", NONE);
                renderParameters(r, &node->functionTypeNode.returnParameters, COMMA_SPACE, 0);
                renderCString(r, ")", SPACE);
            }

            assert(connect == SPACE);
            // renderConnect(r, connect);
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
    switch(decl->dataLocation) {
	    case 1: {
	    	renderCString(r, "memory", SPACE);
	    } break;
    	case 2: {
    		renderCString(r, "storage", SPACE);
    	} break;
    	case 3: {
    		renderCString(r, "calldata", SPACE);
    	}break;
    	case 4: {
    		renderCString(r, "indexed", SPACE);
    	}break;
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
        void renderYulExpression(Render *r, ASTNode *node, ConnectType connect);
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
            write(r->writer, LIT_TO_STR("\""));
            renderToken(r, node->yulHexStringLitExpressionNode.value, NONE);
            write(r->writer, LIT_TO_STR("\""));
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
            write(r->writer, LIT_TO_STR("hex\""));
            renderToken(r, node->yulHexStringLitExpressionNode.value, NONE);
            write(r->writer, LIT_TO_STR("\""));
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

            renderToken(r, node->endToken, NONE);
        } break;
        case ASTNodeType_UncheckedBlockStatement: {
            ASTNodeUncheckedBlockStatement *unchecked = &node->uncheckedBlockStatementNode;
            assert(stringMatch(LIT_TO_STR("unchecked"), r->tokens.tokenStrings[node->startToken]));
            renderToken(r, node->startToken, SPACE);
            renderStatement(r, unchecked->block, NEWLINE);
        } break;
        case ASTNodeType_ReturnStatement: {
            assert(stringMatch(LIT_TO_STR("return"), r->tokens.tokenStrings[node->startToken]));
            renderToken(r, node->startToken, node->returnStatementNode.expression != 0x0 ? SPACE : NONE);
            if(node->returnStatementNode.expression != 0x0) {
                renderExpression(r, node->returnStatementNode.expression, NONE);
            }
            renderConnect(r, SEMICOLON);
        } break;
        case ASTNodeType_ExpressionStatement: {
            if(node->expressionStatementNode.expression != 0x0) {
                renderExpression(r, node->expressionStatementNode.expression, SEMICOLON);
            }
        } break;
        case ASTNodeType_IfStatement: {
        	renderCString(r, "if(", NONE);
        	renderExpression(r, node->ifStatementNode.conditionExpression, NONE);
        	renderCString(r, ")", SPACE);
            ConnectType trueConnect = node->ifStatementNode.trueStatement->type == ASTNodeType_BlockStatement ? NEWLINE : NONE;
        	renderStatement(r, node->ifStatementNode.trueStatement, node->ifStatementNode.falseStatement ? SPACE : trueConnect);
            if(node->ifStatementNode.falseStatement) {
            	renderCString(r, "else", SPACE);
                ConnectType falseConnect = node->ifStatementNode.falseStatement->type == ASTNodeType_BlockStatement ? NEWLINE : NONE;
                renderStatement(r, node->ifStatementNode.falseStatement, falseConnect);
            }
        } break;
        case ASTNodeType_VariableDeclarationStatement: {
            ASTNodeVariableDeclarationStatement *statement = &node->variableDeclarationStatementNode;

            renderVariableDeclaration(r, statement->variableDeclaration, statement->initialValue != 0x0 ? SPACE : SEMICOLON);
            if(statement->initialValue != 0x0) {
            	renderCString(r, "=", SPACE);
                renderExpression(r, statement->initialValue, SEMICOLON);
            }
        } break;
        case ASTNodeType_VariableDeclarationTupleStatement: {
            ASTNodeVariableDeclarationTupleStatement *statement = &node->variableDeclarationTupleStatementNode;

            renderCString(r, "(", NONE);
            ASTNodeLink *decl = statement->declarations.head;
            for(u32 i = 0; i < statement->declarations.count; i++, decl = decl->next) {
                if(decl->node.type != ASTNodeType_None) {
                	ConnectType connect = i == statement->declarations.count - 1 ? NONE : COMMA_SPACE;
                    renderVariableDeclaration(r, &decl->node, connect);
                }
                renderConnect(r, connect);
            }
            renderCString(r, ") =", SPACE);

            renderExpression(r, statement->initialValue, SEMICOLON);
        } break;
        case ASTNodeType_DoWhileStatement: {
            ASTNodeWhileStatement *statement = &node->whileStatementNode;
            renderCString(r, "do", SPACE);
            renderStatement(r, statement->body, SPACE);
            renderCString(r, "while(", NONE);
            renderExpression(r, statement->expression, NONE);
            renderCString(r, ")", SEMICOLON);
        } break;
        case ASTNodeType_WhileStatement: {
            ASTNodeWhileStatement *statement = &node->whileStatementNode;
            renderCString(r, "while(", NONE);
            renderExpression(r, statement->expression, NONE);
            renderCString(r, ")", SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_ForStatement: {
            ASTNodeForStatement *statement = &node->forStatementNode;
            renderCString(r, "for(", NONE);

            if(statement->variableStatement != 0x0) {
                renderStatement(r, statement->variableStatement, NONE);
            }
            if(statement->conditionExpression != 0x0) {
                renderExpression(r, statement->conditionExpression, NONE);
            }
            renderCString(r, ";", SPACE);

            ASTNode virtualExpressionStatement = { 0 };
            virtualExpressionStatement.type = ASTNodeType_ExpressionStatement;
            virtualExpressionStatement.expressionStatementNode.expression = statement->incrementExpression;
            renderStatement(r, &virtualExpressionStatement, NONE);

            renderCString(r, ")", SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_RevertStatement: {
            ASTNodeRevertStatement *statement = &node->revertStatementNode;
            renderCString(r, "revert", SPACE);
            renderExpression(r, statement->expression, SEMICOLON);
        } break;
        case ASTNodeType_EmitStatement: {
            ASTNodeEmitStatement *statement = &node->emitStatementNode;
            renderCString(r, "emit", SPACE);
            renderExpression(r, statement->expression, SEMICOLON);
        } break;
        case ASTNodeType_TryStatement: {
            void renderParameters(Render *r, ASTNodeList *list, ConnectType connect, u32 connectIsInclusive);

            ASTNodeTryStatement *statement = &node->tryStatementNode;
            renderCString(r, "try", SPACE);

            renderExpression(r, statement->expression, SPACE);
            renderCString(r, "returns (", NONE);
            renderParameters(r, &statement->returnParameters, COMMA_SPACE, 0);
            renderCString(r, ")", SPACE);
            renderStatement(r, statement->body, statement->catches.count > 0 ? SPACE : NEWLINE);

            ASTNodeLink *catchLink = statement->catches.head;
            for(u32 i = 0; i < statement->catches.count; i++, catchLink = catchLink->next) {
                ASTNodeCatchStatement *catch = &catchLink->node.catchStatementNode;
                renderCString(r, "catch", SPACE);
                if(catch->parameters.count != -1) {
                    renderCString(r, "(", NONE);
                    renderParameters(r, &catch->parameters, COMMA_SPACE, 0);
                    renderCString(r, ")", SPACE);
                }
                renderStatement(r, catch->body, i == statement->catches.count - 1 ? NEWLINE : SPACE);
            }
        } break;
        case ASTNodeType_BreakStatement: {
            renderCString(r, "break", SEMICOLON);
        } break;
        case ASTNodeType_ContinueStatement: {
            renderCString(r, "continue", SEMICOLON);
        } break;
        case ASTNodeType_AssemblyStatement: {
            ASTNodeAssemblyStatement *statement = &node->assemblyStatementNode;

            renderCString(r, "assembly", SPACE);
            if(statement->isEVMAsm) {
                renderCString(r, "\"evmasm\"", SPACE);
            }

            if(statement->flags.count > 0) {
                renderCString(r, "(", NONE);
                for(u32 i = 0; i < statement->flags.count; i++) {
                    write(r->writer, LIT_TO_STR("\""));
                    renderToken(r, listGetTokenId(&statement->flags, i), NONE);
                    write(r->writer, LIT_TO_STR("\""));
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

            renderToken(r, node->endToken, NONE);
        } break;
        case ASTNodeType_YulVariableDeclaration: {
            ASTNodeYulVariableDeclaration *statement = &node->yulVariableDeclarationNode;

            renderCString(r, "let", SPACE);
            ConnectType valueConnect = statement->value != 0x0 ? SPACE : NEWLINE;
            for(u32 i = 0; i < statement->identifiers.count; i++) {
                ConnectType connect = i == statement->identifiers.count - 1 ? valueConnect : COMMA_SPACE;
                renderToken(r, listGetTokenId(&statement->identifiers, i), connect);
            }

            if(statement->value != 0x0) {
                renderCString(r, ":=", SPACE);
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
                renderCString(r, ":=", SPACE);
                renderYulExpression(r, statement->value, NEWLINE);
            }
        } break;
        case ASTNodeType_YulFunctionCallExpression: {
            renderYulFunctionCall(r, node, NEWLINE);
        } break;
        case ASTNodeType_YulIfStatement: {
            ASTNodeYulIfStatement *statement = &node->yulIfStatementNode;

            renderCString(r, "if", SPACE);
            renderYulExpression(r, statement->expression, SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_YulForStatement: {
            ASTNodeYulForStatement *statement = &node->yulForStatementNode;

            renderCString(r, "for", SPACE);
            renderStatement(r, statement->variableDeclaration, SPACE);
            renderYulExpression(r, statement->condition, SPACE);
            renderStatement(r, statement->increment, SPACE);
            renderStatement(r, statement->body, NEWLINE);
        } break;
        case ASTNodeType_YulFunctionDefinition: {
            ASTNodeYulFunctionDefinition *statement = &node->yulFunctionDefinitionNode;

            renderCString(r, "function", SPACE);
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

            renderCString(r, "switch", SPACE);
            ConnectType expressionConnect = statement->cases.count > 0 || statement->defaultBlock != 0x0 ? NEWLINE : SPACE;
            renderYulExpression(r, statement->expression, expressionConnect);

            ASTNodeLink *it = statement->cases.head;
            for(u32 i = 0; i < statement->cases.count; i++, it = it->next) {
                renderCString(r, "case", SPACE);
                renderYulExpression(r, it->node.yulCaseNode.literal, SPACE);
                renderStatement(r, it->node.yulCaseNode.block, NEWLINE);
            }

            if(statement->defaultBlock != 0x0) {
                renderCString(r, "default", SPACE);
                renderStatement(r, statement->defaultBlock->yulCaseNode.block, NEWLINE);
            }
        } break;
        case ASTNodeType_YulLeaveStatement: {
            renderCString(r, "leave", NEWLINE);
        } break;
        case ASTNodeType_YulBreakStatement: {
            renderCString(r, "break", NEWLINE);
        } break;
        case ASTNodeType_YulContinueStatement: {
            renderCString(r, "continue", NEWLINE);
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
renderParameters(Render *r, ASTNodeList *list, ConnectType connect, u32 connectIsInclusive) {
	ASTNodeLink *it = list->head;
	for(u32 i = 0; i < list->count; i++, it = it->next) {
		ASTNodeVariableDeclaration *decl = &it->node.variableDeclarationNode;
        ConnectType connections[3] = { SPACE, SPACE, SPACE };
		if(!connectIsInclusive && i == list->count - 1) {
			connect = NONE;
		}

        if(decl->name != INVALID_TOKEN_ID) {
            connections[2] = connect;
        } else {
            if(decl->dataLocation > 0) {
                connections[1] = connect;
            } else {
                connections[0] = connect;
            }
        }

		renderType(r, decl->type, connections[0]);

        if(decl->dataLocation == 1) {
            renderCString(r, "memory", connections[1]);
        } else if(decl->dataLocation == 2) {
            renderCString(r, "storage", connections[1]);
        } else if(decl->dataLocation == 3) {
            renderCString(r, "calldata", connections[1]);
        } else if(decl->dataLocation == 4) {
            renderCString(r, "indexed", connections[1]);
        }

		if(decl->name != INVALID_TOKEN_ID) {
			renderToken(r, decl->name, connections[2]);
		}
	}
}

static void
renderInheritanceSpecifier(Render *r, ASTNode *node, ConnectType connect) {
    ASTNodeInheritanceSpecifier *inheritance = &node->inheritanceSpecifierNode;
    renderType(r, inheritance->identifier, NONE);
    renderCallArgumentList(r, &inheritance->argumentsExpression, &inheritance->argumentsName);
    renderConnect(r, connect);
}

static void
renderModiferInvocations(Render *r, ASTNodeList *modifiers) {
    if(modifiers && modifiers->count > 0) {
        ASTNodeLink *it = modifiers->head;
        for(u32 i = 0; i < modifiers->count; i++, it = it->next) {
            ASTNodeModifierInvocation *invocation = &it->node.modifierInvocationNode;
            renderType(r, invocation->identifier, NONE);
            if(invocation->argumentsExpression.count != -1) {
                renderCallArgumentList(r, &invocation->argumentsExpression, &invocation->argumentsName);
            }
            renderConnect(r, SPACE);
        }
    }
}

static void
renderOverrides(Render *r, u32 hasOverride, ASTNodeList *overrides) {
    if(hasOverride) {
        renderCString(r, "override", NONE);
        if(overrides->count > 0) {
            renderCString(r, "(", NONE);
            ASTNodeLink *override = overrides->head;
            for(u32 i = 0; i < overrides->count; i++, override = override->next) {
                renderType(r, &override->node, i == overrides->count - 1 ? NONE : COMMA_SPACE);
            }
            renderCString(r, ")", NONE);
        }
        renderConnect(r, SPACE);
    }
}

static void
renderMember(Render *r, ASTNode *member) {
    switch(member->type) {
        case ASTNodeType_Pragma: {
        	ASTNodePragma *pragma = &member->pragmaNode;
            renderCString(r, "pragma", SPACE);
        	renderToken(r, pragma->major, SPACE);
    	    TokenId firstTokenId = listGetTokenId(&pragma->following, 0);
		    TokenId lastTokenId = listGetTokenId(&pragma->following, pragma->following.count - 1);
		    String first = getTokenString(r->tokens, firstTokenId);
		    String last = getTokenString(r->tokens, lastTokenId);
		    String string = {
		        .data = first.data,
		        .size = (u32)(last.data - first.data) + last.size,
		    };

            renderString(r, string, SEMICOLON);
        } break;
        case ASTNodeType_Import: {
            assert(stringMatch(LIT_TO_STR("import"), r->tokens.tokenStrings[member->startToken]));
        	renderToken(r, member->startToken, SPACE);

        	if(member->symbols.count > 0) {
                assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 1]));
        		renderToken(r, member->startToken + 1, SPACE);
                u32 endingToken = member->startToken + 2;
			    for(u32 i = 0; i < member->symbols.count; i++) {
        			TokenId symbol = listGetTokenId(&member->symbols, i);
        			TokenId alias = listGetTokenId(&member->symbolAliases, i);

        			ConnectType lastConnect = i == member->symbols.count - 1 ? SPACE : COMMA_SPACE;
        			ConnectType connect = alias == INVALID_TOKEN_ID ? lastConnect : SPACE;
        			renderToken(r, symbol, connect);
        			if(alias != INVALID_TOKEN_ID) {
                        assert(stringMatch(LIT_TO_STR("as"), r->tokens.tokenStrings[alias - 1]));
                        renderToken(r, alias - 1, SPACE);
        				renderToken(r, alias, lastConnect);
        			}
			    }

                assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->pathTokenId - 2]));
                assert(stringMatch(LIT_TO_STR("from"), r->tokens.tokenStrings[member->pathTokenId - 1]));
                renderToken(r, member->pathTokenId - 2, SPACE);
                renderToken(r, member->pathTokenId - 1, SPACE);
        	}

		    ConnectType connect = member->unitAliasTokenId != INVALID_TOKEN_ID ? SPACE : NONE;
		    renderTokenAsString(r, member->pathTokenId, connect);
		    if(member->unitAliasTokenId != INVALID_TOKEN_ID) {
                assert(stringMatch(LIT_TO_STR("as"), r->tokens.tokenStrings[member->unitAliasTokenId - 1]));
		    	renderToken(r, member->unitAliasTokenId - 1, SPACE);
		    	renderToken(r, member->unitAliasTokenId, NONE);
		    }

            renderConnect(r, SEMICOLON);
        } break;
        case ASTNodeType_Using: {
            assert(stringMatch(LIT_TO_STR("using"), r->tokens.tokenStrings[member->startToken]));
			renderToken(r, member->startToken, SPACE);
			ASTNodeUsing *using = &member->usingNode;

			if(using->onLibrary) {
				assert(using->identifiers.count == 1);
				renderType(r, &using->identifiers.head->node, SPACE);
			} else {
				renderCString(r, "{", SPACE);
				ASTNodeLink *it = using->identifiers.head;
				for(u32 i = 0; i < using->identifiers.count; i++, it = it->next) {
					ConnectType connect = i == using->identifiers.count - 1 ? SPACE : COMMA_SPACE;
					u16 operator = listGetU16(&using->operators, i);
					renderType(r, &it->node, operator ? SPACE : NONE);

					if(operator) {
						renderCString(r, "as", SPACE);
                        renderString(r, tokenTypeToString(operator), NONE);
					}

					renderConnect(r, connect);
				}
				renderCString(r, "}", SPACE);
			}

			if(using->forType != 0x0) {
				renderCString(r, "for", SPACE);
				renderType(r, using->forType, using->global ? SPACE : NONE);
			}

			if(using->global) {
                renderCString(r, "global", NONE);
			}

			renderConnect(r, SEMICOLON);
        } break;
        case ASTNodeType_EnumDefinition: {
            renderToken(r, member->startToken, SPACE);
			renderToken(r, member->nameTokenId, SPACE);
            renderToken(r, member->startToken + 2, member->values.count > 0 ? NEWLINE : SPACE);

			pushIndent(r->writer);
			for(u32 i = 0; i < member->values.count; i++) {
				TokenId value = listGetTokenId(&member->values, i);
				renderToken(r, value, COMMA);
			}
			popIndent(r->writer);

            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 2]));
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));
            renderToken(r, member->endToken, NEWLINE);
        } break;
        case ASTNodeType_Struct: {
            renderToken(r, member->startToken, SPACE);
			renderToken(r, member->structNode.nameTokenId, SPACE);
            renderToken(r, member->startToken + 2, member->values.count > 0 ? NEWLINE : SPACE);

			pushIndent(r->writer);
			renderParameters(r, &member->structNode.members, SEMICOLON, 1);
			popIndent(r->writer);

            assert(stringMatch(LIT_TO_STR("{"), r->tokens.tokenStrings[member->startToken + 2]));
            assert(stringMatch(LIT_TO_STR("}"), r->tokens.tokenStrings[member->endToken]));
            renderToken(r, member->endToken, NEWLINE);
        } break;
        case ASTNodeType_Error: {
            assert(stringMatch(LIT_TO_STR("error"), r->tokens.tokenStrings[member->startToken]));
            renderToken(r, member->startToken, SPACE);
			ASTNodeEvent *event = &member->eventNode;
			renderToken(r, event->identifier, NONE);
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[member->startToken + 2]));
            renderToken(r, member->startToken + 2, NONE);

			pushIndent(r->writer);
			renderParameters(r, &member->errorNode.parameters, COMMA_SPACE, 0);
			popIndent(r->writer);

            assert(stringMatch(LIT_TO_STR(")"), r->tokens.tokenStrings[member->endToken - 1]));
            renderToken(r, member->endToken - 1, SEMICOLON);
        } break;
        case ASTNodeType_Event: {
            assert(stringMatch(LIT_TO_STR("event"), r->tokens.tokenStrings[member->startToken]));
            renderToken(r, member->startToken, SPACE);
			ASTNodeEvent *event = &member->eventNode;
			renderToken(r, event->identifier, NONE);
            assert(stringMatch(LIT_TO_STR("("), r->tokens.tokenStrings[member->startToken + 2]));
            renderToken(r, member->startToken + 2, NONE);

			pushIndent(r->writer);
			renderParameters(r, &member->eventNode.parameters, COMMA_SPACE, 0);
			popIndent(r->writer);

            u32 startToken = member->startToken + 3;
            if(member->eventNode.parameters.count > 0) {
                startToken = member->eventNode.parameters.last->node.endToken + 1;
            }

            for(u32 token = startToken; token < member->endToken - 1; token++) {
                renderToken(r, token, SPACE);
            }
            renderToken(r, member->endToken - 1, SEMICOLON);
        } break;
        case ASTNodeType_Typedef: {
            assert(stringMatch(LIT_TO_STR("type"), r->tokens.tokenStrings[member->startToken]));
            assert(stringMatch(LIT_TO_STR("is"), r->tokens.tokenStrings[member->startToken + 2]));

            renderToken(r, member->startToken, SPACE);
    	    ASTNodeTypedef *typedefNode = &member->typedefNode;
    	    renderToken(r, typedefNode->identifier, SPACE);
            renderToken(r, member->startToken + 2, SPACE);
    	    renderType(r, typedefNode->type, SEMICOLON);
        } break;
        case ASTNodeType_ConstVariable: {
    	    ASTNodeConstVariable *constNode = &member->constVariableNode;
    	    renderType(r, constNode->type, SPACE);
    	    renderCString(r, "constant", SPACE);
    	    renderToken(r, constNode->identifier, SPACE);
    	    renderCString(r, "=", SPACE);
    	    renderExpression(r, constNode->expression, SEMICOLON);
        } break;
        case ASTNodeType_StateVariableDeclaration: {
            ASTNodeConstVariable *decl = &member->constVariableNode;

            renderType(r, decl->type, SPACE);
            renderVisibility(r, decl->visibility);
            switch(decl->mutability) {
                case 1: {
                    renderCString(r, "constant", SPACE);
                } break;
                case 2: {
                    renderCString(r, "immutable", SPACE);
                } break;
            }

            renderOverrides(r, decl->override, &decl->overrides);
            renderToken(r, decl->identifier, decl->expression ? SPACE : NONE);

            if(decl->expression) {
                assert(stringMatch(LIT_TO_STR("="), r->tokens.tokenStrings[decl->identifier + 1]));
                renderToken(r, decl->identifier + 1, SPACE);
                renderExpression(r, decl->expression, NONE);
            }
            renderConnect(r, SEMICOLON);
        } break;
        case ASTNodeType_FallbackFunction:
        case ASTNodeType_ReceiveFunction:
        case ASTNodeType_FunctionDefinition: {
    	    ASTNodeFunctionDefinition *function = &member->functionDefinitionNode;
            if(member->type == ASTNodeType_FallbackFunction) {
                assert(stringMatch(LIT_TO_STR("fallback"), r->tokens.tokenStrings[member->startToken]));
                renderToken(r, member->startToken, SPACE);
            } else if(member->type == ASTNodeType_ReceiveFunction) {
                assert(stringMatch(LIT_TO_STR("receive"), r->tokens.tokenStrings[member->startToken]));
                renderToken(r, member->startToken, SPACE);
            } else {
                assert(stringMatch(LIT_TO_STR("function"), r->tokens.tokenStrings[member->startToken]));
                renderToken(r, member->startToken, SPACE);
                if(function->name != INVALID_TOKEN_ID) {
        			renderToken(r, function->name, NONE);
                }
            }

			renderCString(r, "(", NONE);
			renderParameters(r, &member->structNode.members, COMMA_SPACE, 0);
			renderCString(r, ")", SPACE);

            renderVisibility(r, function->visibility);
            renderStateMutability(r, function->stateMutability);

            if(function->virtual) {
                renderCString(r, "virtual", SPACE);
            }

            if(function->override) {
                renderCString(r, "override", SPACE);
                if(function->overrides->count > 0) {
                    renderCString(r, "(", NONE);
                    ASTNodeLink *override = function->overrides->head;
                    for(u32 i = 0; i < function->overrides->count; i++, override = override->next) {
                        renderType(r, &override->node, i == function->overrides->count - 1 ? NONE : COMMA_SPACE);
                    }
                    renderCString(r, ")", NONE);
                }
            }

			if(function->returnParameters) {
				renderCString(r, "returns (", NONE);
				renderParameters(r, function->returnParameters, COMMA_SPACE, 0);
				renderCString(r, ")", SPACE);
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
            renderCString(r, ")", SPACE);

            renderModiferInvocations(r, &constructor->modifiers);

            renderVisibility(r, constructor->visibility);
            renderStateMutability(r, constructor->stateMutability);

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
                renderCString(r, ")", SPACE);
            }

            if(function->virtual) {
                renderCString(r, "virtual", SPACE);
            }

            if(function->override) {
                renderCString(r, "override", NONE);
                if(function->overrides->count > 0) {
                    renderCString(r, "(", NONE);
                    ASTNodeLink *override = function->overrides->head;
                    for(u32 i = 0; i < function->overrides->count; i++, override = override->next) {
                        renderType(r, &override->node, i == function->overrides->count - 1 ? NONE : COMMA_SPACE);
                    }
                    renderCString(r, ")", NONE);
                }
            }

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
		.indentSize = 2,
	};

	Render render = {
		.writer = &writer,
		.tokens = tokens,
        .sourceBaseAddress = originalSource.data,
	};

    u32 startOfTokens = tokens.tokenStrings[tree.startToken].data - originalSource.data;
    renderComments(&render, 0, startOfTokens);
	renderSourceUnit(&render, &tree);
	return (String){ .data = writer.data, .size = writer.size };
}
