/* 
 * File:   seeWhat.c
 * Author: Lütfullah TÜRKER
 *
 * Created on 13 April 2017, 13:30
 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>

#define FIFO_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

static  volatile sig_atomic_t sigFlag = 0;
double determinant(double a[20][20],double k);
void cofactor(double num[10][20],double f);
void transpose(double num[10][20],double fac[10][20],double r);
void fillNxNMatrix(double matrix[20][20],double matInv[10][20],int whichRegion,int n);
void convolution(double in[20][20], double out[20][20], int size,double kernel[3][3]);
void fillLog(double matrix[20][20],int n,FILE* matrixLog);
void sendShowResult(int tmpFifo,double result,double timedifInverse);

static int logCount = 0;

static void sigHandler (int sigNo, siginfo_t *siginfo, void *context){

    sigFlag = 1;
    printf("\nCtrl+C (SIGINT) caught.Program is terminating...\n");
}


int main(int argc, char** argv) {
    int readMatrix,i,j,n,m,l,mainFifo,tmpFifo,myPid;
    char matrixFifo[20],logName[40],tmpFifoName[50];
    struct sigaction signal;
    double matrix[20][20],inversedMatrix[20][20],k,detMain,detInverse,detConv,matInv1[10][20],matInv2[10][20]
    ,matInv3[10][20],matInv4[10][20],result1,result2,kernel[3][3],matConv[20][20];
    pid_t serverPid,childInverse,childConv;
    long timedifConv,timedifInverse;
    struct timeval endConv,endInverse;
    struct timeval startConv,startInverse;
    FILE *matrixLog,*pidTemp;
    
    
    if (argc != 2){
        fprintf(stderr,"Wrong number of arguments !\nUsage : ./seeWhat <mainpipename> ");
        exit(EXIT_FAILURE);
    }
    
    signal.sa_sigaction = &sigHandler;
    signal.sa_flags = SA_SIGINFO;
    if ((sigemptyset(&signal.sa_mask) == -1) || (sigaction(SIGINT, &signal, NULL) == -1)) 
        perror("Failed to set signal handler");
    
    myPid = getpid();
    /*
     * Kernel matris olarak 2. matris i seçtim.
     */
    
    for (i=0;i<3;++i)
        for(j=0;j<3;++j)
            kernel[i][j] = 0;
    kernel[0][1] = 1;
    kernel[2][0] = 1;
    kernel[2][1] = 1;
    kernel[0][2] = -1;
    kernel[1][0] = -1;
    kernel[1][2] = -1;
    
    while (((mainFifo = open(argv[1], O_RDONLY,FIFO_PERMS)) <0))
        if (sigFlag)
            exit(1);
    if ((read(mainFifo, &serverPid, sizeof(pid_t))) <0){
        perror("mainFifo read error ");
        raise(SIGINT);
    }
    close(mainFifo);
    sprintf(matrixFifo,"Client%d",getpid());

    if (((pidTemp = fopen("pidTemp", "a+")) == NULL)){
        if (sigFlag){
            remove("pidTemp");
            exit(1);
        }
    }
    fprintf(pidTemp,"%d ",myPid);
    fclose(pidTemp);
    

    sprintf(tmpFifoName,"temp%d",getpid());
    while (!sigFlag)
    {
        if (kill(serverPid,SIGUSR1) == -1){
            remove("pidTemp");
            exit(1);
        }
        
        sprintf(logName,"log/Client%dMatrix%d.log",getpid(),++logCount);
        if ((matrixLog = fopen(logName,"a+")) ==NULL){
            perror ("ClientMatrixLog Open Failed ");
            raise(SIGINT);
            remove("pidTemp");
            exit(1);
        }

        while (((readMatrix = open(matrixFifo,O_RDONLY)) == -1)){
            if (sigFlag){
                fclose(matrixLog);
                remove("pidTemp");
                exit(1);
            }     
        }
        if ((read(readMatrix, &n, sizeof(int))) <=0){
            perror("matrixFifo is empty ");
            close(readMatrix);
            fclose(matrixLog);
            unlink(matrixFifo);
            remove("pidTemp");
            exit(1);
        }
        
        for (i=0;i<2*n;++i)
            for(j=0;j<2*n;++j)
                matConv[i][j] = 0 ;
        
        
        
        for(i = 0;i<2*n;++i){
            for (j = 0;j<2*n;++j){
                if (read(readMatrix, &matrix[i][j], sizeof(double)) < 0){
                    perror("matrixFifo read error ");
                    close(readMatrix);
                    fclose(matrixLog);
                    unlink(matrixFifo);
                    remove("pidTemp");
                    if (kill(serverPid,0) != -1)
                        kill(serverPid,SIGINT);
                    exit(1);
                }
            }
        }
        fprintf(matrixLog,"The Original Matrix is  ==>>\n");
        fprintf(matrixLog,"original = [");
        fillLog(matrix,2*n,matrixLog);
        fflush(matrixLog);
        k=(double)n;
        if ((detMain=determinant(matrix,2*k)) == 0){
            close(readMatrix);
            fclose(matrixLog);
            unlink(matrixFifo);
            continue;
        }

        
        
        if (mkfifo(tmpFifoName, FIFO_PERMS) == -1){
            perror("Failed to create tmpFifo");
            close(readMatrix);
            fclose(matrixLog);
            unlink(matrixFifo);
            if (kill(serverPid,0) != -1)
                kill(serverPid,SIGINT);
            exit(1);
        }
        if (((tmpFifo = open(tmpFifoName, O_RDWR)) == -1)){
            if (kill(serverPid,0) == -1 || sigFlag == 1){
                close(readMatrix);
                fclose(matrixLog);
                unlink(matrixFifo);
                exit(1);
            }
        }
        
        write(tmpFifo,&myPid,sizeof(int));
        
        childInverse = fork();
        if (childInverse == -1)
        {
            perror("Failed to fork shifted inverse process ");
            raise(SIGINT);
            close(readMatrix);
            unlink(matrixFifo);
            fclose(matrixLog);
            close(tmpFifo);
            unlink(tmpFifoName);
            if (kill(serverPid,0) != -1)
                kill(serverPid,SIGINT);
            exit(1);
        }
        else if (childInverse == 0)
        {

            if (gettimeofday(&startInverse, NULL)) {
                fprintf(stderr, "Failed to get start time\n");
                raise(SIGINT);
                close(readMatrix);
                fclose(matrixLog);
                unlink(matrixFifo);
                close(tmpFifo);
                unlink(tmpFifoName);
                result1 = 0;
                exit(1);
            }
            
            fillNxNMatrix(matrix,matInv1,1,n);
            fillNxNMatrix(matrix,matInv2,2,n);
            fillNxNMatrix(matrix,matInv3,3,n);
            fillNxNMatrix(matrix,matInv4,4,n);

            if (determinant(matInv1,k) == 0 || determinant(matInv2,k) == 0 
                || determinant(matInv3,k) == 0 || determinant(matInv4,k) == 0){
                close(readMatrix);
                fclose(matrixLog);
                unlink(matrixFifo);
                close(tmpFifo);
                unlink(tmpFifoName);
                result1 = 0 ;
                exit(1);
            }
            else{
                cofactor(matInv1,k); 
                cofactor(matInv2,k); 
                cofactor(matInv3,k); 
                cofactor(matInv4,k); 
            }
            for(i = 0;i<n;++i)
                for(j=0;j<n;++j)
                    inversedMatrix[i][j] = matInv1[i][j];

            for(i = 0;i<n;++i)
                 for(j=n,l=0;j<2*n;++j,++l)
                     inversedMatrix[i][j] = matInv2[i][l];

            for(i = n,l=0;i<2*n;++i,++l)
                for(j=0;j<n;++j)
                    inversedMatrix[i][j] = matInv3[l][j];

            for(i = n,m=0;i<2*n;++i,++m)
                for(j=n,l=0;j<2*n;++j,++l)
                    inversedMatrix[i][j] = matInv4[m][l];
            
            detInverse = determinant(inversedMatrix,2*k);
            result1 = detMain - detInverse ;
            
            if (gettimeofday(&endInverse, NULL)) {
                fprintf(stderr, "Failed to get end time\n");
                close(readMatrix);
                fclose(matrixLog);
                unlink(matrixFifo);
                close(tmpFifo);
                unlink(tmpFifoName);
                exit(1);
            }
            
            fprintf(matrixLog,"]\n\n\nThe Inversed Matrix is  ==>>\ninversed = [");
            fillLog(inversedMatrix,2*n,matrixLog);
            fflush(matrixLog);
            /*
             * Çalışma süresini mikrosaniye olarak hesaplıyorum.
             */
            timedifInverse = 1000000L*(endInverse.tv_sec - startInverse.tv_sec) + endInverse.tv_usec - startInverse.tv_usec;
            
            sendShowResult(tmpFifo,result1,(double)timedifInverse/1000.0);
            
            close(readMatrix);
            fclose(matrixLog);
            close(tmpFifo);
            exit(1);
        }
        wait(NULL);
        childConv = fork();
        if (childConv == -1)
        {
            perror("Failed to fork convolution process ");
            raise(SIGINT);
            unlink(matrixFifo);
            if (kill(serverPid,0) != -1)
                kill(serverPid,SIGINT);
        }
        else if (childConv == 0)
        {
            
            if (gettimeofday(&startConv, NULL)) {
                fprintf(stderr, "Failed to get start time\n");
                raise(SIGINT);
                close(readMatrix);
                fclose(matrixLog);
                close(tmpFifo);
                unlink(matrixFifo);
                unlink(tmpFifoName);
                exit(1);
            }
            
            convolution( matrix,  matConv,  2*n, kernel);
            detConv = determinant(matConv,2*k);
            result2 = detMain - detConv ;
            
            if (gettimeofday(&endConv, NULL)) {
                fprintf(stderr, "Failed to get end time\n");
                raise(SIGINT);
                close(readMatrix);
                fclose(matrixLog);
                close(tmpFifo);
                unlink(matrixFifo);
                unlink(tmpFifoName);
                exit(1);
            }
            timedifConv = 1000000L*(endConv.tv_sec - startConv.tv_sec) + endConv.tv_usec - startConv.tv_usec;
            fprintf(matrixLog,"]\n\n\nThe Convolution Matrix is  ==>>\nconv = [");
            fillLog(matConv,2*n,matrixLog);
            fprintf(matrixLog,"]");
            fflush(matrixLog);
            
            sendShowResult(tmpFifo,result2,(double)timedifConv/1000.0);
            
            close(readMatrix);
            fclose(matrixLog);
            close(tmpFifo);
            exit(1);
        }
        wait(NULL);

        usleep(100000);
        fclose(matrixLog);
        close(readMatrix);
        close(tmpFifo);
        unlink(matrixFifo);
        unlink(tmpFifoName);
    }
    if (kill(serverPid,0) != -1)
        kill(serverPid,SIGINT);
    
    

    
    
    
    return (EXIT_SUCCESS);
}

