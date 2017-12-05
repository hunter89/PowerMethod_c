#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "PowerMethod.h"

int computeCovariance(timeSeries *timeSeries);
int implementPower(timeSeries *timseSeries);
int returnSeries(timeSeries *timeSeries);
int principalComponents(timeSeries *timeSeries);

int eigenCompute(timeSeries *timeSeries)
{
    int retcode = 0;
    
    // calculates the returns and mean_returns from the time series
    retcode = returnSeries(timeSeries);

    // computes the covariance matrix for the return_series
    retcode = computeCovariance(timeSeries);

    // implements the power method on the covariance matrix to give eigen values
    retcode = implementPower(timeSeries);

    // computes the Principal Component decomposition of the covariance matrix based on its highest eigen values
    retcode = principalComponents(timeSeries);

    BACK:
        return retcode;    
}
    
int returnSeries(timeSeries *timeSeries)
{
    int retcode = 0;
    printf("Inside return Series\n");
    int n = timeSeries->num_assets;
    int k = timeSeries->days;
    timeSeries->return_series = (double *)calloc(n*k, sizeof(double));
    timeSeries->mean_return = (double *)calloc(n, sizeof(double));
    for (int i = 0; i < n; i ++){
        //printf("inside first loop\n");
        double sum_returns = 0;
        for (int j = 0; j < k; j++){
            if (j == k - 1){
                timeSeries->return_series[k*i + j]  = sum_returns/(k-1);
            }
            else{
                timeSeries->return_series[k*i + j] = (timeSeries->prices[k*i + j + 1]/timeSeries->prices[k*i + j] - 1);
            }
            //printf("%f\n", timeSeries->return_series[k*i + j]);
            sum_returns += timeSeries->return_series[k*i + j];
        }
        timeSeries->mean_return[i] = timeSeries->return_series[k*i + k - 1];
        //printf("%f\n", timeSeries->mean_return[i]);
    }
    printf("Return Series constructed and Mean returns calculated\n");
    BACK:
    return retcode;
}

int computeCovariance(timeSeries *timeSeries)
{
    int retcode = 0;
    printf("Inside computeCovariance\n");
    int n = timeSeries->num_assets;
    int k = timeSeries->days;
    //int k = 30;
    
    timeSeries->covariance = (double *)calloc(n*n, sizeof(double));
    for (int i = 0; i < n; i++){
        double var = 0;
        for (int j = 0; j < k; j++){
            var += pow((timeSeries->return_series[k*i + j] - timeSeries->mean_return[i]),2);
        }
        timeSeries->covariance[n*i + i] = var/(k-1);
        for (int j = i + 1; j < n; j++){
            double sum = 0;
            for (int m = 0; m < k; m++){
                sum += (timeSeries->return_series[k*i + m] - timeSeries->mean_return[i])*(timeSeries->return_series[k*j + m] - timeSeries->mean_return[j]);
            }
            timeSeries->covariance[n*i + j] = sum/(k-1);
            timeSeries->covariance[n*j + i] = sum/(k-1);
        }
    }
    //printf("Covariance (0,0) is %f\n", timeSeries->covariance[0]);
    //printf("Covariance (1,1) is %f\n", timeSeries->covariance[251]);

    BACK:
    return retcode;
}

