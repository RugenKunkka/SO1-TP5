CFLAGS = -Wall -Werror -pedantic 

main: main.o shell.o
	gcc $(CFLAGS) -o main main.o shell.o

main.o: main.c 
	gcc $(CFLAGS) -c main.c 

shell.o: shell.c shell.h
	gcc $(CFLAGS) -c shell.c

clean:
	rm -f *.o *.a main
    