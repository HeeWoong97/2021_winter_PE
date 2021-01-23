TOP_DIR = .
INC_DIR = $(TOP_DIR)/inc
SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = $(TOP_DIR)/build
CC=gcc
FLAGS = -pthread -fPIC -g -ggdb -Wall -I$(INC_DIR) 
OBJS = $(BUILD_DIR)/tree.o \
	$(BUILD_DIR)/utils.o \
	$(BUILD_DIR)/lockfree_utils.o \
	$(BUILD_DIR)/vector_arr.o

default: test
all: test test_parallel

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(FLAGS) -c -o $@ $<

test: $(OBJS)
	$(CC) $(FLAGS) $(SRC_DIR)/test.c -o test $(OBJS)

test_parallel: $(OBJS)
	$(CC) $(FLAGS) $(SRC_DIR)/test_parallel.c -o test_parallel $(OBJS)

clean:
	-rm -f $(BUILD_DIR)/*.o test test_parallel
