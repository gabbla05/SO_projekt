
CC = g++
CFLAGS = -Wall -pthread -g -Iinclude


TARGET = salon


SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(INCLUDE_DIR)/*.h)
OBJ = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)


all: $(TARGET)


$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^


$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJ_DIR) $(TARGET)
