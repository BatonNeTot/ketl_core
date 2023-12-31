﻿//🍲ketl
#ifndef compiler_syntax_node_h
#define compiler_syntax_node_h

#include "ketl/operators.h"
#include "ketl/utils.h"

#include <inttypes.h>

typedef uint8_t KETLSyntaxNodeType;

#define KETL_SYNTAX_NODE_TYPE_BLOCK 0
#define KETL_SYNTAX_NODE_TYPE_SIMPLE_EXPRESSION 1
#define KETL_SYNTAX_NODE_TYPE_DEFINE_VAR 2
#define KETL_SYNTAX_NODE_TYPE_DEFINE_VAR_OF_TYPE 3

#define KETL_SYNTAX_NODE_TYPE_ID 4
#define KETL_SYNTAX_NODE_TYPE_NUMBER 5
#define KETL_SYNTAX_NODE_TYPE_STRING_LITERAL 6

#define KETL_SYNTAX_NODE_TYPE_IF_ELSE 7
#define KETL_SYNTAX_NODE_TYPE_RETURN 8

#define KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET	100
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_PLUS (KETL_OPERATOR_CODE_BI_PLUS + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_MINUS (KETL_OPERATOR_CODE_BI_MINUS + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_PROD (KETL_OPERATOR_CODE_BI_PROD + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_DIV (KETL_OPERATOR_CODE_BI_DIV + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_EQUAL (KETL_OPERATOR_CODE_BI_EQUAL + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_UNEQUAL (KETL_OPERATOR_CODE_BI_UNEQUAL + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)
#define KETL_SYNTAX_NODE_TYPE_OPERATOR_BI_ASSIGN (KETL_OPERATOR_CODE_BI_ASSIGN + KETL_SYNTAX_NODE_TYPE_OPERATOR_OFFSET)

KETL_DEFINE(KETLSyntaxNode) {
	uint32_t lineInSource;
	uint32_t columnInSource;
	uint32_t length;
	KETLSyntaxNodeType type;
	KETLSyntaxNode* nextSibling;
	union {
		const char* value;
		KETLSyntaxNode* firstChild;
	};
};

#endif /*compiler_syntax_node_h*/
