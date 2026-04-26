OBJECTS= ./build/compiler.o ./build/cprocess.o ./build/token.o ./build/lex_process.o ./build/lexer.o ./build/helpers/buffer.o ./build/helpers/vector.o 
INCLUDES= -I./
BUILD_DIRS= ./build ./build/helpers

all: dirs ${OBJECTS}
	gcc main.c ${INCLUDES} ${OBJECTS} -g -o ./main

dirs:
	mkdir -p ${BUILD_DIRS}

./build/compiler.o: ./compiler.c
	gcc compiler.c ${INCLUDES} -o ./build/compiler.o -g -c

./build/cprocess.o: ./cprocess.c
	gcc cprocess.c ${INCLUDES} -o ./build/cprocess.o -g -c

./build/lexer.o: ./lexer.c
	gcc lexer.c ${INCLUDES} -o ./build/lexer.o -g -c

./build/token.o: ./token.c
	gcc token.c ${INCLUDES} -o ./build/token.o -g -c
	
./build/lex_process.o: ./lex_process.c
	gcc lex_process.c ${INCLUDES} -o ./build/lex_process.o -g -c

./build/helpers/buffer.o: ./helpers/buffer.c
	gcc helpers/buffer.c ${INCLUDES} -o ./build/helpers/buffer.o -g -c

./build/helpers/vector.o: ./helpers/vector.c
	gcc helpers/vector.c ${INCLUDES} -o ./build/helpers/vector.o -g -c

clean:
	rm -f ./main
	rm -rf ${OBJECTS}

.PHONY: all clean dirs