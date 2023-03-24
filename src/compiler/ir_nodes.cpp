﻿/*🍲Ketl🍲*/
#include "ir_nodes.h"

#include "bnf_nodes.h"
#include "garbage_collector.h"

namespace Ketl {

	class IRBlock : public IRNode {
	public:

		IRBlock(std::vector<std::unique_ptr<IRNode>>&& commands)
			: _commands(std::move(commands)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			for (const auto& command : _commands) {
				command->produceInstructions(instructions, context);
			}
			return {};
		}

	private:
		std::vector<std::unique_ptr<IRNode>> _commands;
	};

	std::unique_ptr<IRNode> createBlockTree(const ProcessNode* info) {
		std::vector<std::unique_ptr<IRNode>> commands;

		auto it = info->firstChild;
		while (it) {
			commands.emplace_back(it->node->createIRTree(it));
			it = it->nextSibling;
		}

		return std::make_unique<IRBlock>(std::move(commands));
	}

	std::unique_ptr<IRNode> proxyTree(const ProcessNode* info) {
		auto child = info->firstChild;
		return child->node->createIRTree(child);
	}

	std::unique_ptr<IRNode> emptyTree(const ProcessNode*) {
		return {};
	}

	class IRDefineVariable : public IRNode {
	public:

		IRDefineVariable(std::string_view id, std::unique_ptr<IRNode>&& type, std::unique_ptr<IRNode>&& expression)
			: _id(id), _type(std::move(type)), _expression(std::move(expression)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			auto expression = _expression->produceInstructions(instructions, context);

			const TypeObject* type = nullptr;

			if (_type) {
				type = reinterpret_cast<TypeObject*>(context.evaluate(*_type).as(typeid(TypeObject), context.context())); // TODO hide into var
			}
			else {
				type = expression.getVarAsItIs()->getType();
			}
			auto var = context.createVar(_id, *type);

			auto& instruction = instructions.addInstruction();
			instruction.firstVar = var;
			instruction.secondVar = expression.getVarAsItIs();
			instruction.code = Instruction::Code::DefinePrimitive;
			
			return {};
		};

	private:
		std::string_view _id;
		std::unique_ptr<IRNode> _type;
		std::unique_ptr<IRNode> _expression;
	};

	std::unique_ptr<IRNode> createDefineVariable(const ProcessNode* info) {
		auto typeNode = info->firstChild;
		auto type = typeNode->node->createIRTree(typeNode);

		auto idNode = typeNode->nextSibling;
		auto id = idNode->node->value(idNode->iterator);

		return std::make_unique<IRDefineVariable>(id, std::move(type), nullptr);
	}

	std::unique_ptr<IRNode> createDefineVariableByAssignment(const ProcessNode* info) {
		auto typeNode = info->firstChild;
		auto type = typeNode->node->createIRTree(typeNode);

		auto idNode = typeNode->nextSibling;
		auto id = idNode->node->value(idNode->iterator);

		auto expressionNode = idNode->nextSibling;
		auto expression = expressionNode->node->createIRTree(expressionNode);

		return std::make_unique<IRDefineVariable>(id, std::move(type), std::move(expression));
	}

	class IRFunction : public IRNode {
	public:

		struct Parameter {
			std::unique_ptr<IRNode> type;
			std::string_view id;
			bool isConst;
			bool isRef;
		};

		IRFunction(std::vector<Parameter>&& parameters, std::unique_ptr<IRNode>&& outputType, std::unique_ptr<IRNode>&& block)
			: _parameters(std::move(parameters)), _outputType(std::move(outputType)), _block(std::move(block)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			SemanticAnalyzer analyzer(context.context(), &context);

			TypeObject* returnType = nullptr; 
			if (_outputType) {
				returnType = reinterpret_cast<TypeObject*>(context.evaluate(*_outputType).as(typeid(TypeObject), context.context()));
			}
			else {
				// TODO deduce from block
				returnType = context.context().getVariable("Int64").as<TypeObject>();
			} 
			std::vector<FunctionTypeObject::Parameter> parameters;
			
			for (auto& parameter : _parameters) {
				auto type = reinterpret_cast<TypeObject*>(context.evaluate(*parameter.type).as(typeid(TypeObject), context.context())); // TODO hide into var
				analyzer.createFunctionParameterVar(parameter.id, *type);
				auto isConst = parameter.isConst;
				auto isRef = parameter.isRef;
				parameters.emplace_back(isConst, isRef, type);
			}

			auto function = std::move(analyzer).compile(*_block);
			if (std::holds_alternative<std::string>(function)) {
				context.pushErrorMsg(std::get<std::string>(function));
				return &context._undefinedVar;
			}
			
			auto classType = context.context().getVariable("ClassType").as<TypeObject>();
			auto [functionType, refHolder] = context.context().createObject<FunctionTypeObject>(*returnType, std::move(parameters));
			refHolder->registerAbsLink(classType);
			refHolder->registerAbsLink(returnType);
			for (const auto& type : functionType->getParameters()) {
				refHolder->registerAbsLink(type.type);
			}

			return context.createFunctionVar(std::get<0>(function), *functionType);
		};

