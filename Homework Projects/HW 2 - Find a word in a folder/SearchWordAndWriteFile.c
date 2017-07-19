/* 
 * File:   main.c
 * Author: Lütfullah TÜRKER 141044050
 *
 * Created on 27 February 2017, 12:22
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/limits.h>

void searchWordInFile (FILE* file,const char* search,char* argv2,const char* curWD,const char* exeDir);

int main(int argc, char** argv) {
    FILE *file;
    char currentWD[PATH_MAX];
    if (argc != 4)
    {
        fprintf(stderr, "Wrong number of argument !\nPlease enter with command line parameters like ./exe seller test.txt !\n");
        exit (1);
    }
    getcwd(currentWD,PATH_MAX);
    file=fopen(argv[2],"r");
    if (file == NULL) 
    {
        perror ("File Open Failed !");
        exit(1);
    } 
    searchWordInFile (file,argv[1],argv[2],currentWD,argv[3]);
    fclose(file);
    
    return 0;
}


void searchWordInFile (FILE* file,const char* search,char* argv2,const char* curWD,const char* exeDir)
{
    int i = 0;
    char readChar ;
    long int positionInFile,temp;
    FILE *logFile, *countFile;
    int row = 1;
    int count = 0;
    int col = 1;
    char fileTmp[PATH_MAX];
    int searchedRow,searchedCol;
    strcpy(fileTmp,exeDir);
    strcat(fileTmp,"/log.log");
    logFile = fopen(fileTmp,"a+");
    strcpy(fileTmp,exeDir);
    strcat(fileTmp,"/count.log");
    countFile = fopen(fileTmp,"a+");

    if (countFile == NULL)
    {
        perror ("Total Count File Open Failed !");
        exit(1);
    } 
    if (logFile == NULL)
    {
        perror ("Log File Open Failed !");
        exit(1);
    } 

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
                count ++;
                fprintf(logFile,"%s : [%d,%d] '%s' first character is found.\n",argv2,searchedRow,searchedCol,search);
				fflush(logFile);
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
       
    fprintf(countFile,"%d ",count);
    fclose(countFile);
}
