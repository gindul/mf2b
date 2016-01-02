CC := gcc
CFLAGS := -g -DMAX_SUBS=128
SRC := $(wildcard *.c)
BIN := $(patsubst %.c,%,${SRC})

all: ${BIN}
	for unit in $^; do \
		./$$unit || echo -e "\nERROR: unit $$unit failed!\n"; \
	done
