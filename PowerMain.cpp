#include <stdio.h>
#include <stdlib.h>
#include "PowerMethod.h"

int main(int argc, char **argv)
{
    int retcode = 0;
    int max_names;
    timeSeries *myTimeSeries;
    myTimeSeries = (struct timeSeries *)calloc(1, sizeof(struct timeSeries));
    FILE *covariance_matrix, *returns_file;
    int file2 = 0; // to indicate whether second file is present
    const char *covarianceFilename = "cov_mat.csv";
    const char *returnsFilename = "returns_file.csv";
    
    if ((argc < 2 ) || (argc > 4)){
	  printf("usage: PowerMethod filename [max_names] [filename2]\n");  retcode = 1;
	  goto BACK;
    }

    if (argc == 2) max_names = 0;
    if (argc == 3) {
        max_names = atoi(argv[2]);
        printf("Max names in portofolio are %d\n", max_names);
    }
    if (argc == 4) {
        max_names = atoi(argv[2]);
        file2 = 1;
    }
    retcode = readit(argv[1], myTimeSeries);
    if (retcode) goto BACK;

    retcode = eigenCompute(myTimeSeries);
    if (retcode) goto BACK;

    retcode = optimizer(myTimeSeries, max_names);
    if (retcode) goto BACK;

    free(myTimeSeries);

    if(file2){
        myTimeSeries = (struct timeSeries *)calloc(1, sizeof(struct timeSeries));
        
        retcode = readit(argv[3], myTimeSeries);
        if (retcode) goto BACK;

        retcode = eigenCompute(myTimeSeries);
        if (retcode) goto BACK;

        retcode = longshort(myTimeSeries);
        if (retcode) goto BACK;

        free(myTimeSeries);
    }


    /* following code for printing returns and covariance matrix to a file
    
    returns_file = fopen(returnsFilename, "w");
    
    for (int i = 0; i < myTimeSeries->num_assets; i ++){
        for (int j = 0; j < myTimeSeries->days; j++){
            if (j == 0) {
                fprintf(returns_file, "%f",myTimeSeries->return_series[myTimeSeries->days*i + j]);
            }
            else{
                fprintf(returns_file, ",%f",myTimeSeries->return_series[myTimeSeries->days*i + j]);
            }
        }
        fprintf(returns_file, "\n");
    }
    fclose(returns_file);
    covariance_matrix = fopen(covarianceFilename, "w");

    for (int i = 0; i < myTimeSeries->num_assets; i ++){
        for (int j = 0; j < myTimeSeries->num_assets; j++){
            if (j == 0) {
                fprintf(covariance_matrix, "%f",myTimeSeries->covariance[myTimeSeries->num_assets*i + j]);
            }
            else{
                fprintf(covariance_matrix, ",%f",myTimeSeries->covariance[myTimeSeries->num_assets*i + j]);
            }
        }
        fprintf(covariance_matrix, "\n");
    }
    
    fclose(covariance_matrix);
    */
    BACK:
    
    return retcode;
}