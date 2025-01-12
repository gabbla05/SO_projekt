CC = gcc
CFLAGS = -Wall -pthread -g -Iinclude  # Dodano katalog include

TARGET = salon

SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj

SRC = $(wildcard $(SRC_DIR)/*.c)  # Ścieżki źródłowe
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

$(info SRC = $(SRC))  # Debugowanie: sprawdź, co jest w zmiennej SRC

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/*.h
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
