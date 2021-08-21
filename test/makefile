EXEC = test.out
FLAGS = -Wall -Werror
OBJ = main.o test.o
CFLAGS = -O3
CC = gcc
VECTORPATH = ../src/vector.c


all: $(EXEC)
	./$(EXEC)

$(EXEC): $(OBJ) vector.o
	$(CC) $(CFLAGS) -o $@ $^ $(FLAGS)

vector.o: 
	$(CC) $(CFLAGS) -o $@ -c $(VECTORPATH) $(FLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $< $(FLAGS)

rmproper:
	rm -f $(OBJ) $(EXEC)
