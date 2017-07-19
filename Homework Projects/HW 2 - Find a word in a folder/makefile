all: exe

exe: main.o
	gcc main.o -o exe

main.o: main.c
	gcc -c -ansi -pedantic -errors -Wall main.c

clean:
	rm *.o exe
