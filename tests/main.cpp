/*🍲Ketl🍲*/
#include <iostream>

extern "C" {
#include "ketl/compiler/bytecode_compiler.h"
}

// TODO rethink iterators using int map iterator as an example
// TODO rethink getOrCreate in int map

int main(int argc, char** argv) {	
	auto source = R"({
	var test1 := 10;
	i32 test2 := 15;
	if (test1 == test2) {
		var test3 := test2 - test1;
		return test3;
	} else {
		return test1 * test2;
	}
})";

	KETLBytecodeCompiler ketlCompiler;

	ketlInitBytecodeCompiler(&ketlCompiler);

	auto root = ketlSolveSyntax(source, KETL_NULL_TERMINATED_LENGTH, &ketlCompiler.syntaxSolver, &ketlCompiler.syntaxNodePool);

	return 0;
}