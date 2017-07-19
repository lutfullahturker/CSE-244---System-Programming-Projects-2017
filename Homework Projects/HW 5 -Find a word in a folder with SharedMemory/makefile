all: grepSh

grepSh: main.o
	gcc main.o -o grepSh -lpthread

main.o: main.c
	gcc -c main.c

clean:
	rm *.o grepSh
