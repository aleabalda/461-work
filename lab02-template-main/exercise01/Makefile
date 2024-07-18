CC=gcc
EXECUTABLES=gen_numbers.out select_arg.out gen_in_range.out
SOURCES=gen_numbers.c gen_in_range.c select_arg.c
HEADERS=utils.h

.PHONY: all clean test

UNAME_S:= $(shell uname -s)
ifeq ($(UNAME_S),Linux)
all: $(EXECUTABLES)
else
all:
	@echo "Sorry, I prefer Linux"
endif

gen_numbers.out: gen_numbers.c utils.c $(headers)
	$(cc) -o $@ $^

select_arg.out: select_arg.c utils.c $(headers)
	$(cc) -o $@ $^

gen_in_range.out: gen_in_range.c
	$(cc) -o $@ $<

clean:
	rm -f $(EXECUTABLES)

test: $(EXECUTABLES)
	./gen_numbers.out 10
	./select_arg.out arg1 arg2 arg3
	./gen_in_range.out 11 29
