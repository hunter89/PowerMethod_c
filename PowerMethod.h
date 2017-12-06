typedef struct timeSeries{
    int num_assets, days;
    double *prices, *return_series, *mean_return, *covariance, *eigen_values, *eigen_vectors, *eigen_vectors_transpose, *sigma;
    int num_eigen;
    double lambda;
}timeSeries;

int readit(char *filename, timeSeries *timeSeries);
int eigenCompute(timeSeries *timeSeries);
int optimizer(timeSeries *timeSeries, int max_names);
int longshort(timeSeries *timeSeries);
int readportfolio(const char *portfolioFile, double *positions);