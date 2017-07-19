/* 
 * File:   timeServer.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 12 April 2017, 15:37
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>


#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


static  volatile sig_atomic_t sigFlag = 0;
static  volatile sig_atomic_t sigClientFlag = 0;
pid_t client_arr[200];
static int used = 0;


void createNewMatrix(int n,struct timeval tStart,pid_t client);
void writeLog(double det,double time,pid_t client);
double findDet(double a[20][20] , double k);
int searchInPids(pid_t arr[200],pid_t search,int size);

static void sigHandler (int sigNo, siginfo_t *siginfo, void *context){
    if (sigNo == SIGUSR1){
        sigClientFlag = 1;
        if (used >= 200)
            exit(1);
        if (searchInPids(client_arr,siginfo->si_pid,used) == 0)
            client_arr[used++] = siginfo->si_pid;
    }
    else{
        sigFlag = 1;
        printf("\nCtrl+C (SIGINT) caught.Program is terminating...\n");
    }
}

/*
 * 
 */
int main(int argc, char** argv) {

    
    struct sigaction signal;
    int mainFifo,i;
    pid_t child,myPid,mainChild;
    long timer = atoi(argv[1])*1000;
    struct timeval tStart;
    sigset_t mask;
    
   if (gettimeofday(&tStart, NULL)) {
      fprintf(stderr, "Failed to get start time\n");
      return 1;
   }
   
    
    if (argc != 4){
        fprintf(stderr,"Wrong number of arguments !\nUsage : ./timeServer <ticks in miliseconds> <n> <mainpipename>");
        exit(EXIT_FAILURE);
    }
    if (atoi(argv[2]) > 10){
        fprintf(stderr,"Matrix order <n> cannot be greater than 10 ");
        exit(EXIT_FAILURE);
    }

    
    
    /* Ders slaytlarındaki signals bölümünden yaralanılmıştır.*/
    signal.sa_sigaction = &sigHandler;
    signal.sa_flags = SA_SIGINFO;
    if ((sigemptyset(&signal.sa_mask) == -1) || (sigaction(SIGINT, &signal, NULL) == -1) || (sigaction(SIGUSR1, &signal, NULL) == -1)) 
        perror("Failed to set signal handler");
    sigemptyset (&mask);
    sigaddset (&mask, SIGINT);
    sigaddset (&mask, SIGUSR1);
    
    
    
    
    
    
    mainChild = fork();
    if (mainChild == -1)
    {
        perror("Failed to fork");
        raise(SIGINT);
        exit(1);
    }
    else if (mainChild == 0){
        while(!sigFlag){
            if (mkfifo(argv[3], FIFO_PERMS) == -1){
                perror("Failed to create mainFifo");
                unlink(argv[3]);
                exit(1);
            }
            if ((mainFifo = open(argv[3], O_WRONLY)) < 0) {
                    unlink(argv[3]);
                    exit(1);
            }
            myPid = getppid();
            write(mainFifo,&myPid,sizeof(pid_t));
            close(mainFifo);
            unlink(argv[3]);
        }
        exit(1);
    }
    while(!sigFlag){
        if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) {
            perror ("sigprocmask error ");
            return 1;
        } 
        usleep(timer);
        
        sigprocmask(SIG_UNBLOCK, &mask, NULL);
        if (sigClientFlag == 1){
            for (i = 0;i<used;++i){
                child = fork();
                if (child == -1)
                {
                    perror("Failed to fork");
                    unlink(argv[3]);
                    exit(1);
                }
                else if (child == 0)
                {
                    createNewMatrix(atoi(argv[2]),tStart,client_arr[i]);
                    exit(1);
                }  
            }
            wait(NULL);

            sigClientFlag = 0;

           for (i=0;i<used;++i)
                if (kill(client_arr[i],0) == -1){
                    close(mainFifo);
                    unlink(argv[3]);
                    exit(1);
                }
                    
            used = 0;
        }
        
        
    }
    return (EXIT_SUCCESS);
}

int searchInPids(pid_t arr[200],pid_t search,int size)
{
    int i ;
    for (i = 0;i<size;++i){
        if (search == arr[i])
            return 1;
    }
    return 0 ;
}


void createNewMatrix(int n,struct timeval tStart,pid_t client)
{
    struct timeval t1,tEnd;
    char fifoName[50];
    double matrix[20][20],det;
    int fifo,j,k,flag = 1;
    double timedif;
    
    sprintf(fifoName,"Client%d",client);
    if (mkfifo(fifoName, FIFO_PERMS) == -1){
        perror("Failed to create matrixFifo");
        raise(SIGINT);
    }
    if (kill(client,0) == -1){
        unlink(fifoName);
        exit(1);
    }
    while (((fifo = open(fifoName, O_WRONLY)) == -1)) {
        if (kill(client,0) == -1){
            unlink(fifoName);
            exit(1);
        }
    }
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);

    while(flag){
        for (j=0;j<2*n;++j)
            for (k=0;k<2*n;++k)
                matrix[j][k] = rand()%250-125;
        
            if ((det = findDet(matrix,(double)2*n)) == 0);
            else
                flag = 0;
    }  
    /*
     * http://bilgisayarkavramlari.sadievrenseker.com/2009/01/01/c-ile-zaman-islemleri/
     * sitesinden yararlanılmıştır.
     */ 
    write(fifo,&n,sizeof(int));
    for (j=0;j<2*n;++j){
        for (k=0;k<2*n;++k)
            if (write(fifo,&matrix[j][k],sizeof(double))<0)
            {
                perror("MatrixFifo writing error !");
                raise(SIGINT);
            }
    }
    
    close(fifo);
    if (gettimeofday(&tEnd, NULL)) {
      fprintf(stderr, "Failed to get end time\n");
      exit(1);
    }
    timedif = (double)(1000000L*(tEnd.tv_sec - tStart.tv_sec) + tEnd.tv_usec - tStart.tv_usec)/1000.0;
    writeLog(det,timedif,client);
}
   
void writeLog(double det,double time,pid_t client){
    FILE* mainLog;
    if ((mainLog = fopen("log/timeServer.log","a+")) ==NULL){
        perror ("timeServer.log Open Failed ");
        raise(SIGINT);
    }
    fprintf(mainLog,"%7.3lf\t\t",time);
    fprintf(mainLog,"%d\t\t",client);
    fprintf(mainLog,"%.0lf\n",det);
    fclose(mainLog);
}


/*
 http://www.ccodechamp.com/c-program-to-find-determinant-of-matrix-c-code-champ/
 */
double findDet(double a[20][20] , double k){
  double s=1,det=0,b[20][20];
  int i,j,m,n,c;
  if (k==1)
    {
     return (a[0][0]);
    }
  else
    {
     det=0;
     for (c=0;c<k;c++)
       {
        m=0;
        n=0;
        for (i=0;i<k;i++)
          {
            for (j=0;j<k;j++)
              {
                b[i][j]=0;
                if (i != 0 && j != c)
                 {
                   b[m][n]=a[i][j];
                   if (n<(k-2))
                    n++;
                   else
                    {
                     n=0;
                     m++;
                     }
                   }
               }
             }
          det=det + s * (a[0][c] * findDet(b,k-1));
          s=-1 * s;
          }
    }
 
    return (det);
}