	private:
		std::vector<Parameter> _parameters;
		std::unique_ptr<IRNode> _outputType;
		std::unique_ptr<IRNode> _block;
	};

	static void createParameter(const ProcessNode* parameter, std::vector<IRFunction::Parameter>& parameters) {
		auto parameterQualifiers = parameter->firstChild;
		auto isConst = false;
		auto isRef = false;
		for (auto it = parameterQualifiers->firstChild; it != nullptr; it = it->nextSibling) {
			auto qualifier = it->node->value(it->iterator);
			if (qualifier == "const") {
				isConst = true;
			}
			else if (qualifier == "in") {
				isRef = true;
			}
			else if (qualifier == "out") {
				isRef = true;
			}
		}

		auto parameterTypeNode = parameterQualifiers->nextSibling;
		auto parameterType = parameterTypeNode->node->createIRTree(parameterTypeNode);

		auto parameterIdNode = parameterTypeNode->nextSibling;
		auto parameterId = parameterIdNode->node->value(parameterIdNode->iterator);

		parameters.emplace_back(std::move(parameterType), parameterId, isConst, isRef);
	}

	std::unique_ptr<IRNode> createLambda(const ProcessNode* info) {
		std::vector<IRFunction::Parameter> parameters;
		auto parametersNode = info->firstChild;

		for (auto it = parametersNode->firstChild; it != nullptr; it = it->nextSibling) {
			auto parameter = it->firstChild;

			createParameter(parameter, parameters);
		}

		auto outputTypeNodeHolder = parametersNode->nextSibling;
		std::unique_ptr<IRNode> outputType;

		if (outputTypeNodeHolder->firstChild) {
			auto outputTypeNode = outputTypeNodeHolder->firstChild;
			outputType = outputTypeNode->node->createIRTree(outputTypeNode);
		}

		auto blockNode = outputTypeNodeHolder->nextSibling;
		auto block = blockNode->node->createIRTree(blockNode);

		return std::make_unique<IRFunction>(std::move(parameters), std::move(outputType), std::move(block));
	}

	std::unique_ptr<IRNode> createDefineFunction(const ProcessNode* info) {
		auto outputTypeNode = info->firstChild;
		auto outputType = outputTypeNode->node->createIRTree(outputTypeNode);

		auto idNode = outputTypeNode->nextSibling;
		auto id = idNode->node->value(idNode->iterator);

		std::vector<IRFunction::Parameter> parameters;
		auto parametersNode = idNode->nextSibling;

		for (auto it = parametersNode->firstChild; it != nullptr; it = it->nextSibling) {
			auto parameter = it->firstChild;

			createParameter(parameter, parameters);
		}

		auto blockNode = parametersNode->nextSibling;
		auto block = blockNode->node->createIRTree(blockNode);

		auto function = std::make_unique<IRFunction>(std::move(parameters), std::move(outputType), std::move(block));

		return std::make_unique<IRDefineVariable>(id, nullptr, std::move(function));
	}

	class IRStruct : public IRNode {
	public:

		struct Field {
			TypeAccessModifier accessMofifier;
			std::unique_ptr<IRNode> type;
			std::string_view id;
		};

		IRStruct(const std::string_view& id, std::vector<Field>&& fields)
			: _id(id), _fields(std::move(fields)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			__debugbreak();

			return {};
		};

	private:
		std::string_view _id;
		std::vector<Field> _fields;
	};

