# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -pthread -g -std=c11 -Iinclude  # Dodano standard C11

# Plik wynikowy
TARGET = salon

# Ścieżki katalogów
SRC_DIR = src
INCLUDE_DIR = include
OBJ_DIR = obj

# Pliki źródłowe i obiektowe
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Informacje pomocnicze podczas budowy
$(info SRC = $(SRC))
$(info OBJ = $(OBJ))

# Domyślna reguła budowania
all: $(TARGET)

# Budowanie pliku wykonywalnego
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Reguła dla plików obiektowych
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE_DIR)/*.h
	mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Reguła czyszczenia plików tymczasowych
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
