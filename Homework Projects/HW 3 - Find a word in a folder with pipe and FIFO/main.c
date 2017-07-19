/* 
 * File:   main.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 18 March 2017, 12:00
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
#include <fcntl.h>

#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define MAX_LINE_LENGTH 1024
/*
 *  Recursive ile klasörün içindeki tüm dosyaları kontrol ettiğimiz ana fonksiyon 
 */
void readFolder(const char* argv1,char* argv2,char* exeDir,int fifoDW,int fifoCount);
/*
 * readPipe
 * her dosya için pipe dan gelen stringleri log dosyasına yazıp toplam countu da
 * return eder.
 */
int readPipe(const char* exeDir);
/*
 * readFifo
 * Program boyunca bir proses tarafından çalışır.Ana klasörün fifosuna countların
 * yazılmasını bekler.Her gelen countu toplar. hepsini topladıktan sonra program
 * biterken log dosyasının sonuna yazar.
 */
void readFifo(int fifoDR,const char* exePath,const char* argv1);
/*
 *  gelen =_WRONLY modunda açılmış fifo dosyasına count u yazar.alt klasörler içindir.
 */
void writeFifo(int fifoDW,int count);
/*
 * Alt klasörün fifosundan üst klasörün fifosuna countu aktarır.
 */
void writeFifoToFifo(int fifoDW,int subFifo);


int main(int argc, char** argv) {
    char exeDir[PATH_MAX+1];
    char exeFolder[PATH_MAX+1];
    char forRemoveLog[PATH_MAX+1];
    int fifoDR,fifoDW,fifoCount = 1;
    pid_t child;
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
    if (mkfifo("myfifo.fifo", FIFO_PERMS) == -1){
        perror("Failed to create myfifo.fifo");
        return (1);
    }
    child = fork();
    if (child == -1)
    {
        perror("Failed to fork");
        return 1;
    }
    else if (child == 0)
    {
        if ((fifoDW = open("myfifo.fifo", O_WRONLY)) < 0) {
            perror("fifo open failed ");
            exit(1);
        }
        readFolder(argv[1],argv[2],exeDir,fifoDW,fifoCount);   
		close(fifoDW);
        exit(1);
    }
    if ((fifoDR = open("myfifo.fifo", O_RDONLY)) < 0) {
        perror("fifo open failed ");
        exit(1);
    }
    readFifo(fifoDR,exeFolder,argv[1]);
    
    
    while(wait(NULL) != -1);
    
    close(fifoDR);
    strcat(exeFolder,"/myfifo.fifo");
    if (unlink(exeFolder) == -1)
        perror("Failed to remove myfifo.fifo ");
    return 0 ;
}

