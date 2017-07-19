/* 
 * File:   main.c
 * Author: Lütfullah TÜRKER 141044050
 *
 * Created on 27 February 2017, 12:22
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void searchWordInFile (FILE* file,char* word);


/*
 * 
 */
int main(int argc, char** argv) {
    FILE *file;
    if (argc != 3)
    {
        printf("Please enter command line parameters !");
        exit (0);
    }
    file=fopen(argv[2],"r");
    if (file == NULL) 
    {
        perror ("File Open Failed !");
        exit(0);
    } 
    
    searchWordInFile (file,argv[1]);
    fclose(file);
    
    return 0;
}


void searchWordInFile (FILE* file,char* search)
{
    int i = 0;
    char readChar ;
    long int positionInFile;
    int row = 1;
    int count = 0;
    int col = 1;
    long int temp ;
    int searchedRow,searchedCol;
       
    do {
        readChar = getc (file);
        if (readChar == '\n' )
        {
            ++row;
            col = 1;
        }
        else if (readChar == ' ' || readChar == '\t')
            ++col;
        else if (search[i] == readChar || search[0] == readChar)
        {
            if (i == 0)
            {
                searchedRow = row;
                searchedCol = col;
                positionInFile = ftell(file);
            }
            if (i != 0  && search[0] == readChar)
            {
                temp = ftell(file);
                fscanf(file,"%c",&readChar) ;
                while ( (readChar == '\n' || readChar == ' ' || readChar == '\t') && !feof(file) )
                    fscanf(file,"%c",&readChar) ;
                if (i+1 < strlen(search) && (readChar != search[i+1] || readChar == search[0]))
                {
                    i = 0;
                    searchedRow = row;
                    searchedCol = col;
                    positionInFile = temp;
                }
                fseek(file,temp,SEEK_SET);
            }
            ++col;
            if (i < (strlen(search)-1))
                ++i ;
            else 
            {
                printf("'%s' Found at %d. Row %d. Coloumn .\n",search,searchedRow,searchedCol);
                count ++;
                col = searchedCol +1;
                if (row != searchedRow)
                    row = searchedRow;
                fseek(file,positionInFile,SEEK_SET);
                
                i = 0;
            }
            
        }
        else 
        {
            i = 0 ;
            ++col;
        }
            
    } while (readChar != EOF);
       
    printf("Total %d '%s' Were Found.\n",count,search);
}