/* 
 * File:   main.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 25 May 2017, 17:28
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>

/*
 * 
 */
sem_t sem,semRead;
typedef struct {
    int m;
    int p;
    pid_t clientPid;
}Matrix;

typedef struct {
    Matrix matrix;
    struct sockaddr_in server;
    int portNum;
}forThread;

typedef struct {
    double A[50][50],B[50][50],X[50],E;
}sentFromServer;
double *connectionTimes;
int count = 1;

void* threadsFunc(void *arg);
void writeLog(sentFromServer dataFromServer,Matrix sizes);
void writeClientsLog(int q);



int main(int argc, char** argv) {

    int m,p,q,i,sock,portNum;
    forThread *args;
    pthread_t *threadArr;
    pid_t myPid = getpid();

        
    if (argc != 5){
        fprintf(stderr,"Wrong number of arguments !\nUsage : ./clients <num of columns of A, m> <num of rows of A, p> <num of clients, q> <port num, id >\n");
        exit(EXIT_FAILURE);
    }

    q = atoi(argv[3]);
    if (sem_init(&sem, 0, 1) == -1) {
        perror("Failed to initialize semaphore");
        exit(1) ;
    }
    if (sem_init(&semRead, 0, 1) == -1) {
        perror("Failed to initialize semaphoreRead");
        exit(1) ;
    }
    
    
    
    m = atoi(argv[1]);
    p = atoi(argv[2]);
    portNum =  atoi(argv[4]);
    connectionTimes = (double*)malloc(sizeof(double)*q);
    args = (forThread*)malloc(sizeof(forThread)*q);
    threadArr = (pthread_t *)malloc(sizeof(pthread_t)*q);
    for(i = 0;i<q;i++)
        connectionTimes[i] = 0 ;
    for (i = 0;i< q;++i){
        
        args[i].matrix.m = m;
        args[i].matrix.p = p;
        args[i].matrix.clientPid = myPid;
        args[i].portNum = portNum;
        if (pthread_create(&threadArr[i], NULL, threadsFunc, &args[i])) {
            perror("Failed to create thread ");
            sem_destroy(&sem);
            sem_destroy(&semRead);
            exit(1);
        }
        
        
        
        
    }
    
    for (i = 0 ;i< q;++i){
        if (pthread_join(threadArr[i],NULL) == -1){
            perror("thread join failed");
            sem_destroy(&sem);
            sem_destroy(&semRead);
            return (1);
        }
    }
    writeClientsLog(q);
    sem_destroy(&sem);
    sem_destroy(&semRead);
    free(args);
    free(threadArr);
    return (EXIT_SUCCESS);
}

