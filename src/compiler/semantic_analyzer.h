﻿/*🍲Ketl🍲*/
#ifndef compiler_semantic_analyzer_h
#define compiler_semantic_analyzer_h

#include "common.h"
#include "parser.h"
#include "context.h"

#include <stack>
#include <map>

namespace Ketl {

	class SemanticAnalyzer {
	public:

		SemanticAnalyzer(Context& context, bool localScope = false)
			: _context(context), _localScope(localScope) {}
		~SemanticAnalyzer() {
			for (auto& functionPtr : newFunctions) {
				functionPtr->~FunctionImpl();
				_context._alloc.deallocate(functionPtr);
			}
		}

		std::variant<FunctionImpl, std::string> compile(std::unique_ptr<IRNode>&& block)&&;

		void bakeContext();

		AnalyzerVar* createTempVar(const TypeObject& type);

		AnalyzerVar* createLiteralVar(const std::string_view& value);

		AnalyzerVar* createFunctionVar(FunctionImpl&& function);

		AnalyzerVar* createFunctionArgumentVar(uint64_t index);
		AnalyzerVar* createFunctionParameterVar(const std::string_view& id, const TypeObject& type);

		AnalyzerVar* getVar(const std::string_view& id);
		AnalyzerVar* createVar(const std::string_view& id, const TypeObject& type);

		void pushErrorMsg(const std::string& msg) {
			_compilationErrors.emplace_back(msg);
		}

		bool hasCompilationErrors() const { return !_compilationErrors.empty(); }
		std::string compilationErrors() const;

		Context& context() const {
			return _context;
		}

		bool isLocalScope() const {
			return _localScope;
		}

		void pushScope();
		void popScope();

		void propagateScopeDestructors() {} // TODO

		uint64_t scopeLayer = 0u;
		uint64_t currentStackOffset = 0u;
		uint64_t maxOffsetValue = 0u;

		std::vector<std::unique_ptr<AnalyzerVar>> vars;

		struct ScopeVar {
			AnalyzerVar* var; 
			const TypeObject* type;
			uint64_t scopeLayer;
		};

		std::stack<ScopeVar> scopeVars;
		std::stack<uint64_t> scopeOffsets;
		std::unordered_map<std::string_view, std::map<uint64_t, AnalyzerVar*>> scopeVarsByNames;
		std::unordered_map<std::string_view, AnalyzerVar*> newGlobalVars;

		std::vector<FunctionImpl*> newFunctions;

		Context& _context;

		std::vector<std::string> _compilationErrors;

		bool _localScope;
	};

}

#endif /*compiler_semantic_analyzer_h*/