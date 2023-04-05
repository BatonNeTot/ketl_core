﻿/*🍲Ketl🍲*/
#include "check_tests.h"

static auto registerTests = []() {

	registerTest("If else statement true", []() {
		Ketl::Allocator allocator;
		Ketl::Context context(allocator, 4096);
		Ketl::Compiler compiler;

		int64_t sum = 0;
		context.declareGlobal("sum", &sum);

		auto compilationResult = compiler.compile(R"(
			sum = 3;

			if (sum == 3) {
				sum = 5;
			} else {
				sum = 7;
			}
		)", context);

		if (std::holds_alternative<std::string>(compilationResult)) {
			std::cerr << std::get<std::string>(compilationResult) << std::endl;
			return false;
		}

		auto& command = std::get<0>(compilationResult);
		command();

		return sum == 5;
		});

	registerTest("If else statement false", []() {
		Ketl::Allocator allocator;
		Ketl::Context context(allocator, 4096);
		Ketl::Compiler compiler;

		int64_t sum = 0;
		context.declareGlobal("sum", &sum);

		auto compilationResult = compiler.compile(R"(
			sum = 4;

			if (sum == 3) {
				sum = 5;
			} else {
				sum = 7;
			}
		)", context);

		if (std::holds_alternative<std::string>(compilationResult)) {
			std::cerr << std::get<std::string>(compilationResult) << std::endl;
			return false;
		}

		auto& command = std::get<0>(compilationResult);
		command();

		return sum == 7;
		});

	registerTest("Single if statement true", []() {
		Ketl::Allocator allocator;
		Ketl::Context context(allocator, 4096);
		Ketl::Compiler compiler;

		int64_t sum = 0;
		context.declareGlobal("sum", &sum);

		auto compilationResult = compiler.compile(R"(
			sum = 3;

			if (sum == 3) {
				sum = 5;
			}
		)", context);

		if (std::holds_alternative<std::string>(compilationResult)) {
			std::cerr << std::get<std::string>(compilationResult) << std::endl;
			return false;
		}

		auto& command = std::get<0>(compilationResult);
		command();

		return sum == 5;
		});

	registerTest("Empty if statement false", []() {
		Ketl::Allocator allocator;
		Ketl::Context context(allocator, 4096);
		Ketl::Compiler compiler;

		int64_t sum = 0;
		context.declareGlobal("sum", &sum);

		auto compilationResult = compiler.compile(R"(
			sum = 3;

			if (sum == 3) {} else {
				sum = 5;
			}
		)", context);

		if (std::holds_alternative<std::string>(compilationResult)) {
			std::cerr << std::get<std::string>(compilationResult) << std::endl;
			return false;
		}

		auto& command = std::get<0>(compilationResult);
		command();

		return sum == 3;
		});

	registerTest("Empty if statement true", []() {
		Ketl::Allocator allocator;
		Ketl::Context context(allocator, 4096);
		Ketl::Compiler compiler;

		int64_t sum = 0;
		context.declareGlobal("sum", &sum);

		auto compilationResult = compiler.compile(R"(
			sum = 4;

			if (sum == 3) {} else {
				sum = 5;
			}
		)", context);

		if (std::holds_alternative<std::string>(compilationResult)) {
			std::cerr << std::get<std::string>(compilationResult) << std::endl;
			return false;
		}

		auto& command = std::get<0>(compilationResult);
		command();

		return sum == 5;
		});

	registerTest("Multiple if statement layers", []() {
		Ketl::Allocator allocator;
		Ketl::Context context(allocator, 4096);
		Ketl::Compiler compiler;

		int64_t sum = 0;
		context.declareGlobal("sum", &sum);

		auto compilationResult = compiler.compile(R"(
			sum = 0;
			var first = 3;
			var second = 4;

			if (first == 3) 
				if (second == 4)
					sum = 4;
		)", context);

		if (std::holds_alternative<std::string>(compilationResult)) {
			std::cerr << std::get<std::string>(compilationResult) << std::endl;
			return false;
		}

		auto& command = std::get<0>(compilationResult);
		command();

		return sum == 4;
		});

	return false;
	};
	
BEFORE_MAIN_ACTION(registerTests);