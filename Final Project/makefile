all:
	gcc -c *.c
	gcc -o clients mainClients.o -lpthread -lm
	gcc -o server main.o matrixOperations.o -lm -lpthread

clean:
	rm *.o server clients