void sendShowResult(int tmpFifo,double result,double timedifInverse)
{
    write(tmpFifo,&result,sizeof(double));
    write(tmpFifo,&timedifInverse,sizeof(double));
}

void fillNxNMatrix(double matrix[20][20],double matInv[10][20],int whichRegion,int n)
{
    int i,j,k,l;
    switch (whichRegion){
        case 1:
            for(i = 0;i<n;++i)
                for(j=0;j<n;++j)
                    matInv[i][j] = matrix[i][j];
            break;
        case 2:
            for(i = 0;i<n;++i)
                for(j=n,l=0;j<2*n;++j,++l)
                    matInv[i][l] = matrix[i][j];
            break;
        case 3:
            for(i = n,l=0;i<2*n;++i,++l)
                for(j=0;j<n;++j)
                    matInv[l][j] = matrix[i][j];
            break;
        case 4:
            for(i = n,k=0;i<2*n;++i,++k)
                for(j=n,l=0;j<2*n;++j,++l)
                    matInv[k][l] = matrix[i][j];
            break;
        default:
            fprintf(stderr,"Error !\n");
            raise(SIGINT);
    }
}

void fillLog(double matrix[20][20],int n,FILE* matrixLog)
{
    int i,j;
    for (i = 0;i<n;++i){
        for (j=0;j<n;++j)
            fprintf(matrixLog,"%.4lf ",matrix[i][j]);
        fprintf(matrixLog,"; ");
    }
}


