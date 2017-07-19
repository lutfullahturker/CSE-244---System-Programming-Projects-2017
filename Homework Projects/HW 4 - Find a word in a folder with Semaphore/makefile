all: grepTh

grepTh: main.o
	gcc main.o -o grepTh -lpthread

main.o: main.c
	gcc -c -pedantic -errors -Wall main.c

clean:
	rm *.o grepTh
