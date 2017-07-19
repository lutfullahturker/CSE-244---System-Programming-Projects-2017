/* 
 * File:   matrixOperations.h
 * Author: Lütfullah TÜRKER
 *
 * Created on 29 May 2017, 150:30
 */

#ifndef MATRIXOPERATIONS_H
#define MATRIXOPERATIONS_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "defs_and_types.h"

#define NR_END 1
#define FREE_ARG char*

typedef struct {
	int m, n;
	double ** v;
} mat_t, *mat;


double determinant(double matrix[][50],int n);
void inverseMatrix(double matrix[][50],double inversed[][50],int n);
void matrixMult(double resultMatrix[][50], double matrix1[][50], double matrix2[][50],int m,int p,int p2);
void transpose (double matrix[][50],double transposedMatrix[][50],int m, int p);

void pseudoInverse(double  a[][50],double b[][50],double x[][50],int m,int p);

/*
 * https://rosettacode.org/wiki/QR_decomposition#C
 * sitesindeki kod kullanılarak yapılmıştır.
 */
mat matrix_new(int m, int n);
void matrix_delete(mat m);
void matrix_transpose(mat m);
mat matrix_copy(int n, double a[][n], int m);
mat matrix_mul(mat x, mat y);
mat matrix_minor(mat x, int d);
double *vmadd(double a[], double b[], double s, double c[], int n);
mat vmul(double v[], int n);
double vnorm(double x[], int n);
double* vdiv(double x[], double d, double y[], int n);
double* mcol(mat m, double *v, int c);
void matrix_show(mat m);
void householder(mat m, mat *R, mat *Q);

void QRFactorization(double Q[][50],double R[][50],double B[][50],double xQR[][50],int m,int p);


void svdlapack(double **a, int row, int col, double **u, double **s, double **v);
void printmatrix(char *var, double **a, int row, int col);
double *array1_create(int len);
double **array2_create(int row, int col);
void array1_free(double *arr);
void array2_free(double **arr);


double *dvector(long nl, long nh);
void free_dvector(double *v, long nl, long nh);


int dsvd(double a[50][50], int m, int n, double w[50], double v[50][50]);
void solveSvd(double u[50][50], double w[50], double v[50][50], int m,int n, double b[50], double x[50]);

#endif /* MATRIXOPERATIONS_H */

