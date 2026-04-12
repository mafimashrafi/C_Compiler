#!/bin/bash

# Compiler build script for two-pass compilation with AST and three-address code generation

echo "Building two-pass compiler..."

# Generate parser from yacc file
yacc -d -y --debug --verbose 22101046.y
echo 'Generated the parser C file and header file'

# Compile parser
g++ -w -c -o y.o y.tab.c
echo 'Generated the parser object file'

# Generate scanner from lex file
flex 22101046.l
echo 'Generated the scanner C file'

# Compile scanner
g++ -fpermissive -w -c -o l.o lex.yy.c
echo 'Generated the scanner object file'

# Link and create executable
g++ y.o l.o -o two_pass_compiler
echo 'All ready, running the two-pass compiler...'

# Check if input file argument is provided
if [ $# -eq 0 ]; then
    INPUT_FILE="input.c"
else
    INPUT_FILE=$1
fi

# Run the compiler on the input file
./two_pass_compiler $INPUT_FILE
echo 'Compilation completed.'

# Display output files
echo ''
echo '============ Log output ============'
cat log.txt
echo ''
echo '============ Error output ============'
cat error.txt
echo ''
echo '============ Three Address Code ============'
cat code.txt