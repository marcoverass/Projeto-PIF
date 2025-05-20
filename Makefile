# Compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -std=c11 -I./cli-lib/include

# Arquivos da biblioteca cli-lib
SRC_LIB = $(wildcard ./cli-lib/src/*.c)

# Nome do executável
EXEC = air_hockey

# Alvo principal
all: $(EXEC)

# Regra de compilação
$(EXEC): jogo.c
	$(CC) $(CFLAGS) jogo.c $(SRC_LIB) -o $(EXEC)

# Alvo para limpar arquivos gerados
clean:
	rm -f $(EXEC)