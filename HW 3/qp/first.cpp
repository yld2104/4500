#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "solver.h"
// declare bag
typedef struct bag{
	int n;
	double *x;
	double *lb;
	double *ub;
	double *mu;
	double *covariance;
	double *gradient;
	double *copyofgradient;
	int sortindex;
	double lambda;
	int numiterations;
	FILE *output;
	int iter_counter;
}

int readit(char *nameoffile, int *addressofn, double **, double **, double **, double **);
int algo(int n, double *x, double *lb, double *ub, double*mu, double *covariance, double *gradient, double lamda);
//int improve(int n, double *x, double *lb, double *ub, double*mu, double *covariance, double *gradient, double lamda);
int improve(struct bag pmybag);
int gradient(struct bag *pmybag);
int stepdirection(struct bag *pmybag);
int stepsize(struct bag *pmybag);
int move(struct bag *pmybag);
int feasible(int n, double *x, double *lb, double *ub);

int main(int argc, char **argv)
{
  int retcode = 0;
  int n;
  double *lb, *ub, *covariance, *mu, lambda;

  if (argc != 2){
	  printf("usage: qp1 filename\n");  retcode = 1;
	  goto BACK;
  }

  retcode = readit(argv[1], &n, &lb, &ub, &mu, &covariance);
  BACK:
  return retcode;
}
int algo(){
	int returncode = 0
	double *gradient = NULL, copyofgradient;
	struct bag *pmybag = NULL;

	printf("\n running algorithm \n");

	returncode = feasible(n, x, lb, ub);
	if (returncode != 0){
		returncode = 100; goto BACK;
	}

	gradient = (double*)calloc(2*n, sizeof(double));
	if(!gradient){
		returncode = NOMEMORY; goto BACK;
	}
	copyofgradient = gradient + n;

	pmybag = (struct bag *)calloc(1, sizeof(struct bag));
	if(!pmybag){
		returncode = NOMEMORY; goto BACK;
	}

	// Should have a function to do this
	pmybag -> n = n;
	pmybag -> covariance = covariance;
	pmybag -> gradient = gradient;
	pmybag -> copyofgradient = copyofgradient;
	pmybag -> lb = lb;
	pmybag -> ub = ub;
	pmybag -> mu = mu;
	pmybag -> x = x;
	pmybag -> lambda = lambda;
	pmybag -> sortindex = (int*)calloc(n,sizeof(int));
	if(!pmybag->sortindex){
		returncode = NOMEMORY; goto BACK;
	}
	pmybag -> numiterations = 1;
	pmybag -> output = fopen("theoutput.dat","w");
	if (!pmybag->output){
		printf("Could not open output file\n");
		returncode = CANNOTOPEN; goto BACK;
	}

	returncode = improve(struct bag *pmybag);
BACK:
	return returncode;
}
int gradient(struct bag *pmybag)
{
	int returncode = 0;j, n = pmybag->n, i;
	double lambda = pmybag->lambda, first, second, third;


	fprintf(pmybag->output, "iteration # %d\n", pmybag->iter_counter);
	printf("gradient iteration # %d\n", pmybag->iter_counter);

	for (j=0; j< n; j++){
		first = 0;
		first = 2*lambda*pmybag->x[j]*pmybag->covariance[j*n+j];
		
		second = 0;
		for (i=0; i<n; i++) if(i!=j){
			second += pmybag->covariance[j*n+i]*pmybag->x[i];
		}
		second *= 2*lambda;

		third = -pmybag->mu[j];

		pmybag->gradient[j] = first + second + third;

	}

	return returncode;
}
int stepdirection(struct bag *pmybag)
{
	int returncode = 0;
	/* Sort the gradient */

	/* perform enumeration */

	return returncode;
}
int stepsize(struct bag *pmybag)
{
	int returncode = 0;
	return returncode;
}
int move(struct bag *pmybag)
{
	int returncode = 0;
	return returncode;
}
//int improve(int n, double *x, double *lb, double *ub, double*mu, double *covariance, double *gradient, double lamda){
int improve( struct bag *pmybag){
	int counter, returncode = 0;

	for (counter = 0; counter < pmybag->numiterations; counter ++){
		fprintf(pmybag->output, "iteration # %d\n", counter);
		printf("iteration # %d\n", counter);

		pmybag -> iter_counter = counter;
		/** compute gradient **/
		returncode = gradient(pmybag);
		if(returncode) goto BACK;

		/** compute step direction **/
		returncode = stepdirection(pmybag);
		if(returncode) goto BACK;

		/** compute stepsize **/
		returncode = stepsize(pmybag);
		if(returncode) goto BACK;

		/** move in the descent direction by step size **/
		returncode = move(pmybag);
		if(returncode) goto BACK;

		fprintf(pmybag->output, "done with iteration # %d\n\n", counter);
		printf("done with iteration # %d\n\n", counter);
	}
	return returncode;
}

int feasible(int n, double *x, double *lb, double *ub){
	int returncode = 0; j;

}

int readit(char *filename, int *address_of_n, double **plb, double **pub,
		double **pmu, double **pcovariance)
{
	int readcode = 0, fscancode;
	FILE *datafile = NULL;
	char buffer[100];
	int n, i, j;
	double *lb = NULL, *ub = NULL, *mu = NULL, *covariance = NULL;

	datafile = fopen(filename, "r");
	if (!datafile){
		printf("cannot open file %s\n", filename);
		readcode = 2;  goto BACK;
	}

	printf("reading data file %s\n", filename);

	fscanf(datafile, "%s", buffer);
	fscancode = fscanf(datafile, "%s", buffer);
	if (fscancode == EOF){
		printf("problem: premature file end at ...\n");
		readcode = 4; goto BACK;
	}

	n = *address_of_n = atoi(buffer);

	printf("n = %d\n", n);

	lb = (double *)calloc(n, sizeof(double));
	*plb = lb;
	ub = (double *)calloc(n, sizeof(double));
	*pub = ub;
	mu = (double *)calloc(n, sizeof(double));
	*pmu = mu;
	covariance = (double *)calloc(n*n, sizeof(double));
	*pcovariance = covariance;

	if (!lb || !ub || !mu || !covariance){
		printf("not enough memory for lb ub mu covariance\n"); readcode = 3; goto BACK;
	}

	fscanf(datafile, "%s", buffer);

	for (j = 0; j < n; j++){
		fscanf(datafile, "%s", buffer);
		fscanf(datafile, "%s", buffer);
		lb[j] = atof(buffer);
		fscanf(datafile, "%s", buffer);
		ub[j] = atof(buffer);
		fscanf(datafile, "%s", buffer);
		mu[j] = atof(buffer);
		printf("j = %d lb = %g ub = %g mu = %g\n", j, lb[j], ub[j], mu[j]);
	}


	fscanf(datafile, "%s", buffer);
	fscanf(datafile, "%s", buffer);


	fscanf(datafile, "%s", buffer); /* reading 'covariance'*/

	for (i = 0; i < n; i++){
		for (j = 0; j < n; j++){ 
			fscanf(datafile, "%s", buffer);
			covariance[i*n + j] = atof(buffer);
		}
	}


	fscanf(datafile, "%s", buffer);
	if (strcmp(buffer, "END") != 0){
		printf("possible error in data file: 'END' missing\n");
	}


	fclose(datafile);

BACK:

	return readcode;
}
