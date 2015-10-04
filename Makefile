#
###############################################################################

CXX=clang++

SOURCES := fix_acc_test.cpp main.cpp TimeMeasure.cpp

CXXFLAGS += -std=c++11 -g -O3

###############################################################################

.PHONY: all
all: fix_acc_test.elf

.PHONY: run
run: fix_acc_test.elf
	./$<

fix_acc_test.elf: ${SOURCES:%.cpp=%.o}
	${CXX} -o $@ $^ ${LDFLAGS} ${LIBS}

*.o: Makefile

${SOURCES:%.cpp=%.o}: *.h

########################################

.PHONY: ci
ci:
	ci ${CXXFLAGS} ${CPPFLAGS} fix_acc_test.cpp

########################################

.PHONY: dis
dis: fix_acc_test.S

fix_acc_test.S: fix_acc_test.o
	objdump -dS $< > $@ 

########################################

.PHONY: clean
clean:
	rm -rf *.o *.elf *.bc *.ll *.S *.txt

###############################################################################
