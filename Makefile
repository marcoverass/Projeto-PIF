CC = gcc
CFLAGS = -Wall -std=c11 -I./cli-lib/include
LDFLAGS = -lncurses

SRC = jogo.c \
      cli-lib/src/keyboard.c \
      cli-lib/src/screen.c \
      cli-lib/src/timer.c

TARGET = air_hockey

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

