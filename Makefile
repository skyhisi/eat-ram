# eat-ram Makefile

DEFINES:=-D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=600
WARNINGS:=-W -Wall -Werror -pedantic -pedantic-errors
CFLAGS:=-std=c11 -O2 $(CFLAGS)

COSMOCC:=~/cosmocc/bin/cosmocc

.PHONY:
all: eat-ram eat-ram.com

eat-ram: eat-ram.c
	$(CC) $(CFLAGS) $(DEFINES) $(WARNINGS) $^ -o $@

eat-ram.com : eat-ram.c
	$(COSMOCC) $(CFLAGS) $(DEFINES) $(WARNINGS) $^ -o $@

.PHONY:
clean:
	rm -f eat-ram eat-ram.com eat-ram.com.dbg eat-ram.aarch64.elf

