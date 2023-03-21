CC      = gcc
RM      = rm -f

run: main.c
	$(CC) -o program main.c heap.c
clean:
	$(RM) -f main 