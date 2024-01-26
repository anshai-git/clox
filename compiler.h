#ifndef clox_compiler_h
#define clox_compiler_h

#include "vm.h"

// We pass in the chunk where the compiler will write the code, and then
// compile() returns whether or not compilation succeeded.
bool compile(const char* source, Chunk* chunk);

#endif
