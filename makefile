# Makefile for building small shell
# by Kyle Huang

# edit to add exta c files
TARGETS=smallsh

CROSS_TOOL = 
CC_C = $(CROSS_TOOL)gcc

# -Werror
C_FLAGS = -Wall -g -std=c99 

all: clean $(TARGETS)

$(TARGETS):
	$(CC_C) $(C_FLAGS) $@.c -o $@

clean:
	rm -f $(TARGETS)
