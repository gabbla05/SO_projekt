# Kompilator i flagi kompilacji
CC = gcc
CFLAGS = -Wall -pthread -g -std=c11 -I.

# Pliki źródłowe
SRC_KIEROWNIK = kierownik.c utils.c
SRC_KLIENT = klient.c utils.c
SRC_FRYZJER = fryzjer.c utils.c

# Pliki obiektowe
OBJ_KIEROWNIK = $(SRC_KIEROWNIK:.c=.o)
OBJ_KLIENT = $(SRC_KLIENT:.c=.o)
OBJ_FRYZJER = $(SRC_FRYZJER:.c=.o)

# Nazwy programów
TARGET_KIEROWNIK = kierownik
TARGET_KLIENT = klient
TARGET_FRYZJER = fryzjer

# Reguła główna (tworzy wszystkie pliki wykonywalne)
all: $(TARGET_KIEROWNIK) $(TARGET_KLIENT) $(TARGET_FRYZJER)

# Kompilacja głównego procesu (kierownik)q

$(TARGET_KIEROWNIK): $(OBJ_KIEROWNIK)
	$(CC) $(CFLAGS) -o $@ $(OBJ_KIEROWNIK)

# Kompilacja procesu klienta
$(TARGET_KLIENT): $(OBJ_KLIENT)
	$(CC) $(CFLAGS) -o $@ $(OBJ_KLIENT)

# Kompilacja procesu fryzjera
$(TARGET_FRYZJER): $(OBJ_FRYZJER)
	$(CC) $(CFLAGS) -o $@ $(OBJ_FRYZJER)

# Reguła dla plików obiektowych
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Czyszczenie plików obiektowych i binarnych
clean:
	rm -f $(OBJ_KIEROWNIK) $(OBJ_KLIENT) $(OBJ_FRYZJER) $(TARGET_KIEROWNIK) $(TARGET_KLIENT) $(TARGET_FRYZJER)
