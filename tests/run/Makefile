MICRO_F2B := ../../mf2b
TESTS := $(wildcard *.test)

all: ${TESTS}

${TESTS}:
	MF2B=${MICRO_F2B} ./$@ || echo -e "\nERROR: test $@ failed!\n"

.PHONY: ${TESTS}
