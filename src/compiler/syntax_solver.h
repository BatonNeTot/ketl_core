﻿//🍲ketl
#ifndef compiler_syntax_solver_h
#define compiler_syntax_solver_h

#include "ketl/common.h"

#include <stdbool.h>

KETL_FORWARD(KETLSyntaxNode);

KETL_FORWARD(KETLToken);
KETL_FORWARD(KETLBnfNode);
KETL_FORWARD(KETLObjectPool);

KETLSyntaxNode* ketlSolveBnf(KETLToken* firstToken, KETLBnfNode* scheme, KETLObjectPool* syntaxNodePool);

#endif /*compiler_bnf_solver_h*/