void* threadsFunc(void *arg){
    int sock,rcv,portNum = (*(forThread*)arg).portNum ,read_size,i,j;
    struct sockaddr_in server = (*(forThread*)arg).server;
    Matrix matrix = (*(forThread*)arg).matrix;
    sentFromServer fromServer;
    struct timeval tStart,tEnd;
    double timedif;
    
    if (gettimeofday(&tStart, NULL)) {
      fprintf(stderr, "Failed to get start time\n");
      return NULL;
    }
    if ((sock = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("socket Error ");
        return NULL;
        }
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(portNum );
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return NULL;
    }
    
    
    
    if (sem_wait(&sem) == -1){
        fprintf(stderr,"sem_wait Failed ");
        sem_destroy(&sem);
        sem_destroy(&semRead);
        close(sock);
        exit(1);
    }
    if( send(sock , &matrix , sizeof(Matrix) , 0) < 0)
    {
        fprintf(stderr,"Send m and p failed\n");
        return NULL;
    }
    
    if (sem_post(&sem) == -1){
            fprintf(stderr, "Thread failed to unlock semaphore\n");
            return NULL;
    }
    if (sem_wait(&semRead) == -1){
        fprintf(stderr,"sem_wait Failed ");
        sem_destroy(&sem);
        sem_destroy(&semRead);
        close(sock);
        exit(1);
    }
    while( (read_size = recv(sock , &fromServer , sizeof(sentFromServer),MSG_WAITALL)) > 0 );
    if(read_size == -1)
    {
        perror("socket read failed ");
        return NULL;
    }
    if (gettimeofday(&tEnd, NULL)) {
      fprintf(stderr, "Failed to get end time\n");
      exit(1);
    }
    timedif = (double)(1000000L*(tEnd.tv_sec - tStart.tv_sec) + tEnd.tv_usec - tStart.tv_usec)/1000.0;
    connectionTimes[count-1] = timedif;
    writeLog(fromServer,matrix);
    if (sem_post(&semRead) == -1){
        fprintf(stderr,"sem_wait Failed ");
        sem_destroy(&sem);
        sem_destroy(&semRead);
        close(sock);
        exit(1);
    }
    
    printf("serverdan aldım  \nA ==> \n");
    for(i = 0 ;i<matrix.p;++i){
        for(j=0;j<matrix.m;++j){
            printf("A[%d][%d]  %.2lf\t",i,j,fromServer.A[i][j]);
        }
        printf("\n");
    }
    printf("\n");
    for(j=0;j<matrix.p;++j){
            printf("B[%d]  %.2lf\t",j,fromServer.B[j][0]);
        }
    printf("\n\n");
    for(j=0;j<matrix.m;++j){
            printf("X[%d]  %.2lf\t",j,fromServer.X[j]);
        }
    printf("\n\nE ==> %.3lf\n\n",fromServer.E);
    
    close(sock);
}

void writeClientsLog(int q){
    FILE *file;
    char path[50] ;
    int i,j,tmpcount = 0;
    double mean = 0.0,s=0.0;
    sprintf(path,"Logs/Clients/Client%d.log",getpid());
    
    if ((file = fopen(path,"a+")) == NULL){
        perror("fopen failed");
        exit(1);
    }
    for (i=0;i<q;++i)
        if (connectionTimes[i] != 0){
            mean += connectionTimes[i];
            tmpcount ++;
        }
    mean = mean/(double)tmpcount;
    for(i = 0;i<q;++i)
        s += pow(connectionTimes[i]-mean,2);
    s = s/tmpcount;
    s = sqrt(s);
    fprintf(file,"Average connection time is (in milliseconds) ==>>  %.3lf\n\n",mean);
    fprintf(file,"The standard deviation s is ==>>  %.3lf\n\n",s);
    
    fprintf(stdout,"Average connection time is ==>>  %.3lf\n\n",mean);
    fprintf(stdout,"The standard deviation s is ==>>  %.3lf\n\n",s);
    
    fclose(file);
}

void writeLog(sentFromServer dataFromServer,Matrix sizes){
    FILE *file;
    char path[50] ;
    int i,j;
    sprintf(path,"Logs/Client%dLog%d.log",getpid(),count++);
    
    if ((file = fopen(path,"a+")) == NULL){
        perror("fopen failed");
        exit(1);
    }
    fprintf(file,"Thread id  ==>>\t\t%ld\n\n",pthread_self());
    fprintf(file,"Error Norm e ==>\t\t%.3lf\n\nA Matrix is ==>> \n\n",dataFromServer.E);
    for(i =0 ;i<sizes.p;++i){
        for(j=0;j<sizes.m;++j)
            fprintf(file,"A[%d][%d]\t %.2lf\t",i,j,dataFromServer.A[i][j]);
        fprintf(file,"\n");
    }
    fprintf(file,"\nX Matrix is ==>>\n\n");
    for(i =0 ;i<sizes.m;++i)
        fprintf(file,"X[%d]   %.3lf\t ",i,dataFromServer.X[i]);
    fprintf(file,"\n\n");
    fprintf(file,"B Matrix is ==>>\n\n");
    for(i =0 ;i<sizes.p;++i)
        fprintf(file,"B[%d]  %.2lf\t ",i,dataFromServer.B[i][0]);
    fclose(file);
}