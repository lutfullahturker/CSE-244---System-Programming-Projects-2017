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
#include "matrixOperations.h"

typedef struct {
    int m;
    int p;
    pid_t clientPid;
}Matrix;

typedef struct {
    double A[50][50],B[50][50];
}matrixAandB;

typedef struct {
    double A[50][50],B[50][50],X[50],E;
}sendToClient;

static  volatile sig_atomic_t sigFlag = 0;
double xVecGlob[50];
int rowA,count =0,colA,concurrentlyClients = 0,forJoin,clientArrSize =0 ,clientArrCap = 100;
pthread_t* acceptTh,myTid;
pid_t *clientArr;
pthread_mutex_t mutexForConcurrently = PTHREAD_MUTEX_INITIALIZER;
void createNewMatrix(int m,int p,double arrA[][50],double arrB[][50]);
void* threadsFuncForRequest(void *arg);
void* threadsFuncForP2QR(void *arg);
void* threadsFuncForP2SVD(void *arg);
void* threadsFuncForP2PSD(void *arg);
double verify(double a[][50],double x[50],double b[][50],int m,int p);
pthread_t* reallocThArr(pthread_t* arr,int size,int cap);
void* threadsFuncForPool(void* arg);

static void sigHandler (int sigNo, siginfo_t *siginfo, void *context){
   /* int i;
    sigFlag = 1;
    if (pthread_self() == myTid){
        if (forJoin != 0){
            for(i = 0;i<forJoin;++i)
                pthread_kill(acceptTh[i],SIGINT);
        }
        for(i =0 ;i<clientArrSize;++i){
            if (kill(clientArr[i],0) != -1){
                kill(clientArr[i],SIGINT);
            }
        }
        printf("\nCtrl+C (SIGINT) caught.Program is terminating...\n");
    }*/
}


/*
 * 
 */
