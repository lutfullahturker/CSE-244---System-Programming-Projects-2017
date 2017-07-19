/* 
 * File:   Hw2Main.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 04 March 2017, 23:34
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <sys/wait.h>

/* Recursive ile klasörün içindeki tüm dosyaları kontrol ettiğimiz ana fonksiyon */
void readFolder(const char* argv1,char* argv2,char* exeDir);
/* Program sonunda tüm txt lerde bulunan toplam sayıları toplayıp log dosyasının
 * sonuna yazan fonksiyonumuz.
 */
void writeSumOfFound(const char* exeDir,const char* argv1);

int main(int argc, char** argv) {
    char exeDir[PATH_MAX+1];
    char exeFolder[PATH_MAX+1];
    char forRemoveLog[PATH_MAX+1];
    getcwd(exeDir,PATH_MAX + 1);
    strcpy(exeFolder,exeDir) ;
    strcpy(forRemoveLog,exeDir);
    strcat(forRemoveLog,"/log.log");
    remove(forRemoveLog);
    /* Program başında daha önceden log dosyası oluşturulmuşsa onu siliyoruz.
     * Ve exemizin çalıştırıldığı path bize lazım olduğu için chdir yapmadan önce
     * onu bir değişkende tutuyoruz.
     */
    if (argc != 3)
    {
        fprintf(stderr, "Wrong number of argument !\nUsage : Please enter with command line parameters like ./exe SearchWord DirPath\n");
        exit (1);
    }    
    if (argv[2][0] == '.' && strlen(argv[2]) == 1){
        strcpy(argv[2],exeDir);
        strcat(argv[2],"\0");
    }
    readFolder(argv[1],argv[2],exeDir);    
    while(wait(NULL) != -1);
    writeSumOfFound(exeFolder,argv[1]);
    
    return 0 ;
}

void readFolder(const char* argv1,char* argv2,char* exeDir)
{
    DIR *folder ;
    struct dirent *direntp;
    int child = 5;
    struct stat bufStat;
    char curDir[PATH_MAX+1];
    char temp[PATH_MAX+1];
    char exePath[PATH_MAX+1];
    strcpy(temp,argv2);
    strcpy(exePath,exeDir);
    strcat(exeDir,"/SearchWordAndWriteFile");
    
    if ((folder = opendir(argv2)) == NULL) {
        perror ("Cannot Open Directory ") ;
        exit (1);
    }
    strcpy(curDir,argv2); 
    
    
   
    while ((direntp = readdir (folder)) != NULL)
    {
        /* 
         * UNIX 'de Her klasörün içinde . ve .. isimli iki görünmeyen dosya vardır.
         * readdir fonksiyonu bu dosyaları da okuduğu için bu görünmeyen dosyalarla
         * işlem yapmamak için kontrol ediyoruz.
         */
        if (direntp->d_name[0] != '.')
        {
            if (chdir(curDir) == -1){
                perror("Change Directory Failed ");
                exit(1);
            }
            
            stat(direntp->d_name,&bufStat);
            if (S_ISDIR(bufStat.st_mode))
            {
                strcat(temp,"/");
                strcat(temp,direntp->d_name);
                child = fork();
                if(child == -1)
                {
                    perror("Fork Failed ");
                    exit (1);
                }
                if(child == 0)
                {   /*Klasör bulunca recursive ile o klasörü de arıyoruz*/
                    readFolder(argv1,temp,exePath);
                    exit(1);
                }
                strcpy(temp,argv2); 
            }
                
            else
            {
                child = fork();
                if(child == -1)
                {
                    perror("Fork Failed ");
                    exit (1);
                }
                if(child == 0)
                {  /* Dosya okuma işlemini child a yaptırıyoruz.Diğer programda okuyup log a yazıyor.*/
                   execl(exeDir,"SearchWordAndWriteFile",argv1,direntp->d_name,exePath,NULL);
                   perror("Child failed to execute the program ");
                   exit (1);
                }
            }
         }
    }
while(wait(NULL) != -1);

closedir(folder);    
}

void writeSumOfFound(const char* exeDir,const char* argv1)
{
    FILE *countLog,*logLog;
    int i,sum = 0,temp=0;
    char path[PATH_MAX+1];
    strcpy(path,exeDir);
    strcat(path,"/count.log");
    countLog = fopen(path,"a+");
    if (countLog == NULL)
    {
        perror("count.log file open failed ");
        exit (1);
    }
    for (i = 0;fscanf(countLog,"%d",&temp) > 0;++i)
        sum += temp;
    fclose(countLog);
    if( remove(path) != 0 )
    	perror( "Error deleting count.log file" );
    strcpy(path,exeDir);
    strcat(path,"/log.log");
    logLog = fopen(path,"a+");
    fprintf(logLog,"\nTotal %d '%s' found in %d files.",sum,argv1,i);
    fclose(logLog);
}
