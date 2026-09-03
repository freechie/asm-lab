# Assembly, CPU, and Performance Lab
#
#   make list                 show the lessons
#   make run LESSON=06        build and run every program in lesson 06
#   make asm LESSON=02        dump cleaned assembly for lesson 02
#   make asm LESSON=02 DIFF=1 dump -O0 next to -O2
#   make x86 LESSON=01        same source compiled for x86-64
#   make all                  run every lesson start to finish
#   make clean
#
# Knobs: OPT=-O0 ARCH=x86_64 FUNC=name

CC      := clang
CFLAGS  := -Wall -Wextra -std=gnu17 -I$(CURDIR)/tools
LDLIBS  := -lm

# Which interpreter the Python lessons measure. Shell functions and pyenv
# shims are invisible to make, so an interactive `python3` and make's
# `python3` can be different builds. Override explicitly to compare:
#   make run LESSON=01 PYTHON=$(pyenv which python3)
PYTHON  ?= python3
OPT     ?= -O2
ARCH    ?= arm64
DIFF    ?= 0
FUNC    ?=
BUILD   := build

LESSONS := $(sort $(patsubst %/,%,$(dir $(wildcard [0-9][0-9]-*/))))

ifdef LESSON
  DIR := $(firstword $(filter $(LESSON)-%,$(LESSONS)))
endif

.PHONY: help list all run asm x86 clean check-dir

help:
	@sed -n '3,12p' $(MAKEFILE_LIST) | sed 's/^# \{0,1\}//'

list:
	@echo "lessons:"
	@for d in $(LESSONS); do \
	   printf "  %-22s %s\n" "$${d%%-*}" "$$d"; \
	 done
	@echo
	@echo "run one with:  make run LESSON=01"

check-dir:
	@test -n "$(DIR)" || { \
	   echo "error: set LESSON=NN (e.g. LESSON=01). Run 'make list'."; exit 1; }

# Only files containing a main() are executables; the rest exist to be read as
# assembly, so building them would fail on a missing entry point.
run: check-dir
	@mkdir -p $(BUILD)
	@ran=0; \
	 for f in $(DIR)/*.c; do \
	   [ -f "$$f" ] || continue; \
	   grep -q 'int main' "$$f" || continue; \
	   b=$(BUILD)/$$(basename $$f .c); \
	   $(CC) $(CFLAGS) $(OPT) -arch $(ARCH) "$$f" $(wildcard $(DIR)/*.S) \
	     -o "$$b" $(LDLIBS) || exit 1; \
	   "./$$b" || exit 1; \
	   ran=1; \
	 done; \
	 if [ $$ran -eq 0 ]; then \
	   echo "$(DIR) has no runnable programs; try: make asm LESSON=$(LESSON)"; \
	 fi
	@for p in $(DIR)/*.py; do \
	   [ -f "$$p" ] || continue; \
	   $(PYTHON) "$$p" || exit 1; \
	 done

asm: check-dir
	@for f in $(DIR)/*.c; do \
	   [ -f "$$f" ] || continue; \
	   if [ "$(DIFF)" = "1" ]; then \
	     tools/asmdump.sh -d -a $(ARCH) $(if $(FUNC),-f $(FUNC),) "$$f"; \
	   else \
	     tools/asmdump.sh -a $(ARCH) -O $(patsubst -O%,%,$(OPT)) \
	       $(if $(FUNC),-f $(FUNC),) "$$f"; \
	   fi; \
	   echo; \
	 done

x86:
	@$(MAKE) --no-print-directory asm LESSON=$(LESSON) ARCH=x86_64 FUNC=$(FUNC)

all:
	@for d in $(LESSONS); do \
	   echo; echo "########## $$d ##########"; \
	   $(MAKE) --no-print-directory run LESSON=$${d%%-*} || exit 1; \
	 done

clean:
	@rm -rf $(BUILD)
	@echo "cleaned"
