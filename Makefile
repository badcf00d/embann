# SPDX-License-Identifier: GPL-2.0-only
#
#   Makefile - EMbedded Backpropogating Artificial Neural Network.
#   Copyright Peter Frost 2019

ifeq ($(OS),Windows_NT)
	CC=gcc
	CPP_CHECK=./tools/cppcheck/cppcheck.exe
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CC=gcc
		CPP_CHECK=./tools/cppcheck/cppcheck-linux
	endif
	ifeq ($(UNAME_S),Darwin)
		CC=gcc-9
		CPP_CHECK=./tools/cppcheck/cppcheck-mac
	endif
endif

EXE = embann

SRC_DIR = src
OBJ_DIR = obj
INC_DIRS = -Iinclude

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
LIBS = -lm


OPT_CFLAGS = -O3 -ffinite-math-only -fno-signed-zeros -march=native
CFLAGS = $(OPT_CFLAGS) -Wall -Wno-format -flto -fopenmp -fverbose-asm -fopt-info-all-optall=opt.log #-masm=intel -fopt-info-vec-missed -ffast-math -fdump-final-insns
GEN_PROFILE_CFLAGS = -fprofile-generate -fprofile-update=single
USE_PROFILE_CFLAGS = -fprofile-use
GEN_TREE_CFLAGS = -fdump-tree-optimized-graph
GRAPH_PDF_NAME = embann-graph.pdf

.PHONY: clean check debug generate-profile use-profile menuconfig all graph

all: $(EXE)



$(EXE): $(OBJ)
	$(CC) $^ $(INC_DIRS) $(LIBS) $(CFLAGS) -o $(EXE)
	$(info ### executable is located at ${EXE})

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(INC_DIRS) $(CFLAGS) --save-temps -c $< -o $@




debug: CFLAGS += -g -pg
debug: all

generate-profile: CFLAGS += $(GEN_PROFILE_CFLAGS) $(GEN_TREE_CFLAGS)
generate-profile: all
	

use-profile: CFLAGS += $(USE_PROFILE_CFLAGS) $(GEN_TREE_CFLAGS)
use-profile: GRAPH_PDF_NAME := after-profile.pdf
use-profile: | all graph



clean:
	rm -f ./$(OBJ_DIR)/* ./*.out ./*.s ./*.i ./*.res ./$(SRC_DIR)/*.c.dump \
	./$(SRC_DIR)/*.gcda ./$(EXE) ./$(EXE)-generate-profile ./$(EXE)-profiled \
	./opt.log ./$(SRC_DIR)/$(EXE) ./embann.ltrans0* ./embann.wpa* \
	./$(OBJ_DIR)/embann.c.*

clean-keep-profile:
	rm -f ./$(OBJ_DIR)/*.o ./*.out ./*.s ./*.i ./*.res ./$(SRC_DIR)/*.c.dump \
	./$(EXE) ./$(EXE)-generate-profile ./$(EXE)-profiled ./opt.log \
	./$(SRC_DIR)/$(EXE) ./embann.ltrans0* ./embann.wpa* \
	./$(OBJ_DIR)/embann.c.*

check:
	$(CPP_CHECK) --inline-suppr --max-configs=1 --addon=cert --addon=./cppcheck/addons/misra.json ./ -i./cppcheck -UARDUINO

check-all:
	$(CPP_CHECK) --inline-suppr --force --addon=cert --addon=./cppcheck/addons/misra.json ./ -i./cppcheck -UARDUINO

menuconfig:
	python ./tools/Kconfiglib/menuconfig.py ./Kconfig
	python ./tools/Kconfiglib/genconfig.py --header-path ./include/embann_config.h

graph: CFLAGS += $(GEN_TREE_CFLAGS)
graph: all
	dot -Tpdf embann.ltrans0.231t.optimized.dot -o $(GRAPH_PDF_NAME)