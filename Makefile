CC = gcc
CFLAGS = -Wall -pthread -g -Iinclude
TARGET = salon

SRC = src/main.c src/fryzjer.c src/klient.c src/kierownik.c src/utils.c
HEADERS = include/fryzjer.h include/klient.h include/kierownik.h include/utils.h

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)
