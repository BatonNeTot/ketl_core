//🍲ketl
#include "ketl/compiler/syntax_solver.h"

#include "bnf_parser.h"
#include "syntax_parser.h"

#include "token.h"
#include "lexer.h"
#include "ketl/compiler/syntax_node.h"
#include "bnf_node.h"
#include "bnf_scheme.h"
#include "ketl/object_pool.h"
#include "ketl/stack.h"
#include "ketl/assert.h"

void ketlInitSyntaxSolver(KETLSyntaxSolver* syntaxSolver) {
	ketlInitObjectPool(&syntaxSolver->tokenPool, sizeof(KETLToken), 16);
	ketlInitObjectPool(&syntaxSolver->bnfNodePool, sizeof(KETLBnfNode), 16);
	ketlInitStack(&syntaxSolver->bnfStateStack, sizeof(KETLBnfParserState), 32);
	syntaxSolver->bnfScheme = ketlBuildBnfScheme(&syntaxSolver->bnfNodePool);
}

void ketlDeinitSyntaxSolver(KETLSyntaxSolver* syntaxSolver) {
	ketlDeinitStack(&syntaxSolver->bnfStateStack);
	ketlDeinitObjectPool(&syntaxSolver->bnfNodePool);
	ketlDeinitObjectPool(&syntaxSolver->tokenPool);
}

KETLSyntaxNode* ketlSolveSyntax(const char* source, size_t length, KETLSyntaxSolver* syntaxSolver, KETLObjectPool* syntaxNodePool) {
	KETLStack* bnfStateStack = &syntaxSolver->bnfStateStack;
	KETLObjectPool* tokenPool = &syntaxSolver->tokenPool;

	KETLLexer lexer;
	ketlInitLexer(&lexer, source, length, tokenPool);

	if (!ketlHasNextToken(&lexer)) {
		KETLSyntaxNode* node = ketlGetFreeObjectFromPool(syntaxNodePool);

		node->type = KETL_SYNTAX_NODE_TYPE_BLOCK;
		node->positionInSource = 0;
		node->value = 0;
		node->firstChild = NULL;
		node->nextSibling = NULL;
		node->length = 0;

		return node;
	}
	KETLToken* firstToken = ketlGetNextToken(&lexer);
	KETLToken* token = firstToken;

	while (ketlHasNextToken(&lexer)) {
		token = token->next = ketlGetNextToken(&lexer);
	}
	token->next = NULL;

	{
		KETLBnfParserState* initialSolver = ketlPushOnStack(bnfStateStack);
		initialSolver->bnfNode = syntaxSolver->bnfScheme;
		initialSolver->token = firstToken;
		initialSolver->tokenOffset = 0;

		initialSolver->parent = NULL;
	}

	KETLBnfErrorInfo error;
	{
		error.maxToken = NULL;
		error.maxTokenOffset = 0;
		error.bnfNode = NULL;
	}

	bool success = ketlParseBnf(bnfStateStack, &error);
	if (KETL_CHECK_VOEM(success, "Failed to parse source", 0)) {
		// TODO proper log error
		return NULL;
	}

	KETLStackIterator iterator;
	ketlInitStackIterator(&iterator, bnfStateStack);

	KETLSyntaxNode* rootSyntaxNode = ketlParseSyntax(syntaxNodePool, &iterator);

	ketlResetStack(bnfStateStack);
	ketlResetPool(tokenPool);
	
	return rootSyntaxNode;
}