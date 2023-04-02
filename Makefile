# Makefile
# Build memkv
# By J. Stuart McMurray
# Created 20230401
# Last Modified 20230402

.PHONY: test

CFLAGS=-O2 -Wall --pedantic -Wextra -static
BUILD=cc ${CFLAGS}  -o $@ $>
CLIENT=memkv
SERVER=memkvd

DEFAULT: ${CLIENT} ${SERVER}

*.c: *.h

${SERVER}: common.o memkvd.o handle.o tree.o
	${BUILD}

${CLIENT}: common.o memkv.o
	${BUILD} -lpthread

clean:
	rm -f *.o ${CLIENT} ${SERVER}
