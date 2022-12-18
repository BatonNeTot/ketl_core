﻿/*🍲Ketl🍲*/
#ifndef context_h
#define context_h

#include "type.h"

#include <cinttypes>
#include <string>
#include <unordered_map>
#include <memory>
#include <optional>
#include <typeindex>
#include <iostream>
#include <functional>

namespace Ketl {

	class BasicTypeBody;
	class Context;

	class Variable {
	public:

		Variable() {}
		Variable(void* data, std::unique_ptr<const Type>&& type)
			: _data(data), _type(std::move(type)) {}

		template <class T, class... Args>
		T* call(StackAllocator& stack, Args&&... args) {
			auto* function = as<FunctionContainer>();
			auto* pureFunction = function->functions;

			auto stackPtr = stack.allocate(pureFunction->stackSize());
			uint8_t returnData[16]; // TODO
			pureFunction->call(stack, stackPtr, returnData);
			stack.deallocate(pureFunction->stackSize());
			return reinterpret_cast<T*>(returnData);
		}

		template <class T>
		T* as() {
			return reinterpret_cast<T*>(_data);
		}

		const Type* type() const {
			return _type.get();
		}

		void data(void* data) {
			_data = data;
		}

	private:

		void* _data = nullptr;
		std::unique_ptr<const Type> _type;

	};

	class ContextedVariable {
	public:

		ContextedVariable(Context& context, Variable& variable)
			: _context(context), _var(variable) {}

		template <class... Args>
		void operator()(Args&&... args);

		template <class T>
		T* as() {
			return _var.as<T>();
		}

		const Type* type() const {
			return _var.type();
		}

	private:
		Context& _context;
		Variable& _var;
	};

	class Context {
	public:

		Context(Allocator& allocator, uint64_t globalStackSize);

		// maybe would be better to return pointer and make it null in case of lack
		ContextedVariable getVariable(const std::string& id) {
			auto it = _globals.find(id);
			return ContextedVariable(*this, it == _globals.end() ? _emptyVar : it->second);
		}

		template <class T>
		BasicTypeBody* declareType(const std::string& id) {
			return declareType(id, sizeof(T));
		}

		template <>
		BasicTypeBody* declareType<void>(const std::string& id) {
			return declareType(id, 0);
		}

		BasicTypeBody* declareType(const std::string& id, uint64_t sizeOf);

		bool declareGlobal(const std::string& id, void* stackPtr, const Type& type) {
			auto [it, success] = _globals.try_emplace(id, stackPtr, Type::clone(type));
			return success;
		}

		template <class T>
		T* declareGlobal(const std::string& id, const Type& type) {
			auto [it, success] = _globals.try_emplace(id, nullptr, Type::clone(type));
			if (success) {
				auto valuePtr = reinterpret_cast<Type*>(allocateOnGlobalStack(*it->second.type()));
				it->second.data(valuePtr);
			}
			return it->second.as<T>();
		}

		uint8_t* allocateOnGlobalStack(const Type& type) {
			auto ptr = _globalStack.allocate(type.sizeOf());
			//type.construct(ptr);
			return ptr;
		}

	public: // TODO

		static Variable _emptyVar;

		StackAllocator _globalStack;
		std::unordered_map<std::string, Variable> _globals;
	};

	template <class... Args>
	void ContextedVariable::operator()(Args&&... args) {
		_var.call<void>(_context._globalStack, std::forward<Args>(args)...);
	}
}

#endif /*context_h*/