/*
 * convolution hesaplamada http://www.songho.ca/dsp/convolution/convolution.html#convolution_2d
 * sitesinden yararlanılmıştır.
 */
void convolution(double in[20][20], double out[20][20], int size,double kernel[3][3])
{
    int i, j, m, n, mm, nn,kSize = 3;
    int kCenterIndex;                         // center index of kernel 
    int ii, jj;
// find center position of kernel (half of kernel size)
    kCenterIndex = kSize / 2;

    for(i=0; i < size; ++i)              // rows
    {
        for(j=0; j < size; ++j)          // columns
        {
            for(m=0; m < kSize; ++m)     // kernel rows
            {
                mm = kSize - 1 - m;      // row index of flipped kernel

                for(n=0; n < kSize; ++n) // kernel columns
                {
                    nn = kSize - 1 - n;  // column index of flipped kernel

                    // index of input signal, used for checking boundary
                    ii = i + (m - kCenterIndex);
                    jj = j + (n - kCenterIndex);

                    // ignore input samples which are out of bound
                    if( ii >= 0 && ii < size && jj >= 0 && jj < size )
                        out[i][j] += in[ii][jj] * kernel[mm][nn];
                }
            }
        }
    }
}


/*
 * inverse işlemlerinde http://www.ccodechamp.com/c-program-to-find-inverse-of-matrix/
 * sitesinden yararlanılmıştır.
 */

double determinant(double a[20][20],double k)
{
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
          det=det + s * (a[0][c] * determinant(b,k-1));
          s=-1 * s;
          }
    }
 
    return (det);
}


void cofactor(double num[10][20],double f)
{
 double b[10][20],fac[10][20];
 int p,q,m,n,i,j;
 for (q=0;q<f;q++)
 {
   for (p=0;p<f;p++)
    {
     m=0;
     n=0;
     for (i=0;i<f;i++)
     {
       for (j=0;j<f;j++)
        {
          if (i != q && j != p)
          {
            b[m][n]=num[i][j];
            if (n<(f-2))
             n++;
            else
             {
               n=0;
               m++;
               }
            }
        }
      }
      fac[q][p]=pow(-1,q + p) * determinant(b,f-1);
    }
  }
  transpose(num,fac,f);
}

void transpose(double num[10][20],double fac[10][20],double r)
{
  int i,j;
  double b[20][20],d;
 
  for (i=0;i<r;i++)
    {
     for (j=0;j<r;j++)
       {
         b[i][j]=fac[j][i];
        }
    }
  d=determinant(num,r);
  for (i=0;i<r;i++)
    {
     for (j=0;j<r;j++)
       {
        num[i][j]=b[i][j] / d;
        }
    }
}