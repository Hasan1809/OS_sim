CC = gcc
CFLAGS = -Wall
DEPS = memory.h process.h
OBJ = main.o memory.o process.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

os_sim: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o os_sim