int implementPower(timeSeries *timeSeries)
{
    int retcode = 0, num_eigen_values;
    num_eigen_values = 15;
    timeSeries->num_eigen = num_eigen_values;
    srand(time(0));
    printf("Implementing Power Method for eigen value computation\n");
    int n = timeSeries->num_assets;
    int k = timeSeries->days;
    //int k = 30;
    double *w_0, *w, *w_next, *diff, *cov, magnitude_0,magnitude, magnitude_next, tolerance, *temp;
    timeSeries->eigen_values = (double *)calloc(num_eigen_values, sizeof(double));
    timeSeries->eigen_vectors = (double *)calloc(num_eigen_values*n, sizeof(double));
    timeSeries->eigen_vectors_transpose = (double *)calloc(n*num_eigen_values, sizeof(double));
    cov = (double *)calloc(n*n, sizeof(double));
    w_0 = (double *)calloc(n, sizeof(double));
    w = (double *)calloc(n, sizeof(double));
    w_next = (double *)calloc(n, sizeof(double));
    temp = (double *)calloc(n, sizeof(double));
    diff = (double *)calloc(n, sizeof(double));
    tolerance = 0.000005;
    magnitude = 0;
    for (int i = 0; i < n; i++){
        w_0[i] = (double)rand()/RAND_MAX;
        w[i] = w_0[i];
        magnitude += w[i]*w[i];
        //printf("w%d is %f\n", i, w[i]); 
    }
    magnitude = sqrt(magnitude);
    magnitude_0 = magnitude;
    
    for (int a = 0; a < num_eigen_values; a++){
        for (int b = 0; b < 1000; b++){
            magnitude_next = 0;
            int count = 0;
            for (int i = 0; i < n; i++){
                w_next[i] = 0;
                for (int j = 0; j < n; j++){
                    if (a == 0 && b == 0){
                        cov[n*i + j] = timeSeries->covariance[n*i + j];
                    }
                    if (a > 0 && b == 0){
                        cov[n*i + j] = cov[n*i + j] - timeSeries->eigen_values[a-1]*timeSeries->eigen_vectors[(a-1)*n + i]*timeSeries->eigen_vectors[(a-1)*n + j];
                    }
                    w_next[i] += cov[n*i + j]*w[j]/magnitude;
                }
                magnitude_next += w_next[i]*w_next[i];
                
                //printf("w_next%d is %f\n", i, w_next[i]);
                
                diff[i] = fabs(w[i] - w_next[i]);
                w[i] = w_next[i];
                //printf("diff%d is %f\n", i, diff[i]);
                if (diff[i] <= tolerance) count++;   
            }
            magnitude_next = sqrt(magnitude_next);
            magnitude = magnitude_next;

/*
            int count = 0;
            for (int i = 0; i < n; i++){
                diff[i] = fabs(temp[i]/magnitude - w_next[i]/magnitude_next);
                
                //printf("diff%d is %f\n", i, diff[i]);
                if (diff[i] <= tolerance){
                    count++;
                }
                else break;
            }

*/
            //printf("count is %d\n", count);
            
            if (count == n){
                printf("count is %d\n", b + 1);
                double sum = 0, product = 0;
                printf("Eigen vector number %d is --\n", a+1);                
                for (int c = 0; c < n; c++){
                    timeSeries->eigen_vectors[a*n + c] = w_next[c]/magnitude_next;
                    timeSeries->eigen_vectors_transpose[c*num_eigen_values + a] = w_next[c]/magnitude_next;
                    product += timeSeries->eigen_vectors[a*n + c]*w_0[c];
                    sum += cov[c]*w_next[c];
                    if (c == 0){
                        printf("[ %f  ,  ", timeSeries->eigen_vectors[a*n + c]);
                    }
                    if (c == 1) {
                        printf("%f  , ...", timeSeries->eigen_vectors[a*n + c]);
                    }
                    if (c == n-1){
                        printf(", %f  ]\n", timeSeries->eigen_vectors[a*n + c]); 
                    }
                }
                //printf("\nProduct is %f\n", product);
                timeSeries->eigen_values[a] = magnitude_next;
                magnitude = 0;
                for (int c = 0; c < n; c++){
                    w[c] = w_0[c] - product*timeSeries->eigen_vectors[a*n + c];
                    magnitude += w[c]*w[c];
/*
                    for (int d = 0; d <= c; d++){
                        cov[n*c + d] = cov[n*c + d] - timeSeries->eigen_values[a]*timeSeries->eigen_vectors[a*n + c]*timeSeries->eigen_vectors[a*n + d];
                        cov[n*d + c] = cov[n*c + d];
                    }
*/
                }
                magnitude = sqrt(magnitude);
                printf("eigen value number %d is %f\n", a+1, timeSeries->eigen_values[a]);
                //printf("magnitude is %f\n", magnitude);
                break;
            }  
        }
    }
    BACK:
    return retcode;
}

int principalComponents(timeSeries *timeSeries){
    
    /* Principal component decompostion of Q is obtained based on
    Q = transpose(V)*FV + D, where V is the rxn matrix of r eigen vectors,
    F is a rxr diagonal matrix whose elements are the top r eigen values,
    and D is a diagonal matrix obtained by Q - transpose(V)*FV.
    F and V are known as computed by the power method.
    The diagonal matrix d is also called the sigma matrix of the errors.
    */

    int retcode = 0;
    int n = timeSeries->num_assets;
    timeSeries->sigma = (double *)calloc(n, sizeof(double));
    printf("\nDiagonal Matrix D is --\n");
    for (int i = 0; i < n; i ++){
        double sum_squares = 0;
        for (int j = 0; j < timeSeries->num_eigen; j++){
            sum_squares += timeSeries->eigen_values[j]*timeSeries->eigen_vectors_transpose[i*timeSeries->num_eigen + j]*timeSeries->eigen_vectors_transpose[i*timeSeries->num_eigen + j];
        }
        timeSeries->sigma[i] = timeSeries->covariance[i*n + i] - sum_squares;
        //printf("%lf ", sum_squares);
        
        if (i == 0){
            printf("[ %lf  , ", timeSeries->sigma[i]);
            //printf("[ %lf  , ", sum_squares);
        }
        if (i == 2) {
            printf("%lf  , ...", timeSeries->sigma[i]);
            //printf("%lf  , ...", sum_squares);
        }
        if (i == n -1) {
            printf(" , %lf  ]\n", timeSeries->sigma[i]);
            //printf(" , %lf  ]\n", sum_squares);
        }
        
    }
    BACK:
    return retcode;
}