int main(int argc, char** argv) {
    int sock,i,thSize,thCap,portNum,whichServer,clientConnct,sizeSockaddr_in,*new_sock;
    struct sockaddr_in server,client;
    sigset_t mask;
    struct sigaction signal;
    myTid = pthread_self();
    if (argc != 3){
        fprintf(stderr,"Wrong number of arguments !\nUsage : ./server <port num, id> <thpool size, k >");
        exit(EXIT_FAILURE);
    }
    /* Ders slaytlarındaki signals bölümünden yaralanılmıştır.*/
    /*signal.sa_sigaction = &sigHandler;
    signal.sa_flags = SA_SIGINFO;
    if ((sigemptyset(&signal.sa_mask) == -1) || (sigaction(SIGINT, &signal, NULL) == -1) || (sigaction(SIGUSR1, &signal, NULL) == -1)) 
        perror("Failed to set signal handler");
    sigemptyset (&mask);
    sigaddset (&mask, SIGINT);
    sigaddset (&mask, SIGUSR1);*/
    portNum =  atoi(argv[1]);
    if ((sock = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("socket Error ");
        return 1;
    }
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(portNum );
    if( bind(sock,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("bind failed ");
        return 1;
    }
    listen(sock , 1000);
    whichServer = atoi(argv[2]);
    thSize = 0;
    thCap = 30;
    forJoin = whichServer;
    clientArr = (pid_t*)malloc(sizeof(pid_t)*clientArrCap);
    switch (whichServer){
        case 0:
            acceptTh = (pthread_t*)malloc(sizeof(pthread_t)*thCap);
            sizeSockaddr_in = sizeof(struct sockaddr_in);
            while( (clientConnct = accept(sock, (struct sockaddr *)&client, (socklen_t*)&sizeSockaddr_in)) && sigFlag == 0 )
            {
                new_sock = malloc(sizeof(int));
                *new_sock = clientConnct;
                if (thSize >= thCap){
                    acceptTh = reallocThArr(acceptTh,thSize,thCap);
                    thCap *= 2;
                }
                if( pthread_create( &acceptTh[thSize] , NULL ,  threadsFuncForRequest , (void*) new_sock) < 0)
                {
                    perror("could not create thread");
                    return 1;
                }
                thSize ++;
            }
            if (clientConnct < 0){
                perror("accept failed");
                return 1;
            }
            close(sock);
            for (i=0;i<thSize;++i)
                pthread_join(acceptTh[i],NULL);
            break;
        default :
            acceptTh = (pthread_t*)malloc(sizeof(pthread_t)*whichServer);
            for (i=0;i<whichServer;++i)
                pthread_create(&acceptTh[i],NULL,threadsFuncForPool,(void*)&sock);
            while(sigFlag == 0)
                usleep(1000);
            break;
    }
    close(sock);
    free(acceptTh);
    free(clientArr);
    pthread_mutex_destroy(&mutexForConcurrently);
    

    
    return (EXIT_SUCCESS);
}

pthread_t* reallocThArr(pthread_t* arr,int size,int cap){
    pthread_t* retVal = (pthread_t*)malloc(sizeof(pthread_t)*cap*2);
    int i;
    for (i =0;i< size;++i)
        retVal[i] = arr[i];
    return retVal;
}
void writeLog(sendToClient dataFromServer,Matrix sizes){
    FILE *file;
    char path[50] ;
    int i,j;
    sprintf(path,"Logs/Server%dLog%d.log",getpid(),++count);
    
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

void* threadsFuncForPool(void* arg){
    int sockP = *(int*)arg,sock,clientConnect;
    int read_size,i,j,shmIDP1,shmIDP3;
    Matrix cameFromClient;
    pid_t childP1,childP2,childP3,*tmpCliArr;
    matrixAandB *matrices,*P2PSD,*P2SVD,*P2QR;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t p2Th1,p2Th2,p2Th3;
    struct sockaddr_in client;
    double *xVec;
    int sizeSockaddr_in = sizeof(struct sockaddr_in);
    if(pthread_mutex_lock(&mutexForConcurrently) != 0){
            perror("MUTEX Failed ");
            raise(SIGINT);
            exit(1);
        }
        ++concurrentlyClients;
        printf("%d Clients Running Concurrently at the moment..\n",concurrentlyClients);
    if(pthread_mutex_unlock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    while (sigFlag == 0 )
    {
        if ((sock = accept(sockP, (struct sockaddr *)&client, (socklen_t*)&sizeSockaddr_in)) && sigFlag == 0){
        if( (read_size = read(sock , &cameFromClient , sizeof(Matrix))) < 0 ){
            perror("socket read failed ");
            return NULL;
        }
        if ((shmIDP1 = shmget(pthread_self(), sizeof(matrixAandB), IPC_CREAT | 0666)) < 0) {
            perror("shmget P1 failed ");
            exit(1);
        }
        if ((matrices = shmat(shmIDP1, NULL, 0)) == (matrixAandB *) -1) {
            perror("shmat matrices failed ");
            exit(1);
        }
        matrices->B[0][49] = -1;
        if ((shmIDP3 = shmget(pthread_self()+3000, sizeof(double)*cameFromClient.p, IPC_CREAT | 0666)) < 0) {
            perror("shmget P3 failed ");
            exit(1);
        }
        if ((xVec = shmat(shmIDP3, NULL, 0)) == (double *) -1) {
            perror("shmat matrices failed ");
            exit(1);
        }
        rowA = cameFromClient.p;
        colA = cameFromClient.m;
        if (clientArrSize >= clientArrCap){
            tmpCliArr = (pid_t*)malloc(sizeof(pid_t)*clientArrCap*2);
            clientArrCap *= 2;
            for (i=0;i<clientArrSize;++i)
                tmpCliArr[i] = clientArr[i];
            clientArr = tmpCliArr;
        }
        clientArr[clientArrSize++] = cameFromClient.clientPid;
        xVec[colA-1] = -100;
        childP1 = fork();
        if (childP1 == -1)
        {
            perror("Failed to fork P1");
            raise(SIGINT);
            exit(1);
        }
        else if (childP1 == 0){
            if(pthread_mutex_lock(&mutex) != 0){
                perror("MUTEX Failed ");
                raise(SIGINT);
                exit(1);
            }
            createNewMatrix(cameFromClient.m,cameFromClient.p,matrices->A,matrices->B);

            matrices->B[0][49] = 0;
            shmdt(matrices);
            if(pthread_mutex_unlock(&mutex) != 0){
                perror("MUTEX Unlock Failed ");
                raise(SIGINT);
                exit(1);
            }
            exit(1);
        }
        childP2 = fork();
        if (childP2 == -1)
        {
            perror("Failed to fork P2");
            raise(SIGINT);
            exit(1);
        }
        else if (childP2 == 0){

            while (matrices->B[0][49] != 0 && sigFlag == 0);
            if(pthread_mutex_lock(&mutex) != 0){
                perror("MUTEX Failed ");
                raise(SIGINT);
                exit(1);
            }
            P2PSD = (matrixAandB*)malloc(sizeof(matrixAandB));
            P2QR = (matrixAandB*)malloc(sizeof(matrixAandB));
            P2SVD = (matrixAandB*)malloc(sizeof(matrixAandB));
            for(i=0;i<cameFromClient.p;++i)
                for(j=0;j<cameFromClient.m;++j){
                    P2PSD->A[i][j] = matrices->A[i][j];
                    P2QR->A[i][j] = matrices->A[i][j];
                    P2SVD->A[i][j] = matrices->A[i][j];
                }
            for(i=0;i<cameFromClient.m;++i)
                P2PSD->B[i][0] = matrices->B[i][0];
                P2QR->B[i][0] = matrices->B[i][0];
                P2SVD->B[i][0] = matrices->B[i][0];
            if( pthread_create( &p2Th1 , NULL ,  threadsFuncForP2PSD , (void*) P2PSD) < 0)
            { 
                perror("could not create thread");
                return NULL;
            }
            if( pthread_create( &p2Th2 , NULL ,  threadsFuncForP2SVD , (void*) P2SVD) < 0)
            {
                perror("could not create thread");
                return NULL;
            }
            if( pthread_create( &p2Th3 , NULL ,  threadsFuncForP2QR , (void*) P2QR) < 0)
            {
                perror("could not create thread");
                return NULL;
            }
            shmdt(matrices);

            pthread_join(p2Th1,NULL);
            pthread_join(p2Th2,NULL);
            pthread_join(p2Th3,NULL);
            for(i = 0;i<colA;++i)
                xVec[i] = xVecGlob[i];
            if(pthread_mutex_unlock(&mutex) != 0){
                perror("MUTEX Unlock Failed ");
                raise(SIGINT);
                exit(1);
            }
            exit(1);
        }
        childP3 = fork();
        if (childP3 == -1)
        {
            perror("Failed to fork P3");
            raise(SIGINT);
            exit(1);
        }
        else if (childP3 == 0){
            sendToClient sent;
            while (xVec[colA-1] == -100 && sigFlag == 0);
            if(pthread_mutex_lock(&mutex) != 0){
                perror("MUTEX Failed ");
                raise(SIGINT);
                exit(1);
            }
            sent.E = verify(matrices->A,xVec,matrices->B,rowA,colA);
            for(i=0;i<rowA;++i){
                for(j=0;j<colA;++j){
                    sent.A[i][j] = matrices->A[i][j];
                    sent.X[j] = xVec[j];
                }
                sent.B[i][0] = matrices->B[i][0];
            }
            if( send(sock , &sent , sizeof(sendToClient) , 0) < 0)
            {
                fprintf(stderr,"Send Values to Client failed\n");
                return NULL;
            }
            writeLog(sent,cameFromClient);
            if (shmctl(shmIDP1,IPC_RMID,NULL) == -1){
                perror("shmctl matrices failed");
                exit(1);
            }
            if (shmctl(shmIDP3,IPC_RMID,NULL) == -1){
                perror("shmctl matrices failed");
                exit(1);
            }
            if(pthread_mutex_unlock(&mutex) != 0){
                perror("MUTEX Unlock Failed ");
                raise(SIGINT);
                exit(1);
            }
            exit(1);
        }
        
        while (wait(NULL) != -1);
        close(sock);
        }
    }
    if(pthread_mutex_lock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    --concurrentlyClients;
    printf("%d Clients Running Concurrently at the moment..\n",concurrentlyClients);
    if(pthread_mutex_unlock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    pthread_mutex_destroy(&mutex);
}

void* threadsFuncForRequest(void *arg){
    int sock = *(int*)arg;
    int read_size,i,j,shmIDP1,shmIDP3;
    Matrix cameFromClient;
    pid_t childP1,childP2,childP3;
    matrixAandB *matrices,*P2PSD,*P2SVD,*P2QR;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_t p2Th1,p2Th2,p2Th3;
    double *xVec;
    
    if(pthread_mutex_lock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    ++concurrentlyClients;
    printf("%d Clients Running Concurrently at the moment..\n",concurrentlyClients);
    if(pthread_mutex_unlock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    if( (read_size = read(sock , &cameFromClient , sizeof(Matrix))) < 0 ){
        perror("socket read failed ");
        return NULL;
    }
    if ((shmIDP1 = shmget(pthread_self(), sizeof(matrixAandB), IPC_CREAT | 0666)) < 0) {
        perror("shmget P1 failed ");
        exit(1);
    }
    if ((matrices = shmat(shmIDP1, NULL, 0)) == (matrixAandB *) -1) {
        perror("shmat matrices failed ");
        exit(1);
    }
    matrices->B[0][49] = -1;
    if ((shmIDP3 = shmget(pthread_self()+3000, sizeof(double)*cameFromClient.p, IPC_CREAT | 0666)) < 0) {
        perror("shmget P3 failed ");
        exit(1);
    }
    if ((xVec = shmat(shmIDP3, NULL, 0)) == (double *) -1) {
        perror("shmat matrices failed ");
        exit(1);
    }
    rowA = cameFromClient.p;
    colA = cameFromClient.m;
    xVec[colA-1] = -100;
    childP1 = fork();
    if (childP1 == -1)
    {
        perror("Failed to fork P1");
        raise(SIGINT);
        exit(1);
    }
    else if (childP1 == 0){
        if(pthread_mutex_lock(&mutex) != 0){
            perror("MUTEX Failed ");
            raise(SIGINT);
            exit(1);
        }
        createNewMatrix(cameFromClient.m,cameFromClient.p,matrices->A,matrices->B);
        matrices->B[0][49] = 0;
        shmdt(matrices);
        if(pthread_mutex_unlock(&mutex) != 0){
            perror("MUTEX Unlock Failed ");
            raise(SIGINT);
            exit(1);
        }
        exit(1);
    }
    childP2 = fork();
    if (childP2 == -1)
    {
        perror("Failed to fork P2");
        raise(SIGINT);
        exit(1);
    }
    else if (childP2 == 0){
        
        while (matrices->B[0][49] != 0 && sigFlag == 0);
        if(pthread_mutex_lock(&mutex) != 0){
            perror("MUTEX Failed ");
            raise(SIGINT);
            exit(1);
        }
        P2PSD = (matrixAandB*)malloc(sizeof(matrixAandB));
        P2QR = (matrixAandB*)malloc(sizeof(matrixAandB));
        P2SVD = (matrixAandB*)malloc(sizeof(matrixAandB));
        for(i=0;i<cameFromClient.p;++i)
            for(j=0;j<cameFromClient.m;++j){
                P2PSD->A[i][j] = matrices->A[i][j];
                P2QR->A[i][j] = matrices->A[i][j];
                P2SVD->A[i][j] = matrices->A[i][j];
            }
        for(i=0;i<cameFromClient.m;++i)
            P2PSD->B[i][0] = matrices->B[i][0];
            P2QR->B[i][0] = matrices->B[i][0];
            P2SVD->B[i][0] = matrices->B[i][0];
        if( pthread_create( &p2Th1 , NULL ,  threadsFuncForP2PSD , (void*) P2PSD) < 0)
        { 
            perror("could not create thread");
            return NULL;
        }
        if( pthread_create( &p2Th2 , NULL ,  threadsFuncForP2SVD , (void*) P2SVD) < 0)
        {
            perror("could not create thread");
            return NULL;
        }
        if( pthread_create( &p2Th3 , NULL ,  threadsFuncForP2QR , (void*) P2QR) < 0)
        {
            perror("could not create thread");
            return NULL;
        }
        shmdt(matrices);
        
        pthread_join(p2Th1,NULL);
        pthread_join(p2Th2,NULL);
        pthread_join(p2Th3,NULL);
        for(i = 0;i<colA;++i)
            xVec[i] = xVecGlob[i];
        if(pthread_mutex_unlock(&mutex) != 0){
            perror("MUTEX Unlock Failed ");
            raise(SIGINT);
            exit(1);
        }
        exit(1);
    }
    childP3 = fork();
    if (childP3 == -1)
    {
        perror("Failed to fork P3");
        raise(SIGINT);
        exit(1);
    }
    else if (childP3 == 0){
        sendToClient sent;
        while (xVec[colA-1] == -100 && sigFlag == 0);
        if(pthread_mutex_lock(&mutex) != 0){
            perror("MUTEX Failed ");
            raise(SIGINT);
            exit(1);
        }
        sent.E = verify(matrices->A,xVec,matrices->B,rowA,colA);
        for(i=0;i<rowA;++i){
            for(j=0;j<colA;++j){
                sent.A[i][j] = matrices->A[i][j];
                sent.X[j] = xVec[j];
            }
            sent.B[i][0] = matrices->B[i][0];
        }
        if( send(sock , &sent , sizeof(sendToClient) , 0) < 0)
        {
            fprintf(stderr,"Send Values to Client failed\n");
            return NULL;
        }
        if (shmctl(shmIDP1,IPC_RMID,NULL) == -1){
            perror("shmctl matrices failed");
            exit(1);
        }
        if (shmctl(shmIDP3,IPC_RMID,NULL) == -1){
            perror("shmctl matrices failed");
            exit(1);
        }
        if(pthread_mutex_unlock(&mutex) != 0){
            perror("MUTEX Unlock Failed ");
            raise(SIGINT);
            exit(1);
        }
        exit(1);
    }
    if(pthread_mutex_lock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    --concurrentlyClients;
    printf("%d Clients Running Concurrently at the moment..\n",concurrentlyClients);
    if(pthread_mutex_unlock(&mutexForConcurrently) != 0){
        perror("MUTEX Failed ");
        raise(SIGINT);
        exit(1);
    }
    while (wait(NULL) != -1);
    pthread_mutex_destroy(&mutex);
    close(sock);
    free(arg);
}

void* threadsFuncForP2QR(void *arg){
    /*double Qarr[50][50],Rarr[50][50],xQR[50][50];
    int i,j;
        mat R, Q;
	mat x = matrix_copy(colA, (*(matrixAandB*)arg).A, rowA);
	householder(x, &R, &Q);
        for (i = 0;i<Q->m;++i){
            for (j=0;j<Q->n;++j){
                Qarr[i][j] = Q->v[i][j];
            }
        }
        for (i = 0;i<R->m;++i){
            for (j=0;j<R->n;++j){
                Rarr[i][j] = R->v[i][j];
            }
        }
        QRFactorization(Qarr,Rarr,(*(matrixAandB*)arg).B,xQR,rowA,colA);
        matrix_delete(x);
	matrix_delete(R);
	matrix_delete(Q);
    */
    
    free(arg);
}
void* threadsFuncForP2SVD(void *arg){
    double x[50],v[50][50],w[50],bTmp[50],aTmp[50][50];
    int i,j;
    for (i=0;i<rowA;++i){
        bTmp[i] = (*(matrixAandB*)arg).B[i][0];
        for (j=0;j<colA;++j)
            aTmp[i][j] = (*(matrixAandB*)arg).A[i][j];
    }
    dsvd(aTmp, rowA, colA, w, v);
    solveSvd(aTmp, w, v, rowA, colA,bTmp, x);   
    
    free(arg);
}
void* threadsFuncForP2PSD(void *arg){
    double x[50][50],tmpx[50][50];
    int i;
    pseudoInverse((*(matrixAandB*)arg).A,(*(matrixAandB*)arg).B,x,rowA,colA);
    for(i = 0;i<colA;++i){
        xVecGlob[i] = x[i][0];
    }
    free(arg);
    return NULL;
}


void createNewMatrix(int m,int p,double arrA[][50],double arrB[][50]){
    int j,k;
    struct timeval t1;
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);
    if (m > 50 || p > 50){
        fprintf(stderr,"Matrix Size cannot be greater than 50 !\n");
        raise(SIGINT);
        exit(1);
    }
    for (j=0;j < p;++j)
        for (k=0;k < m;++k)
            arrA[j][k] = (rand()%100)+1;
    for (j = 0;j<p;++j)
        arrB[j][0] = (rand()%100)+1;
}

double verify(double a[][50],double x[50],double b[][50],int m,int p){
    double e[50][50],multAX[50][50],transposeE[50][50],multESqr[50][50],xTmp[50][50],result;
    int i;
    for(i = 0;i<p;++i)
        xTmp[i][0] = x[i];
    matrixMult(multAX,a,xTmp,m,p,1);
    for(i=0;i<m;++i)
        e[i][0] = multAX[i][0] - b[i][0];
    transpose(e,transposeE,m,1);
    matrixMult(multESqr,transposeE,e,1,m,1);
    result = sqrt(multESqr[0][0]);
    
    return result;
}