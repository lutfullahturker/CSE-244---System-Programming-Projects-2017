/* 
 * File:   main.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 09 May 2017, 20:50
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
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>

typedef struct {
    char argv1[PATH_MAX+1],dName[PATH_MAX+1],exePath[PATH_MAX+1];
}forExec;

sem_t semLock;
struct timeval tStart;
int *shmCount,msgpid;
char findWord[50];
char pathOfExeFolder[PATH_MAX+1];
struct sigaction sigHand;
static  volatile sig_atomic_t sigFlag = 0;

/* Recursive ile klasörün içindeki tüm dosyaları kontrol ettiğimiz ana fonksiyon */
void readFolder(const char* argv1,char* argv2,char* exeDir);
/* Program sonunda tüm txt lerde bulunan toplam sayıları toplayıp log dosyasının
 * sonuna yazan fonksiyonumuz.
 */
void writeSumOfFound(const char* exeDir,const char* argv1,int exitCond);
void* threadsFunc(void *arg);
void searchWordInFile (FILE* file,const char* search,char* argv2,const char* exeDir);

/*
 * global olan iki stringi de sinyal handler fonksiyonu için global yaptım.
 * Diğer fonksiyonların hepsinde bu global değişkenleri parametre olarak alıyordum.
 * sinyali en son yaptığım ve önceki ödevlerde de böyle olduğu için parametreleri
 * silmek istemedim.(madem global yapmış niye parametre alıyor fonksiyonlar diye düşünmeyin :) )
 */
static void sigHandler (int sigNo, siginfo_t *siginfo, void *context){
       /* printf("girdim \n");*/
    sigFlag = 2;
       /* exit(1);*/
}

int main(int argc, char** argv) {
    char exeDir[PATH_MAX+1];
    char exeFolder[PATH_MAX+1];
    char forRemoveLog[PATH_MAX+1],tmp[128];
    
    if (gettimeofday(&tStart, NULL)) {
        fprintf(stderr, "Failed to get start time\n");
        return 1;
   }
    getcwd(exeDir,PATH_MAX + 1);
    strcpy(pathOfExeFolder,exeDir);
    strcpy(exeFolder,exeDir) ;
    strcpy(forRemoveLog,exeDir);
    strcat(forRemoveLog,"/log.txt");
    remove(forRemoveLog);
    /* Program başında daha önceden log dosyası oluşturulmuşsa onu siliyoruz.
     * Ve exemizin çalıştırıldığı path bize lazım olduğu için chdir yapmadan önce
     * onu bir değişkende tutuyoruz.
     */
    if (argc != 3)
    {
        fprintf(stderr, "Wrong number of argument !\nUsage : Please enter with command line parameters like ./grepSh Word DirName\n");
        exit (1);
    }    
    strcpy(findWord,argv[1]);
    sigHand.sa_sigaction = &sigHandler;
    sigHand.sa_flags = SA_SIGINFO;
    if ((sigemptyset(&sigHand.sa_mask) == -1) || (sigaction(SIGINT, &sigHand, NULL) == -1) || (sigaction(SIGUSR1, &sigHand, NULL) == -1) || (sigaction(SIGUSR2, &sigHand, NULL) == -1)
            || (sigaction(SIGHUP, &sigHand, NULL) == -1) || (sigaction(SIGSEGV, &sigHand, NULL) == -1) || (sigaction(SIGALRM, &sigHand, NULL) == -1) || (sigaction(SIGTERM, &sigHand, NULL) == -1)
            || (sigaction(SIGILL, &sigHand, NULL) == -1) || (sigaction(SIGABRT, &sigHand, NULL) == -1)) 
        perror("Failed to set signal handler");
    
    strcpy(tmp,argv[2]);
    sprintf(argv[2],"%s/%s",exeDir,tmp);
    readFolder(argv[1],argv[2],exeDir);
    
    while(wait(NULL) != -1);
    if (sigFlag == 0)
        writeSumOfFound(exeFolder,argv[1],1);
    else if (sigFlag == 2)
        writeSumOfFound(exeFolder,argv[1],2);
    return 0 ;
}

