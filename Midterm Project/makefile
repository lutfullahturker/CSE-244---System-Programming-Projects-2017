all: timeServer seeWhat showResult

timeServer: timeServer.o
	gcc -o timerServer timeServer.o -lm

timeServer.o: timeServer.c
	gcc -c timeServer.c

seeWhat: seeWhat.o
	gcc -o seeWhat seeWhat.o -lm

seeWhat.o: seeWhat.c
	gcc -c seeWhat.c

showResult: showResult.o
	gcc -o showResults showResult.o -lm

showResult.o: showResult.c
	gcc -c showResult.c
	
clean: 
	rm *.o showResults seeWhat timerServer
