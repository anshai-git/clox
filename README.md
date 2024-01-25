# CLOX
clox implementation from the book "Crafting Interpreters" by Robert Nystrom

SOURCE CODE -> [SCANNER] -> TOKENS -> [COMPILER] -> BYTECODE CHUNK -> [VM]

## Parsing

We map each token type to a different kind of expression. We define a function
for each expression that outputs the appropriate bytecode. Then we build an
array of function pointers. The indexes in the array correspond to the
TokenType enum values, and the function at each index is the code to compile
an expression of that token type.

Parsing strategy: **Pratt parser**

A Pratt parser isn’t a recursive descent parser, but it’s still recursive.
That’s to be expected since the grammar itself is recursive. 