void* threadsFunc(void *arg)
{
    FILE* file;
    file=fopen((*(forExec*)arg).dName,"r");
    if (file == NULL) 
    {
        perror ("File Open Failed !");
        writeSumOfFound((*(forExec*)arg).exePath,(*(forExec*)arg).argv1,3);
        exit(1);
    } 
    searchWordInFile (file,(*(forExec*)arg).argv1,(*(forExec*)arg).dName,(*(forExec*)arg).exePath);
    fclose(file);
    if (sem_post(&semLock) == -1){
        fprintf(stderr, "Thread failed to unlock semaphore\n");
        writeSumOfFound((*(forExec*)arg).exePath,(*(forExec*)arg).argv1,3);
        exit(1);
    }
    return NULL;
}


void readFolder(const char* argv1,char* argv2,char* exeDir)
{
    DIR *folder ;
    FILE *dirTempFile,*dirThreadTemp;
    struct dirent *direntp;
    int child = 5,i=0,j;
    struct stat bufStat;
    char curDir[PATH_MAX+1];
    char temp[PATH_MAX+1];
    char exePath[PATH_MAX+1];
    forExec args;
    pthread_t threadArr[200];
    int shmid,msgid,cameFromSubDir = -1;
    strcpy(exePath,exeDir);
    strcat(exeDir,"/SearchWordAndWriteFile");
    strcpy(temp,exePath);
    strcat(temp,"/dirTemp");
    if ((dirTempFile = fopen(temp,"a+")) == NULL){
        perror("directoryCount temp file open failed ");
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    fprintf(dirTempFile,"%d ",1);
    fclose(dirTempFile);
    strcpy(temp,argv2);
    if ((msgid = msgget((key_t)getpid(),IPC_CREAT | 0666 )) == -1)
    {
        perror("msgget failed ");
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    if ((shmid = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) < 0) {
        perror("shmget failed ");
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    if ((shmCount = shmat(shmid, NULL, 0)) == (int *) -1) {
        perror("shmat failed ");
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    if ((folder = opendir(argv2)) == NULL) {
        perror ("Cannot Open Directory ") ;
        writeSumOfFound(exePath,argv1,3);
        exit (1);
    }
    strcpy(curDir,argv2); 
    if (sem_init(&semLock, 0, 1) == -1) {
        perror("Failed to initialize semaphore");
        closedir(folder);
        writeSumOfFound(exePath,argv1,3);
        exit(1) ;
    }
    *shmCount = 0;
    while ((direntp = readdir (folder)) != NULL && (sigFlag == 0 || sigFlag == 1))
    {
        /* 
         * UNIX 'de Her klasörün içinde . ve .. isimli iki görünmeyen dosya vardır.
         * readdir fonksiyonu bu dosyaları da okuduğu için bu görünmeyen dosyalarla
         * işlem yapmamak için kontrol ediyoruz.
         */
        if (direntp->d_name[0] != '.' && direntp->d_name[strlen(direntp->d_name) -1] != '~')
        {
            if (chdir(curDir) == -1){
                perror("Change Directory Failed ");
                closedir(folder);
                sem_destroy(&semLock);
                writeSumOfFound(exePath,argv1,3);
                exit(1);
            }
            
            stat(direntp->d_name,&bufStat);
            if (S_ISDIR(bufStat.st_mode))
            {
                strcat(temp,"/");
                strcat(temp,direntp->d_name);
                for (j=0;j<i;j++){
                    if (pthread_join(threadArr[j],NULL) == -1){
                        perror("thread join failed");
                        closedir(folder);
                        sem_destroy(&semLock);
                        writeSumOfFound(exePath,argv1,3);
                        exit(1);
                    }
                }
                child = fork();
                if(child == -1)
                {
                    perror("Fork Failed ");
                    closedir(folder);
                    sem_destroy(&semLock);
                    writeSumOfFound(exePath,argv1,3);
                    exit (1);
                }
                if(child == 0)
                {   /*Klasör bulunca recursive ile o klasörü de arıyoruz*/
                    sigFlag = 1;
                    closedir(folder);
                    readFolder(argv1,temp,exePath);
                    exit(1);
                }
                strcpy(temp,argv2); 
            }
            else
            {
                /* Dosya okuma işlemini thread e yaptırıyoruz.Diğer programda okuyup log a yazıyor.*/
                if (sem_wait(&semLock) == -1){
                    fprintf(stderr,"sem_wait Failed ");
                    closedir(folder);
                    sem_destroy(&semLock);
                    writeSumOfFound(exePath,argv1,3);
                    exit(1);
                }
                strcpy(args.argv1,argv1);
                strcpy(args.dName,direntp->d_name);
                strcpy(args.exePath,exePath);
                
                if (pthread_create(&threadArr[i], NULL, threadsFunc, &args)) {
                    perror("Failed to create thread ");
                    closedir(folder);
                    sem_destroy(&semLock);
                    writeSumOfFound(exePath,argv1,3);
                    exit(1);
                }
                ++i;
            }
         }
    }
    while(wait(NULL) != -1);
    for (j=0;j<i;j++){
        if (pthread_join(threadArr[j],NULL) == -1){
            perror("thread join failed");
            closedir(folder);
            sem_destroy(&semLock);
            writeSumOfFound(exePath,argv1,3);
            exit(1);
        }
    }
    strcpy(temp,exePath);
    strcat(temp,"/dirThreadTemp");
    if ((dirThreadTemp = fopen(temp,"a+")) == NULL){
        perror("dirThreadTemp file open failed ");
        closedir(folder);
        sem_destroy(&semLock);
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    fprintf(dirThreadTemp,"%s\n%d\n",argv2,i);
    fclose(dirThreadTemp);
    if ((msgpid = msgget((key_t)getppid(),IPC_CREAT | 0666 )) == -1)
    {
        perror("msgget failed ");
        closedir(folder);
        sem_destroy(&semLock);
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    while (msgrcv(msgid,&cameFromSubDir,sizeof(int),0, O_NONBLOCK) != -1){
        if (cameFromSubDir != -1 )
            *shmCount += cameFromSubDir;
    }
    if (*shmCount != 0 && msgsnd(msgpid,(void *)shmCount,sizeof(int),0) == -1){
        perror("msgsnd failed ");
        closedir(folder);
        sem_destroy(&semLock);
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    if (shmctl(shmid,IPC_RMID,NULL) == -1){
        perror("shmctl failed");
        closedir(folder);
        sem_destroy(&semLock);
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    if (msgctl(msgid,IPC_RMID,NULL) == -1){
        perror("msgctl failed ");
        closedir(folder);
        sem_destroy(&semLock);
        writeSumOfFound(exePath,argv1,3);
        exit(1);
    }
    sem_destroy(&semLock);
    closedir(folder);    
}


void writeSumOfFound(const char* exeDir,const char* argv1,int exitCond)
{
    FILE *logLog,*dirTemp,*lineTemp,*dirThreadTemp;
    int i=0,sum = 0,temp,totalDir,j=0,k,mesQueue=0;
    struct timeval tEnd;
    double timedif;
    char path[PATH_MAX+1],tempStr[PATH_MAX+1],ch;
    usleep(100000);
    
    while (msgrcv(msgpid,&temp,sizeof(int),0,O_NONBLOCK) != -1){
        mesQueue += temp;
    }
    if (msgctl(msgpid,IPC_RMID,NULL) == -1){
        perror("msgctl failed ");
        exit(1);
    }
    strcpy(path,exeDir);
    strcat(path,"/dirThreadTemp");
    if ((dirThreadTemp = fopen(path,"a+")) == NULL)
    {
        perror ("dirThreadTemp File Open Failed !");
        exit(1);
    }
    for (j = 0;fgets(tempStr,1024,dirThreadTemp) != NULL;++j){
        k=0;
        while (tempStr[k] != '\0'){
            if (tempStr[k] == '\n'){
                tempStr[k] = '\0';
                break;
            }
            k++;
        }
        fscanf(dirThreadTemp,"%d%c",&temp,&ch);
        i += temp;
    }
    fclose(dirThreadTemp);
    strcpy(path,exeDir);
    strcat(path,"/log.txt");
    logLog = fopen(path,"a+");
    fprintf(logLog,"\nTotal %d '%s' found in %d files.",mesQueue,argv1,i);
    fclose(logLog);
    
    printf("Total number of strings found :    %d (String is '%s')\n",mesQueue,argv1);
    
    strcpy(path,exeDir);
    strcat(path,"/dirTemp");
    if ((dirTemp = fopen(path,"a+")) == NULL)
    {
        perror ("dirTemp File Open Failed !");
        exit(1);
    }
    for (totalDir = 0;fscanf(dirTemp,"%d",&temp) > 0;++totalDir);
    fclose(dirTemp);
    remove(path);
            
    printf("Number of directories searched :    %d\n",totalDir);
    printf("Number of files searched :    %d\n",i);
    
    sum = 0;
    strcpy(path,exeDir);
    strcat(path,"/lineTemp");
    if ((lineTemp = fopen(path,"a+")) == NULL)
    {
        perror ("lineTemp File Open Failed !");
        exit(1);
    }
    for (j = 0;fscanf(lineTemp,"%d",&temp) > 0;++j)
        sum += temp;
    fclose(lineTemp);
    remove(path);
    
    printf("Number of lines searched :    %d\n",sum);
    
    strcpy(path,exeDir);
    strcat(path,"/dirThreadTemp");
    if ((dirThreadTemp = fopen(path,"a+")) == NULL)
    {
        perror ("dirThreadTemp File Open Failed !");
        exit(1);
    }
    i = 0;
    printf("Number of cascade threads created :\n");
    for (j = 0;fgets(tempStr,1024,dirThreadTemp) != NULL;++j){
        k=0;
        while (tempStr[k] != '\0'){
            if (tempStr[k] == '\n'){
                tempStr[k] = '\0';
                break;
            }
            k++;
        }
        fscanf(dirThreadTemp,"%d%c",&temp,&ch);
        printf("\tTotal number of threads in \"%s\" is %d\n",tempStr,temp);
        i += temp;
    }
    fclose(dirThreadTemp);
    remove(path);
    
    printf("Number of search threads created :    %d\n",i);
    printf("Max %d of threads running concurrently .\n",totalDir);
    
    if (gettimeofday(&tEnd, NULL)) {
        fprintf(stderr, "Failed to get end time\n");
        exit (1);
    }
    timedif = (double)(1000000L*(tEnd.tv_sec - tStart.tv_sec) + tEnd.tv_usec - tStart.tv_usec)/1000.0;
    printf("Total run time, in milliseconds.    %7.3lf\n",timedif);
    printf("Total number of shared memory created :    %d\n",totalDir);
    printf("Sizes of shared memories created :\n==>   ");
    for (i = 0;i<totalDir;++i){
        printf("%d  ",(int)sizeof(int));
    }
    printf("\n");
    switch (exitCond){
        case 1:
            printf("Exit condition :    NORMAL\n");
            break;
        case 2:
            printf("Exit condition :    DUE TO SIGNAL\n");
            break;
        case 3:
            printf("Exit condition :    DUE TO ERROR\n");
            break;
    }
    
}

void searchWordInFile (FILE* file,const char* search,char* argv2,const char* exeDir)
{
    int i = 0;
    char readChar ;
    long int positionInFile,temp;
    FILE *logFile,*lineCountTemp;
    int row = 1;
    int count = 0;
    int col = 1;
    char fileTmp[PATH_MAX];
    int searchedRow,searchedCol;
    
    for (i=0;fgets(fileTmp,1024,file) != NULL;++i);
    strcpy(fileTmp,exeDir);
    strcat(fileTmp,"/lineTemp");
    if ((lineCountTemp = fopen(fileTmp,"a+")) == NULL)
    {
        perror ("Log File Open Failed !");
        writeSumOfFound(exeDir,search,3);
        exit(1);
    } 
    fprintf(lineCountTemp,"%d ",i);
    fclose(lineCountTemp);
    fseek(file,0,SEEK_SET);
    i=0;
    strcpy(fileTmp,exeDir);
    strcat(fileTmp,"/log.txt");
    if ((logFile = fopen(fileTmp,"a+")) == NULL)
    {
        perror ("Log File Open Failed !");
        writeSumOfFound(exeDir,search,3);
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
                fprintf(logFile,"%ld - %lu  %s : [%d,%d] '%s' first character is found.\n",(long)getpid(),pthread_self(),argv2,searchedRow,searchedCol,search);
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
    *shmCount += count ;
    fclose(logFile);
}