void readFolder(const char* argv1,char* argv2,char* exeDir,int fifoDW,int fifoCount)
{
    DIR *folder ;
    struct dirent *direntp;
    pid_t child ;
    struct stat bufStat;
    char curDir[PATH_MAX+1],fifoName[PATH_MAX+1];
    char temp[PATH_MAX+1];
    char exePath[PATH_MAX+1];
    int fileDes[2],count = 0,subFifoW,subFifoR;
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
                sprintf(fifoName,"%s%s%d",exePath,"/subFifo",fifoCount);
                if (mkfifo(fifoName, FIFO_PERMS) == -1){
                    perror("Failed to create subFifo");
                    exit (1);
                }
                
                child = fork();
                if(child == -1)
                {
                    perror("Fork Failed ");
                    exit (1);
                }
                
                if(child == 0)
                {   /*Klasör bulunca recursive ile o klasörü de arıyoruz ve
                     klasördeki toplam bulunan sayısını fifoya yazıyoruz. */
                    if ((subFifoW = open(fifoName, O_WRONLY)) < 0) {
                        perror("subFifoWrite open failed ");
                        exit(1);
                    }   
                    readFolder(argv1,temp,exePath,subFifoW,++fifoCount);
                    close(subFifoW);
                    exit(1);
                }
                else
                {   /* aynı anda alt klasörden gelen count u üst klasörün
                     *  fifosuna yazıyoruz.*/
                    if ((subFifoR = open(fifoName, O_RDONLY)) < 0) {
                        perror("subFifoRead open failed ");
                        exit(1);
                    }  
                    writeFifoToFifo(fifoDW,subFifoR);
                    close(subFifoR);
                    if (unlink(fifoName) == -1)
                        perror("Failed to remove subFifo ");
                }
                strcpy(temp,argv2); 
            }
            else
            {
                /* Dosya bulununca pipe oluşturuyoruz. */
                if (pipe(fileDes) == -1) 
                {
                    perror("pipe error ");
                    exit(1);
                }
                child = fork();
                if(child == -1)
                {
                    perror("Fork Failed ");
                    exit (1);
                }
                if(child == 0)
                {  /* Pipe a yazma işlemini child a yaptırıyoruz.
                    * Diğer programda bulup pipe a yazıyor.*/
                    if (dup2(fileDes[1],1) == -1) {
                        perror("dup2 error ");
                        exit(1);
                    }
                    close(fileDes[0]);
                    close(fileDes[1]);
                    if (execl(exeDir,"SearchWordAndWriteFile",argv1,direntp->d_name,exePath,NULL) == -1) {
                        perror("Child failed to execute the program ");
                        exit (1);
                    }
                }
                else
                {   /* çocuktan gelen dosya içi bilgileri pipe dan okuyup 
                     * gereklileri log dosyasına yazıyoruz. readPipe ile.
                     * Ve readPipe fonksiyonu dosyada bulunan kelime sayısını
                     * return ettiği için klasördeki toplam countu bulmak için
                     * her dosyadan gelen countu topluyoruz.Fonksiyonun sonunda da
                     * klasördeki toplamı fifoya yazıyoruz. */
                    if (dup2(fileDes[0],0) == -1) {
                        perror("dup2 error ");
                        exit(1);
                    }
                    close(fileDes[0]);
                    close(fileDes[1]);
                    count += readPipe(exePath);
                }
            }
         }
    }
    while(wait(NULL) != -1);
    writeFifo(fifoDW,count);
    closedir(folder);    
}

int readPipe(const char* exeDir)
{
    FILE *log;
    char logPath[PATH_MAX+1];
    char str[MAX_LINE_LENGTH];
    int count = 0;
    sprintf(logPath,"%s/log.log",exeDir);
    if ((log = fopen(logPath,"a+")) ==NULL){
        perror ("log.log Open Failed ");
        exit(1);
    }
    while (fgets(str,MAX_LINE_LENGTH,stdin) != NULL )
    {
        if (strncmp(str,"Total",5) == 0)
            sscanf(str,"%*s %d",&count);
        else
            fprintf(log,"%s",str);
    }
    fclose(log);
    return count;
}

void readFifo(int fifoDR,const char* exePath,const char* argv1)
{
    int result,total=0,temp;
    char log[PATH_MAX+1];
    FILE *logFile;
    while ((result = read(fifoDR, &temp, sizeof(int))) > 0){
        total += temp;
    }
    if (result < 0){
        perror("readFifo Error ");
        exit(1);
    }
    sprintf(log,"%s/%s",exePath,"log.log");
    if ((logFile = fopen(log,"a+")) == NULL){
        perror("log.log open failed in readFifo ");
        exit (1);
    }
    fprintf(logFile,"\n\n%d '%s' were found in total.",total,argv1);
    fclose(logFile);
}

void writeFifo(int fifoDW,int count)
{
    write (fifoDW,&count,sizeof(int));
}

void writeFifoToFifo(int fifoDW,int subFifo)
{
    /* alt klasörün fifo sundan geleni üst klasörün fifosuna yazıyor. */
    int result,temp;
    while ((result = read(subFifo, &temp, sizeof(int))) > 0){
        if (write(fifoDW,&temp,sizeof(int))<0)
        {
            perror("write Fifo from subFifo Error ");
            exit(1);
        }
    }
    if (result < 0){
        perror("SubFifo read Error ");
        exit(1);
    }
}
