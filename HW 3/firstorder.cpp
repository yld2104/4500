#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

typedef struct bag{
	int n;
	double *x;
	double *lb;
	double *ub;
	double *mu;
	double *covariance;
	double *gradient;
	double lambda;
	int num_iter;
	int iter_counter;
	double *y;
	double s;
	FILE *debug;
};

int stepdirection(struct bag *pmybag);
int stepsize(struct bag *pmybag);
int move(struct bag *pmybag);
int gradient(struct bag *pmybag);

double calcObjective(struct bag *pmybag);
double minobj(struct bag *pmybag, double *y);
int isfeasible(struct bag *pmybag, double *y);
void quickSort(double arr[], int index[], int low, int high);
double partition (double arr[], int index[], int low, int high);
void swap(double* a, double* b, int* c, int*d);
void printarray(double *y, int n);
void printarrayint(int *y, int n);

int readit(char *nameoffile, int *addressofn, double **, double **, double **, double *, double **);

int algo(int n, double *x, double *lb, double *ub, double *mu, double *covariance, double lambda, int num_iter);
int improve(struct bag *pmybag);
int feasible(int n, double *x, double *lb, double *ub);

int main(int argc, char **argv)
{
  int retcode = 0;
  int n, num_iter;
  double *lb, *ub, *covariance, *mu, lambda, *x;

  if (argc != 3){
	  printf("usage: qp filename num_iter\n");  retcode = 1;
	  goto BACK;
  }

  num_iter = atoi(argv[2]);
  retcode = readit(argv[1], &n, &lb, &ub, &mu, &lambda, &covariance);
  if (retcode) goto BACK;

  x = (double *)calloc(n, sizeof(double));
  if (x == NULL){
	  printf(" no memory for x\n"); retcode = 1; goto BACK;
  }

  retcode = algo(n, x, lb, ub, mu, covariance, lambda, num_iter); 
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


int algo(int n, double *x, double *lb, double *ub, double *mu, double *covariance, double lambda, int num_iter)
{
	int returncode = 0, i;
	double *gradient = NULL;
	struct bag *pmybag = NULL;
	FILE *output;

	printf("\n running algorithm\n");

	returncode = feasible(n, x, lb, ub);
	if (returncode){
		goto BACK;
	}

	gradient = (double *)calloc(n, sizeof(double));
	if (!gradient){
		returncode = 100;
		goto BACK;
	}

	pmybag = (struct bag *)calloc(1, sizeof(struct bag));
	if (!pmybag){
		returncode = 100; goto BACK;
	}

	pmybag->n = n;
	pmybag->covariance = covariance;
	pmybag->gradient = gradient;
	pmybag->lb = lb;
	pmybag->ub = ub;
	pmybag->mu = mu;
	pmybag->x = x;
	pmybag->lambda = lambda;
	pmybag->y = NULL;
	pmybag->num_iter = num_iter;
	pmybag->s = 0;

	pmybag->debug = fopen("debug.dat", "w");
	if (!pmybag->debug){
		printf("could not open debug file\n");
		returncode = 200; goto BACK;
	}


	returncode = improve(pmybag);

	output = fopen("outfirstorder.dat", "w");
	if (!output){
		printf("could not open output file\n");
		returncode = 200; goto BACK;
	}
	printf("Optimal Objective Achieved: %f \n", calcObjective(pmybag));
	fprintf(output, "Optimal Objective Achieved: %f \n", calcObjective(pmybag));
	for(i = 0; i<n; i++){
		fprintf(output, "%d: %f\n", i, pmybag->x[i]);
	}

BACK:
	return returncode;
}

int improve(struct bag *pmybag)
{
	int counter, returncode = 0, returncode2 = 0;

	for (counter = 0; counter < pmybag->num_iter; counter++){
		fprintf(pmybag->debug, "iteration # %d\n", counter);
		printf("iteration # %d\n", counter);

		pmybag->iter_counter = counter;

		/** compute gradient **/
		returncode = gradient(pmybag);
		if (returncode) goto BACK;

		//printarray(pmybag->gradient,pmybag->n);
		/** compute step direction **/
		returncode2 = stepdirection(pmybag);
		if (returncode2){
			printf("No more feasible y found \n");
			goto BACK;
		}
		printf("y vector: \n");
		printarray(pmybag->y,pmybag->n);

		/** compute stepsize **/
		returncode = stepsize(pmybag);
		if (returncode) goto BACK;

		/** move in the descent direction by stepsize **/
		returncode = move(pmybag);
		if (returncode) goto BACK;

		printf("Obj %d: %f \n", counter, calcObjective(pmybag));
		fprintf(pmybag->debug, "done with iteration # %d\n\n", counter);
		printf("done with iteration # %d\n\n", counter);
	}
	BACK:
	return returncode;
}

int gradient(struct bag *pmybag)
{
	int returncode = 0, j, n = pmybag->n, i;
	double lambda = pmybag->lambda, first, second, third;

	fprintf(pmybag->debug, "gradient iteration # %d\n", pmybag->iter_counter);
	printf("gradient iteration # %d\n", pmybag->iter_counter);

	for (j = 0; j < n; j++){
		first = 0;  
		first= 2 * lambda*pmybag->x[j] * pmybag->covariance[j*n + j];

		second = 0;
		for (i = 0; i < n; i++)if (i != j){
			 second += pmybag->covariance[j*n + i]*pmybag->x[i];
		}
		second *= 2 * lambda;


		third = -pmybag->mu[j];

		pmybag->gradient[j] = first + second + third;
		// first = 0 ;
		// for (i=0; i<n; i++){
		// 	first += pmybag->covariance[j*n+i]*pmybag->x[i];
		// }
		// pmybag->gradient[j] = lambda*first - pmybag->mu[j];

	}

	return returncode;
}

int stepdirection(struct bag *pmybag)
{
	int returncode = 1, n = pmybag -> n, i, m, j, ind;
	double sum, min, obj;
	int *index = NULL;
	double *y = NULL, *ub = pmybag->ub, *lb = pmybag->lb, *x = pmybag->x, *yfinal = NULL;

	double *copygradient = (double *)calloc(n, sizeof(double));
	for(i = 0; i < n; i++){
		copygradient[i] = pmybag->gradient[i];
	}

	/** sort gradient **/
	index = (int *)calloc(n, sizeof(int));
	for(i = 0; i<n; i++){
		index[i] = i;
	}
	quickSort(copygradient,index,0,n-1);
	yfinal = (double*)calloc(n, sizeof(double));
	y = (double*)calloc(n, sizeof(double));
	//printf("Gradient, Ordered Gradient, Ordered Indexes - \n");
	//printarray(pmybag->gradient,n);
	//printarray(copygradient,n);
	//printarrayint(index,n);
	
	/** perform enumeration **/
	min = DBL_MAX;
	for (m = 0; m<n; m++){
		sum = 0;
		for(j = 0; j < m; j++){
			ind = index[j];
			y[ind] = lb[ind] - x[ind];
			sum += y[ind];
		}

		for(j=n-1; j > m; j--){
			ind = index[j];
			y[ind] = ub[ind] - x[ind];
			sum += y[ind];
		}

		y[index[m]] = 0 - sum;
		//printarray(y,n);

		ind = index[m];
		/*if(isfeasible(pmybag, y) == 0){
			obj = minobj(pmybag, y);
			if (obj < min){
				min = obj;
				for(i=0; i<n; i++){
					yfinal[i]=y[i];
				}
				returncode = 0;
			}
			printf("minobj: %f \n", obj);
		}*/

		if(x[ind]+y[ind] >= lb[ind]-pow(10,-12) && x[ind]+y[ind] <= ub[ind]+pow(10,-12)){
			obj = minobj(pmybag, y);
			if (obj < min){
				min = obj;
				for(i=0; i<n; i++){
					yfinal[i]=y[i];
				}
				returncode = 0;
			}
			printf("minobj: %f \n", obj);
		}else{
			printf("%d out of bounds: %f, %f - %f \n",m, x[ind]+y[ind], lb[ind], ub[ind]);
		}
	}
	pmybag->y = yfinal;
	return returncode;
}

void printarray(double *y, int n){
	int i;
	printf("Array: ");
	for(i = 0; i<n; i++){
		printf("%f ",y[i]);
	}
	printf("\n");
}
void printarrayint(int *y, int n){
	int i;
	printf("Array: ");
	for(i = 0; i<n; i++){
		printf("%d ",y[i]);
	}
	printf("\n");
}

double minobj(struct bag *pmybag, double *y){
	int n = pmybag -> n, i;
	double sum = 0;

	for( i=0; i<n; i++){
		sum += pmybag->gradient[i]*y[i];
	}

	return sum;
}

/* Used to be used to check that y vector provides a feasible solution but there's a double roundoff error so no longer used */
int isfeasible(struct bag *pmybag, double *y){
	int i, n=pmybag->n;
	double xy, sum=0;

	for(i = 0; i<n; i++){
		xy = pmybag->x[i] + y[i];
		sum += xy;
		if(xy < pmybag->lb[i]){
			printf("%d lower out of bounds: %f, %f %f \n",i, xy, pmybag->lb[i], xy-pmybag->lb[i]);
			return 1;
		}else if (xy > pmybag->ub[i]){
			printf("%d upper out of bounds: %f, %f \n",i, xy, pmybag->ub[i]);
			return 1;
		}
	}

	return 0;
}

int stepsize(struct bag *pmybag)
{
	int returncode = 0, i, j, n = pmybag->n;
	double s, summu, sumcov, sumy;

	summu = 0;
	for(i=0; i<n; i++){
		summu += pmybag->y[i]*pmybag->mu[i];
	}

	sumcov = 0;
	for(i=0; i<n; i++){
		for(j = 0; j<n; j++){
			sumcov += pmybag->covariance[i*n+j]*(pmybag->x[j]*pmybag->y[i]+pmybag->x[i]*pmybag->y[j]);
		}
	}

	sumy = 0;
	for(i=0; i<n; i++){
		for(j=0; j<n; j++){
			sumy += pmybag->covariance[i*n+j]*pmybag->y[i]*pmybag->y[j];
			//printf("%f %f, ", sumy, pmybag->y[i]*pmybag->y[j]);
		}
	}

	s = (summu - pmybag->lambda*sumcov)/(2*pmybag->lambda*sumy);
	//printf("s: %f ",s);
	if (s > 1){
		s = 1;
	}else if (s < 0){
		s = 0;
	}
	printf("summu %f sumcov %f sumy %f s: %f \n", summu, sumcov, sumy, s);
	pmybag->s = s;
	return returncode;
}

int move(struct bag *pmybag)
{
	int returncode = 0, n = pmybag->n, i;
	double s = pmybag->s, sy;

	for(i=0; i<n; i++){
			sy = pmybag->x[i] + s*pmybag->y[i];
			if (sy < 0 && -1*sy < pow(10,-12)){
				sy = 0;
			}
			pmybag->x[i] = sy;
	}

	return returncode;
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

/* C implementation QuickSort from  geeks for geeks tweaked */
 
// A utility function to swap two elements
void swap(double* a, double* b, int* c, int*d)
{
    double t = *a;
    *a = *b;
    *b = t;
    int y = *c;
    *c = *d;
    *d = y;
}
 
/* This function takes last element as pivot, places
   the pivot element at its correct position in sorted
    array, and places all smaller (smaller than pivot)
   to left of pivot and all greater elements to right
   of pivot */
double partition (double arr[], int index[], int low, int high)
{
    double pivot = arr[high];    // pivot
    int i = (low - 1);  // Index of smaller element
 
    for (int j = low; j <= high- 1; j++)
    {
        // If current element is smaller than or
        // equal to pivot
        if (arr[j] >= pivot)
        {
            i++;    // increment index of smaller element
            swap(&arr[i], &arr[j], &index[i], &index[j]);
        }
    }
    swap(&arr[i + 1], &arr[high], &index[i+1], &index[high]);
    return (i + 1);
}
 
/* The main function that implements QuickSort
 arr[] --> Array to be sorted,
 index[] --> keeps track of index of sorted array,
  low  --> Starting index,
  high  --> Ending index */
void quickSort(double arr[], int index[], int low, int high)
{
    if (low < high)
    {
        /* pi is partitioning index, arr[p] is now
           at right place */
        double pi = partition(arr, index, low, high);
 
        // Separately sort elements before
        // partition and after partition
        quickSort(arr, index, low, pi - 1);
        quickSort(arr, index, pi + 1, high);
    }
}