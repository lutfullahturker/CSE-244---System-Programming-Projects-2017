all: listdir SearchWordAndWriteFile

listdir: main.o
	gcc main.o -o listdir

SearchWordAndWriteFile: SearchWordAndWriteFile.o
	gcc SearchWordAndWriteFile.o -o SearchWordAndWriteFile

main.o: main.c
	gcc -c -ansi -pedantic -errors -Wall main.c

SearchWordAndWriteFile.o: SearchWordAndWriteFile.c
	gcc -c -ansi -pedantic -errors -Wall SearchWordAndWriteFile.c

clean:
	rm *.o listdir SearchWordAndWriteFile
