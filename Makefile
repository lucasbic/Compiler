all: tokenizer parser build

tokenizer:
	flex c-.l

parser:
	bison -d c-.y

build:
	gcc -c *.c -fno-builtin-exp -Wno-implicit-function-declaration
	gcc *.o -lfl -o c- -fno-builtin-exp

clean:
	rm -f c-
	rm -f lex.yy.c
	rm -f *.o
	rm -f c-.tab.*