	std::unique_ptr<IRNode> createDefineStruct(const ProcessNode* info) {
		auto idNode = info->firstChild;
		auto id = idNode->node->value(idNode->iterator);

		std::vector<IRStruct::Field> fields;
		auto globalAccessModifier = TypeAccessModifier::Public;
		for (auto it = idNode->nextSibling; it != nullptr; it = it->nextSibling) {
			auto qualifierNode = it->firstChild;
			if (qualifierNode->nextSibling == nullptr) {
				globalAccessModifier = modifierFromString(qualifierNode->node->value(qualifierNode->iterator));
				continue;
			}
			auto localAccessModifier = globalAccessModifier;
			if (qualifierNode->firstChild != nullptr) {
				auto node = qualifierNode->firstChild;
				localAccessModifier = modifierFromString(node->node->value(node->iterator));
			}

			auto typeNode = qualifierNode->nextSibling;
			auto type = typeNode->node->createIRTree(typeNode);

			for (auto fieldNode = typeNode->nextSibling; fieldNode != nullptr; fieldNode = fieldNode->nextSibling) {
				auto fieldId = fieldNode->node->value(fieldNode->iterator);
				fields.emplace_back(localAccessModifier, std::move(type), fieldId);
			}
		}

		return std::make_unique<IRDefineVariable>(id, nullptr, std::make_unique<IRStruct>(id, std::move(fields)));
	}


	class IRDotOperator : public IRNode {
	public:

		IRDotOperator(std::unique_ptr<IRNode>&& lhs, std::unique_ptr<IRNode>&& rhs)
			: _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			auto lhsUVar = _lhs->produceInstructions(instructions, context);
			auto lhsVar = lhsUVar.getVarAsItIs();
			if (lhsVar != nullptr) {
				context.pushErrorMsg("[ERROR] Can't determin variable");
				return &context._undefinedVar;
			}
			auto rhsVar = _rhs->produceInstructions(instructions, context, lhsVar);

			__debugbreak();
			return nullptr;
		};

	private:

		OperatorCode _op;
		std::unique_ptr<IRNode> _lhs;
		std::unique_ptr<IRNode> _rhs;
	};

	std::unique_ptr<IRNode> createDotTree(const ProcessNode* info) {
		auto leftArgNode = info->firstChild;

		auto leftArg = leftArgNode->node->createIRTree(leftArgNode);

		do {
			auto opNode = leftArgNode->nextSibling;

			auto rightArgNode = opNode->nextSibling;
			auto rightArg = rightArgNode->node->createIRTree(rightArgNode);

			leftArg = std::make_unique<IRDotOperator>(std::move(leftArg), std::move(rightArg));

			leftArgNode = rightArgNode;
		} while (leftArgNode->nextSibling);

		return leftArg;
	}


	class IRFunctionCall : public IRNode {
	public:

		IRFunctionCall(std::unique_ptr<IRNode>&& caller, std::vector<std::unique_ptr<IRNode>>&& arguments)
			: _caller(std::move(caller)), _arguments(std::move(arguments)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			auto callerVar = std::move(*_caller).produceInstructions(instructions, context);

			std::vector<UndeterminedVar> arguments;
			arguments.reserve(_arguments.size());

			for (const auto& argument : _arguments) {
				// TODO create temporary instruction sequences for each argument to correctly merge with possible casting
				arguments.emplace_back(argument->produceInstructions(instructions, context));
			}

			return context.deduceFunctionCall(callerVar, arguments, instructions);
		};

	private:
		std::unique_ptr<IRNode> _caller;
		std::vector<std::unique_ptr<IRNode>> _arguments;
	};


	std::unique_ptr<IRNode> createSecondPrecedence(const ProcessNode* info) {
		auto callerNode = info->firstChild;
		auto caller = callerNode->node->createIRTree(callerNode);

		for (auto opNode = callerNode->nextSibling; opNode != nullptr;) {
			auto op = opNode->node->value(opNode->iterator);

			if (op != "(") {
				// TODO
				return nullptr;
			}

			std::vector<std::unique_ptr<IRNode>> arguments;
			auto argumentsNode = opNode->nextSibling;
			for (auto argumentNode = argumentsNode->firstChild; argumentNode != nullptr; argumentNode = argumentNode->nextSibling) {
				arguments.emplace_back(argumentNode->node->createIRTree(argumentNode));
			}

			caller = std::make_unique<IRFunctionCall>(std::move(caller), std::move(arguments));
			opNode = argumentsNode->nextSibling;
		}

		return caller;
	}


	class IRBinaryOperator : public IRNode {
	public:

		IRBinaryOperator(OperatorCode op, std::unique_ptr<IRNode>&& lhs, std::unique_ptr<IRNode>&& rhs)
			: _op(op), _lhs(std::move(lhs)), _rhs(std::move(rhs)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			auto lhsVar = _lhs->produceInstructions(instructions, context);
			auto rhsVar = _rhs->produceInstructions(instructions, context);

			return context.deduceBinaryOperatorCall(_op, lhsVar, rhsVar, instructions);
		};

	private:

		OperatorCode _op;
		std::unique_ptr<IRNode> _lhs;
		std::unique_ptr<IRNode> _rhs;
	};

	std::unique_ptr<IRNode> createRtlTree(const ProcessNode* info) {
		auto rightArgNode = info->firstChild;

		while (rightArgNode->nextSibling) {
			rightArgNode = rightArgNode->nextSibling;
		}

		auto rightArg = rightArgNode->node->createIRTree(rightArgNode);

		do {
			auto opNode = rightArgNode->prevSibling;
			auto op = opNode->node->value(opNode->iterator);

			auto leftArgNode = opNode->prevSibling;
			auto leftArg = leftArgNode->node->createIRTree(leftArgNode);

			rightArg = std::make_unique<IRBinaryOperator>(parseOperatorCode(op), std::move(leftArg), std::move(rightArg));

			rightArgNode = leftArgNode;
		} while (rightArgNode->prevSibling);

		return rightArg;
	}

	std::unique_ptr<IRNode> createLtrTree(const ProcessNode* info) {
		auto leftArgNode = info->firstChild;

		auto leftArg = leftArgNode->node->createIRTree(leftArgNode);

		do {
			auto opNode = leftArgNode->nextSibling;
			auto op = opNode->node->value(opNode->iterator);

			auto rightArgNode = opNode->nextSibling;
			auto rightArg = rightArgNode->node->createIRTree(rightArgNode);

			leftArg = std::make_unique<IRBinaryOperator>(parseOperatorCode(op), std::move(leftArg), std::move(rightArg));

			leftArgNode = rightArgNode;
		} while (leftArgNode->nextSibling);

		return leftArg;
	}



	class IRVariable : public IRNode {
	public:

		IRVariable(const std::string_view& id)
			: _id(id) {}
		IRVariable(const std::string_view& id, std::unique_ptr<IRNode>&& type)
			: _id(id), _type(std::move(type)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			// TODO create actual undetermined var based on name resolution variations
			return context.getVar(_id);
		};

	private:
		std::string _id;
		std::unique_ptr<IRNode> _type;
	};

	std::unique_ptr<IRNode> createVariable(const ProcessNode* info) {
		return std::make_unique<IRVariable>(info->node->value(info->iterator));
	}


	std::unique_ptr<IRNode> createType(const ProcessNode* info) {
		auto idNode = info->firstChild;
		return std::make_unique<IRVariable>(idNode->node->value(idNode->iterator));
	}



	class IRLiteral : public IRNode {
	public:

		IRLiteral(const std::string_view& value)
			: _value(value) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			return context.createLiteralVar(_value);
		};

	private:
		std::string_view _value;
	};

	std::unique_ptr<IRNode> createLiteral(const ProcessNode* info) {
		return std::make_unique<IRLiteral>(info->node->value(info->iterator));
	}

	class IRReturn : public IRNode {
	public:

		IRReturn(std::unique_ptr<IRNode>&& expression)
			: _expression(std::move(expression)) {}

		UndeterminedVar produceInstructions(InstructionSequence& instructions, SemanticAnalyzer& context, AnalyzerVar*) const override {
			auto expression = _expression->produceInstructions(instructions, context);

			// TODO remove later, cause there might be type cast
			// <!--
			auto& instruction = instructions.addInstruction();
			instruction.firstVar = context.createReturnVar(expression.getVarAsItIs());
			instruction.secondVar = expression.getVarAsItIs();
			instruction.code = Instruction::Code::DefinePrimitive;
			// -->

			instructions.addReturnStatement(expression);

			return nullptr;
		};

	private:
		std::unique_ptr<IRNode> _expression;
	};

	std::unique_ptr<IRNode> createReturn(const ProcessNode* info) {
		auto expresstionNode = info->firstChild;
		auto expression = expresstionNode->node->createIRTree(expresstionNode);

		return std::make_unique<IRReturn>(std::move(expression));
	}

}