/* 
 * File:   seeWhat.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 16 April 2017, 15:30
 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>

#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static  volatile sig_atomic_t sigFlag = 0;


void writeLog(int cPid,double result1,double result2,double time1,double time2);


static void sigHandler (int sigNo, siginfo_t *siginfo, void *context){

    sigFlag = 1;
    printf("\nCtrl+C (SIGINT) caught.Program is terminating...\n");
}


int main(int argc, char** argv) {

    struct sigaction signal;
    int result,tmpFifo,cPid,clientPid=0;
    double result1,result2;
    pid_t child;
    double time1,time2;
    char tmpFifoName[50];
    FILE* pidTemp;
    
    
    if (argc != 1){
        fprintf(stderr,"Wrong number of arguments !\nUsage : ./showResult ");
        exit(EXIT_FAILURE);
    }
    
    signal.sa_sigaction = &sigHandler;
    signal.sa_flags = SA_SIGINFO;
    if ((sigemptyset(&signal.sa_mask) == -1) || (sigaction(SIGINT, &signal, NULL) == -1)) 
        perror("Failed to set signal handler");
    
    while (((pidTemp = fopen("pidTemp", "r")) == NULL))
        if (sigFlag)
            exit(1);

    while (((result = (fscanf(pidTemp, "%d",&clientPid))) >=0 || result == EOF) && sigFlag == 0){
        if (result > 0){
            child = fork();
            if (child == -1)
            {
                perror("Failed to fork ");
                raise(SIGINT);
                fclose(pidTemp);
                exit(1);
            }
            else if (child == 0){
                sprintf(tmpFifoName,"temp%d",clientPid);
                while(kill((pid_t)clientPid,0) != -1 && !sigFlag){
                    while (((tmpFifo = open(tmpFifoName, O_RDONLY | O_NONBLOCK)) == -1)){
                        if (kill((pid_t)clientPid,0) == -1 || sigFlag == 1){
                            fclose(pidTemp);
                            exit(1);
                        }
                    }
                    usleep(1000);
                    read(tmpFifo,&cPid,sizeof(int));
                    read(tmpFifo,&result1,sizeof(double));
                    read(tmpFifo,&time1,sizeof(double));
                    read(tmpFifo,&result2,sizeof(double));
                    read(tmpFifo,&time2,sizeof(double));
                    printf("Pid - %d\tResult1 - %.3lf\tResult2 - %.3lf\n\n",cPid,result1,result2);
                    writeLog(cPid,result1,result2,time1,time2);

                    close(tmpFifo);
                    usleep(800000);
                }
                if (kill((pid_t)clientPid,0) != -1)
                    kill((pid_t)clientPid,SIGINT);
                fclose(pidTemp);
                exit(1);
            }
        }
    }
    fclose(pidTemp);
    remove("pidTemp");
        
    
    
    return (EXIT_SUCCESS);
}

void writeLog(int cPid,double result1,double result2,double time1,double time2)
{
    FILE* log;
    
    if ((log = fopen("log/showResult.log","a+")) == NULL)
    {
        perror ( "showResult.log open failed !");
        kill(getppid(),SIGINT);
        return;
    }
    
    fprintf(log,"Pid of Client ==> %d\nResult 1 = %.4lf\tElapsed Time = %.4lf\nResult 2 = %.5lf\tElapsed Time = %.4lf\n\n",
            cPid,result1,time1,result2,time2);
    fflush(log);
    fclose(log);
}
