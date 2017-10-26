#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm> 

typedef struct bag{
	int n;
	double *x;
	double *lb;
	double *ub;
	double *mu;
	double *covariance;
	double lambda;
	int num_iter;
	int xh;
	int xk;
	int iter_counter;
	FILE *output;
};


double calcObjective(struct bag *pmybag);
int readit(char *nameoffile, int *addressofn, double **, double **, double **, double *, double **);

int algo(int n, double *x, double *lb, double *ub, double *mu, double *covariance, double lambda, int limit);
int improve(struct bag *pmybag);
int feasible(int n, double *x, double *lb, double *ub);

int main(int argc, char **argv)
{
  int retcode = 0;
  int limit;
  int n;
  double *lb, *ub, *covariance, *mu, lambda, *x;

  if (argc != 3){
	  printf("usage: qp filename limit\n");  retcode = 1;
	  goto BACK;
  }
  limit = atoi(argv[2]);

  retcode = readit(argv[1], &n, &lb, &ub, &mu, &lambda, &covariance);
  if (retcode) goto BACK;

  x = (double *)calloc(n, sizeof(double));
  if (x == NULL){
	  printf(" no memory for x\n"); retcode = 1; goto BACK;
  }

  retcode = algo(n, x, lb, ub, mu, covariance, lambda, limit);


  BACK:
  return retcode;
}

int readit(char *filename, int *address_of_n, double **address_of_lb, double **pub,
		double **pmu, double *plambda, double **pcovariance)
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
	*address_of_lb = lb;
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
	*plambda = atof(buffer);

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


int algo(int n, double *x, double *lb, double *ub, double *mu, double *covariance, double lambda, int limit)
{
	int returncode = 0, i, j, count;
	double *gradient = NULL;
	struct bag *pmybag = NULL;

	printf("\n running algorithm\n");

	returncode = feasible(n, x, lb, ub);
	if (returncode){
		goto BACK;
	}

	

	pmybag = (struct bag *)calloc(1, sizeof(struct bag));
	if (!pmybag){
		returncode = 100; goto BACK;
	}

	pmybag->n = n;
	pmybag->covariance = covariance;
	pmybag->lb = lb;
	pmybag->ub = ub;
	pmybag->mu = mu;
	pmybag->x = x;
	pmybag->lambda = lambda;
	pmybag->num_iter = 0;


	pmybag->output = fopen("outsimplegd.dat", "w");
	if (!pmybag->output){
		printf("could not open output file\n");
		returncode = 200; goto BACK;
	}

	count = 0;
	while(count <= limit){
		for(i = 0; i<n; i++){
			for(j=0; j<n; j++){
				if(i != j){
					count ++;
					pmybag->xh = i;
					pmybag->xk = j;
					returncode = improve(pmybag);
					if (returncode) goto BACK;
				}
			}
		}
	}

	printf("Optimal Objective Achieved: %f \n", calcObjective(pmybag));
	fprintf(pmybag->output, "Optimal Objective Achieved: %f \n", calcObjective(pmybag));
	for(i = 0; i<n; i++){
		fprintf(pmybag->output, "%d: %f\n", i, pmybag->x[i]);
	}

BACK:
	return returncode;
}

int improve(struct bag *pmybag)
{
	int h = pmybag->xh, k = pmybag->xk, returncode = 0, i, j, n = pmybag->n;
	double lambda = pmybag->lambda, a, b, sum, obj0, e, emin, emax;
	double *covariance = pmybag->covariance, *x = pmybag->x;
	a = lambda*(covariance[h*n+h]+covariance[k*n+k]-2*covariance[h*n+k]);
	sum = 0;
	for(i = 0; i<n; i++){
		if (i != h && i != k){
			sum += (covariance[i*n+k] - covariance[i*n+h])*x[i];
		}
	}
	b = 2*lambda*(-covariance[h*n+h]*x[h]+covariance[k*n+k]*x[k]+covariance[h*n+k]*(x[h]-x[k]) + sum) + pmybag->mu[h]-pmybag->mu[k];

	obj0 = calcObjective(pmybag);

	if(a == 0){
		goto BACK;
	}
	e = -b/(2*a);
	
	//emax = max(min(x[h]-pmybag->lb[h],pmybag->ub[k]-x[k]),0);
	//emin = max(min(pmybag->ub[h]-x[h],x[k]-pmybag->lb[k]),0);
	//printf("lb: %f ub: %f x: %f %f test %f %f \n",pmybag->lb[h],pmybag->ub[k],x[h],x[k], x[h]-pmybag->lb[h], pmybag->ub[k]-x[k]);
	if (x[h]-pmybag->lb[h] <= pmybag->ub[k]-x[k]){
		emax = x[h]-pmybag->lb[h];
	}else{
		emax = pmybag->ub[k]-x[k];
	}

	if (emax < 0){
		emax = 0;
	}

	if (pmybag->ub[h]-x[h] <= x[k]-pmybag->lb[k]){
		emin = pmybag->ub[h]-x[h];
	}else{
		emin = x[k]-pmybag->lb[k];
	}

	if (emin < 0){
		emin = 0;
	}
	

	if(e < -1*emin){
		e = -1*emin;
	}else if (e > emax){
		e = emax;
	}

	//printf("indexes: %d and %d \n", h, k);
	//printf("objective %d: %f b: %f a: %f emin: %f emax: %f \n",pmybag->num_iter,obj0, b, a, emin, emax);

	pmybag->x[h] = x[h]-e;
	pmybag->x[k] = x[k]+e;

	pmybag->num_iter = pmybag->num_iter + 1;

	BACK:
	return returncode;
}

double calcObjective(struct bag *pmybag){
	int i, j, n = pmybag->n;
	double lambda = pmybag->lambda, sum, obj, sumcov, summean;
	double *covariance = pmybag->covariance, *x = pmybag->x;

	sumcov = 0;
	for(i = 0; i<n; i++){
		for(j = 0; j<n; j++){
			sumcov += covariance[i*n+j]*x[i]*x[j];
		}
	}
	summean = 0;
	for(i = 0; i<n; i++){
		summean += pmybag->mu[i]*x[i];
	}
	obj = lambda*sumcov - summean;

	return obj;
}
int feasible(int n, double *x, double *lb, double *ub)
{
	int returncode = 0, j;
	double sum, increase;

	printf(" computing first feasible solution\n");

	sum = 0;
	/* set variables to lower bounds */
	for (j = 0; j < n; j++){
		x[j] = lb[j];
		sum += lb[j];
	}
	printf("after first pass, sum = %g\n", sum);
	for (j = 0; j < n; j++){
		increase = 1.0 - sum;
		if (increase > ub[j] - x[j])
			increase = ub[j] - x[j];
		x[j] += increase;
		sum += increase;
		printf("after j = %d, sum = %g\n", j, sum);
		if (sum >= 1.0)
			break;
	}

	return returncode;
}