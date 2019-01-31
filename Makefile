SRCS:=$(wildcard *.c)
TARGETS:=$(SRCS:%.c=%)
CC:=gcc
all:$(TARGETS)
	@for i in $(TARGETS);do gcc -o $${i} $${i}.c;done
clean:
	rm -rf $(TARGETS)
