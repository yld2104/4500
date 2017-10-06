#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int generatehist(char *histfilename, int iterations, double *portfolio, double *p, int numscen, int numsec, int status)
{
 	int retcode = 0;  
	int i,j,k;  
	FILE *out = NULL;
	double *perturb;
	int *bins;
	double sum;
	int count;
	double perturbvalue;
	
	perturb = (double *)calloc((1 + numscen)*(1 + numsec), sizeof(double));
	if (perturb == NULL){
		printf("no memory\n"); retcode = 400; goto BACK;
	}
	bins = (int *)calloc((1+numscen), sizeof(int));
	for (j = 0; j <= numscen; j++){
	  bins[j] = 0; 
	  //printf("%d \n", bins[j]);
	}  	
	for (i = 0; i < iterations; i++){

		for (k = 1; k <= numscen; k++){
			for (j = 0; j <= numsec; j++){
				//printf("%f",((double)rand()/(double)RAND_MAX)/10);
				if (j == 0){
					perturb[k*(1 + numsec) + j] = 1;
				}else{
					perturbvalue = 0.95 + ((double)rand()/(double)RAND_MAX)/10;
					perturb[k*(1 + numsec) + j] = p[k*(1 + numsec) + j]*perturbvalue;
				}
			}
		}

	  	count = 0;
		for (k = 1; k <= numscen; k++){
			sum = 0;
			for (j = 0; j <= numsec; j++){
				sum = sum + perturb[k*(1 + numsec) + j]*portfolio[j];
				//printf ("pert: %f \n", perturb[k*(1 + numsec) + j]);
			}
			//printf("sum: %f \n", sum);
			if (sum < 0) {
				count = count + 1;
			}
			
		}
		//printf("%d ",count);
		//printf("%d \n", bins[0]);
		bins[count] = bins[count] + 1;

	}
	

	out = fopen(histfilename, "w");
    if (!out){
	  printf("cannot open final output file %s for writing\n", histfilename); retcode = 400; 
	  goto BACK;
    }
  /* read until finding Optimal */

	fprintf(out, "status, %d\n", status);
	if (status != 0){
		fclose(out); goto BACK;
	}
	fprintf(out, "Histogram Bins (Number of Scenerios < 0):,Number\n");
	for (j = 0; j <= numscen; j++){
	  fprintf(out, "%d,%d\n", j,bins[j]); 
	}

	free(perturb); 
	free(bins);
	fclose(out);
   
BACK:
  return retcode;
}