#include <stdio.h>
#include <stdlib.h>

#include "PowerMethod.h"
#include "gurobi_c++.h"


int optimizer(timeSeries *timeSeries, int max_names){
    int retcode = 0;
    GRBenv   *env = NULL;
    GRBmodel *model = NULL;
    int n, i, j;
    double *obj = NULL;
    double *lb  = NULL;
    double *ub  = NULL;
    double *x;
    char *vartype;
    int *qrow, *qcol, Nq;
    double *qval;
    int *cind;
    double rhs;
    char sense;
    double *cval;
    int numnonz;
    char *constraint_string;
    double tolerance = 0.0001;
    double lambda;
    int count = 0;
    //int max_names = 0;
    FILE *optimized_portfolio;
    char **names;


    // number of variables are 2*num_assets + num of factor variables i.e. number of eigen values
    n = 2*timeSeries->num_assets + timeSeries->num_eigen;
    timeSeries->lambda = 50;
    lambda = timeSeries->lambda;

    retcode = GRBloadenv(&env, "factormodel.log");
    if (retcode) goto BACK;

    /* Create initial model */
    retcode = GRBnewmodel(env, &model, "second", n, NULL, NULL, NULL, NULL, NULL);
    if (retcode) goto BACK;

    // allocate array for storing names of variables
    names = (char **) calloc(n, sizeof(char *));

    obj = (double *)calloc(n, sizeof(double));
    ub = (double *)calloc(n, sizeof(double));
    lb = (double *)calloc(n, sizeof(double));
    x = (double *)calloc(n, sizeof(double));
    vartype = (char *)calloc(n, sizeof(char));

    for (j = 0; j < timeSeries->num_assets; j++){
        names[j] = (char *)calloc(5, sizeof(char));
        if (names[j] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j], "x%d", j);
        obj[j] = -1*timeSeries->mean_return[j];
        ub[j] = 0.02;
        //lb[j] = 0.0000005;
    }

    for (j = 0; j < timeSeries->num_eigen; j++){
        names[j + timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j+ timeSeries->num_assets], "y%d", j);
        ub[j + timeSeries->num_assets] = 100;
        lb[j + timeSeries->num_assets] = -100;
    }

    for (j = 0; j < timeSeries->num_assets; j++){
        names[j + timeSeries->num_assets + timeSeries->num_eigen] = (char *)calloc(5, sizeof(char));
        if (names[j + timeSeries->num_assets + timeSeries->num_eigen] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j + timeSeries->num_assets + timeSeries->num_eigen], "z%d", j);
        ub[j + timeSeries->num_assets + timeSeries->num_eigen] = 1;
    }
    for(j = 0; j < n; j++){
        retcode = GRBsetstrattrelement(model, "VarName", j, names[j]);
        if (retcode) goto BACK;

        retcode = GRBsetdblattrelement(model, "Obj", j, obj[j]);
        if (retcode) goto BACK;

        retcode = GRBsetdblattrelement(model, "LB", j, lb[j]);
        if (retcode) goto BACK;

        retcode = GRBsetdblattrelement(model, "UB", j, ub[j]);
        if (retcode) goto BACK;

        if (j < timeSeries->num_assets + timeSeries->num_eigen) vartype[j] = GRB_CONTINUOUS;
        else vartype[j] = GRB_BINARY;
        
        retcode = GRBsetcharattrelement(model, "VTYPE", j, vartype[j]);
        if (retcode) goto BACK;
    }

    Nq = timeSeries->num_assets + timeSeries->num_eigen;
    qrow = (int *) calloc(Nq, sizeof(int));  /** row indices **/
    qcol = (int *) calloc(Nq, sizeof(int));  /** column indices **/
    qval = (double *) calloc(Nq, sizeof(double));  /** values **/

    for (j = 0; j < Nq; j++){
        qrow[j] = j;
        qcol[j] = j;
        // (j,j) indicates only square terms 
        if (j < timeSeries->num_assets){
            qval[j] = lambda*timeSeries->sigma[j];
        }
        else {
            qval[j] = lambda*timeSeries->eigen_values[j - timeSeries->num_assets];
        }
    }

    retcode = GRBaddqpterms(model, Nq, qrow, qcol, qval);
    if (retcode) goto BACK;

    numnonz = timeSeries->num_assets + 1;
    rhs = 0;
    sense = GRB_EQUAL;
    for (i = 0; i < timeSeries->num_eigen; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind = (int *)calloc(timeSeries->num_assets + 1, sizeof(int));
        cval = (double *)calloc(timeSeries->num_assets + 1, sizeof(double));
        for (j = 0; j < timeSeries->num_assets; j++){
            cind[j] = j;
            cval[j] = timeSeries->eigen_vectors[i*timeSeries->num_assets + j];
        }
        cind[j] = i + j;
        cval[j] = -1.0;
        sprintf(constraint_string, "c%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
        if (retcode) goto BACK;
    }
    numnonz = timeSeries->num_assets;
    rhs = 1.0;
    sense = GRB_EQUAL;
    for (i = 0; i < timeSeries->num_assets; i++){
        //cind = (int *)calloc(timeSeries->num_assets, sizeof(int));
        //cval = (double *)calloc(timeSeries->num_assets, sizeof(double));
        cind[i] = i;
        cval[i] = 1.0;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "convexity1");
    if (retcode) goto BACK;
    
    if (max_names != 0){
        numnonz = timeSeries->num_assets;
        rhs = max_names;
        sense = GRB_LESS_EQUAL;
        for (i = 0; i < timeSeries->num_assets; i++){
            //cind = (int *)calloc(timeSeries->num_assets, sizeof(int));
            //cval = (double *)calloc(timeSeries->num_assets, sizeof(double));
            cind[i] = i + timeSeries->num_assets + timeSeries->num_eigen;
            cval[i] = 1.0;
        }
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "convexity2");
        if (retcode) goto BACK;

        numnonz = 2;
        rhs = 0.0;
        sense = GRB_LESS_EQUAL;
        
        for (i = 0; i < timeSeries->num_assets; i++){
            cind[0] = i + timeSeries->num_assets + timeSeries->num_eigen;
            cval[0] = tolerance;
            cind[1] = i;
            cval[1] = -1.0;
            sprintf(constraint_string, "control-lb%d", i);
            retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
            if (retcode) goto BACK; 
        }
        
        for (i = 0; i < timeSeries->num_assets; i++){
            cind[0] = i;
            cval[0] = 1.0;
            cind[1] = i + timeSeries->num_assets + timeSeries->num_eigen;
            cval[1] = -1;
            sprintf(constraint_string, "control-ub%d", i);
            retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
            if (retcode) goto BACK;
        }
    }
    
    
    retcode = GRBupdatemodel(model);
    if (retcode) goto BACK;

    retcode = GRBwrite(model, "PCA-factor.lp");
    if (retcode) goto BACK;

    retcode = GRBoptimize(model);
    if (retcode) goto BACK;

    retcode = GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, n, x);
    if (retcode) goto BACK;

    optimized_portfolio = fopen("portfolio.txt", "w");
    fprintf(optimized_portfolio, "%d\n", timeSeries->num_assets);
    for (j = 0; j < timeSeries->num_assets; j++){
        
        fprintf(optimized_portfolio, "%s = %g\n", names[j], x[j]);
        if (x[j] > 0){
            count++;            
        }
    }
    printf("count is %d\n", count);

    

    BACK:
    fclose(optimized_portfolio);
    GRBfreeenv(env);
    printf("exiting with retcode %d\n", retcode);
    return retcode;
}

int longshort(timeSeries *timeSeries){
    int retcode = 0;
    double *positions;
    const char *portfolioFile = "portfolio.txt";
    GRBenv   *env = NULL;
    GRBmodel *model = NULL;
    int n, i, j;
    double *obj = NULL;
    double *lb  = NULL;
    double *ub  = NULL;
    double *x;
    char *vartype;
    int *qrow, *qcol, Nq;
    double *qval;
    int *cind;
    double rhs;
    char sense;
    double *cval;
    int numnonz;
    char *constraint_string;
    double tolerance = 0.0001;
    double lambda;
    int count = 0;
    char **names;
    FILE *long_short_portfolio;
    double turnover, sum_long, sum_short;

    positions = (double *)calloc(timeSeries->num_assets, sizeof(double));
    retcode = readportfolio(portfolioFile, positions);
    if (retcode) goto BACK;

    // number of variables are 6*num_assets (x, p, m, t, w, z) + num of factor variables i.e. number of eigen values
    n = 6*timeSeries->num_assets + timeSeries->num_eigen;
    timeSeries->lambda = 50;
    lambda = timeSeries->lambda;

    retcode = GRBloadenv(&env, "factormodel.log");
    if (retcode) goto BACK;

    /* Create initial model */
    retcode = GRBnewmodel(env, &model, "second", n, NULL, NULL, NULL, NULL, NULL);
    if (retcode) goto BACK;

    // allocate array for storing names of variables
    names = (char **) calloc(n, sizeof(char *));

    obj = (double *)calloc(n, sizeof(double));
    ub = (double *)calloc(n, sizeof(double));
    lb = (double *)calloc(n, sizeof(double));
    x = (double *)calloc(n, sizeof(double));
    vartype = (char *)calloc(n, sizeof(char));

    // x variable defining the portfolio weights 
    for (j = 0; j < timeSeries->num_assets; j++){
        names[j] = (char *)calloc(5, sizeof(char));
        if (names[j] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j], "x%d", j);
        obj[j] = -1*timeSeries->mean_return[j];
        ub[j] = 0.03;
        lb[j] = -0.03;
    }
    
    // p variable defining the addition w.r.t original portfolio weights
    for (j = 0; j < timeSeries->num_assets; j++){
        names[j + timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j + timeSeries->num_assets], "p%d", j);
        ub[j + timeSeries->num_assets] = 0.03;
        lb[j + timeSeries->num_assets] = 0;
    }

    // t variable defining the turnover for the asset
    for (j = 0; j < timeSeries->num_assets; j++){
        names[j + 2*timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + 2*timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j + 2*timeSeries->num_assets], "t%d", j);
        ub[j + 2*timeSeries->num_assets] = positions[j];
        lb[j + 2*timeSeries->num_assets] = 0;
    }

    // m variable defining the negative weights assigned to an asset
    for (j = 0; j < timeSeries->num_assets; j++){
        names[j + 3*timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + 3*timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j + 3*timeSeries->num_assets], "m%d", j);
        ub[j + 3*timeSeries->num_assets] = 0.03;
        lb[j + 3*timeSeries->num_assets] = 0;
    }

    // w variable defining if weight is added to the original weight
    for (j = 0; j < timeSeries->num_assets; j++){
        names[j + 4*timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + 4*timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j + 4*timeSeries->num_assets], "w%d", j);
        ub[j + 4*timeSeries->num_assets] = 1;
        lb[j + 4*timeSeries->num_assets] = 0;
    }

    // z variable defining if weight is negative on the asset
    for (j = 0; j < timeSeries->num_assets; j++){
        names[j + 5*timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + 5*timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j + 5*timeSeries->num_assets], "z%d", j);
        ub[j + 5*timeSeries->num_assets] = 1;
        lb[j + 5*timeSeries->num_assets] = 0;
    }


    // y variables introduced because of the "factor" matrix 
    for (j = 0; j < timeSeries->num_eigen; j++){
        names[j + 6*timeSeries->num_assets] = (char *)calloc(5, sizeof(char));
        if (names[j + 6*timeSeries->num_assets] == NULL){
            retcode = 1;
            goto BACK;
        }
        sprintf(names[j+ 6*timeSeries->num_assets], "y%d", j);
        ub[j + 6*timeSeries->num_assets] = 100;
        lb[j + 6*timeSeries->num_assets] = -100;
    }

    for(j = 0; j < n; j++){
        retcode = GRBsetstrattrelement(model, "VarName", j, names[j]);
        if (retcode) goto BACK;

        retcode = GRBsetdblattrelement(model, "Obj", j, obj[j]);
        if (retcode) goto BACK;

        retcode = GRBsetdblattrelement(model, "LB", j, lb[j]);
        if (retcode) goto BACK;

        retcode = GRBsetdblattrelement(model, "UB", j, ub[j]);
        if (retcode) goto BACK;

        if (j < 4*timeSeries->num_assets || j >= 6*timeSeries->num_assets) vartype[j] = GRB_CONTINUOUS;
        else vartype[j] = GRB_BINARY;
        
        retcode = GRBsetcharattrelement(model, "VTYPE", j, vartype[j]);
        if (retcode) goto BACK;
    }

    Nq = timeSeries->num_assets + timeSeries->num_eigen;
    qrow = (int *) calloc(Nq, sizeof(int));  /** row indices **/
    qcol = (int *) calloc(Nq, sizeof(int));  /** column indices **/
    qval = (double *) calloc(Nq, sizeof(double));  /** values **/

    for (j = 0; j < Nq; j++){
        if (j < timeSeries->num_assets){
            qrow[j] = j;
            qcol[j] = j;
        }
        else{
            qrow[j] = j + 5*timeSeries->num_assets;
            qcol[j] = j + 5*timeSeries->num_assets;
        }
        // (j,j) indicates only square terms 
        if (j < timeSeries->num_assets){
            qval[j] = lambda*timeSeries->sigma[j];
        }
        else {
            qval[j] = lambda*timeSeries->eigen_values[j - timeSeries->num_assets];
        }
    }

    retcode = GRBaddqpterms(model, Nq, qrow, qcol, qval);
    if (retcode) goto BACK;

    // adding constraint for y - Vx = 0
    numnonz = timeSeries->num_assets + 1;
    rhs = 0;
    sense = GRB_EQUAL;
    for (i = 0; i < timeSeries->num_eigen; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind = (int *)calloc(timeSeries->num_assets + 1, sizeof(int));
        cval = (double *)calloc(timeSeries->num_assets + 1, sizeof(double));
        for (j = 0; j < timeSeries->num_assets; j++){
            cind[j] = j;
            cval[j] = timeSeries->eigen_vectors[i*timeSeries->num_assets + j];
        }
        cind[j] = i + j*6;
        cval[j] = -1.0;
        sprintf(constraint_string, "c%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
        if (retcode) goto BACK;
    }    
    
    // adding bal constraint for equation x - p + t + m = x_start
    numnonz = 4;    
    sense = GRB_EQUAL;
    for (i = 0; i < timeSeries->num_assets; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind = (int *)calloc(4, sizeof(int));
        cval = (double *)calloc(4, sizeof(double));
        for (j = 0; j < 4; j++){
            cind[j] = j*timeSeries->num_assets + i;
            if (j == 1){
                cval[j] = -1;
            }
            else cval[j] = 1; 
        }
        rhs = positions[i];
        sprintf(constraint_string, "bal%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
        if (retcode) goto BACK;
    }    
    
    // Max turnover constraint
    numnonz = timeSeries->num_assets;
    sense = GRB_LESS_EQUAL;
    rhs = 0.2;
    cind = (int *)calloc(timeSeries->num_assets, sizeof(int));
    cval = (double *)calloc(timeSeries->num_assets, sizeof(double));
    for(j = 0; j < timeSeries->num_assets; j++){
        cind[j] = j + 2*timeSeries->num_assets;
        cval[j] = 1;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "MaxTurnover");

    numnonz = timeSeries->num_assets;
    sense = GRB_EQUAL;
    rhs = 1;
    cind = (int *)calloc(timeSeries->num_assets, sizeof(int));
    cval = (double *)calloc(timeSeries->num_assets, sizeof(double));
    for(j = 0; j < timeSeries->num_assets; j++){
        cind[j] = j;
        cval[j] = 1;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "Convexity");

/*
    // constraint for sum of long positions less or equal to 0.6
    numnonz = 2*timeSeries->num_assets;
    sense = GRB_LESS_EQUAL;
    rhs = 100;
    cind = (int *)calloc(2*timeSeries->num_assets, sizeof(int));
    cval = (double *)calloc(2*timeSeries->num_assets, sizeof(double));
    for(j = 0; j < timeSeries->num_assets; j++){
        cind[2*j] = j + timeSeries->num_assets;
        cind[2*j + 1] = j + 2*timeSeries->num_assets;
        cval[2*j] = 1;
        cval[2*j + 1 ] = -1;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "Long-upper");

    // constraint for sum of long positions greater or equal 0.4
    numnonz = 2*timeSeries->num_assets;
    sense = GRB_GREATER_EQUAL;
    rhs = - 100;
    cind = (int *)calloc(2*timeSeries->num_assets, sizeof(int));
    cval = (double *)calloc(2*timeSeries->num_assets, sizeof(double));
    for(j = 0; j < timeSeries->num_assets; j++){
        cind[2*j] = j + timeSeries->num_assets;
        cind[2*j + 1] = j + 2*timeSeries->num_assets;
        cval[2*j] = 1;
        cval[2*j + 1 ] = -1;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "Long-lower");
*/
    // constraint for sum of short positions less or equal to 0.6
    numnonz = timeSeries->num_assets;
    sense = GRB_LESS_EQUAL;
    rhs = 0.6;
    cind = (int *)calloc(timeSeries->num_assets, sizeof(int));
    cval = (double *)calloc(timeSeries->num_assets, sizeof(double));
    for (j = 0; j < timeSeries->num_assets; j++){
        cind[j] = j + 3*timeSeries->num_assets;
        cval[j] = 1;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "Short-upper");
   
    // constraint for sum of short positions greater or equal 0.4
    numnonz = timeSeries->num_assets;
    sense = GRB_GREATER_EQUAL;
    rhs = 0.4;
    cind = (int *)calloc(timeSeries->num_assets, sizeof(int));
    cval = (double *)calloc(timeSeries->num_assets, sizeof(double));
    for (j = 0; j < timeSeries->num_assets; j++){
        cind[j] = j + 3*timeSeries->num_assets;
        cval[j] = 1;
    }
    retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "Short-lower");

    // Integer variable w constraint1
    numnonz = 2;
    sense = GRB_LESS_EQUAL;
    rhs = 0;
    cind = (int *)calloc(2, sizeof(int));
    cval = (double *)calloc(2, sizeof(double));
    for (i = 0; i < timeSeries->num_assets; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind[0] = i + timeSeries->num_assets;
        cind[1] = i + 4*timeSeries->num_assets;
        cval[0] = 1;
        cval[1] = -ub[i + timeSeries->num_assets];
        sprintf(constraint_string, "w1_%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
    }

    // Integer variable w constraint2
    numnonz = 2;
    sense = GRB_LESS_EQUAL;
    rhs = 0.03;
    cind = (int *)calloc(2, sizeof(int));
    cval = (double *)calloc(2, sizeof(double));
    for (i = 0; i < timeSeries->num_assets; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind[0] = i + 3*timeSeries->num_assets;
        cind[1] = i + 4*timeSeries->num_assets;
        cval[0] = 1;
        cval[1] = ub[i + 3*timeSeries->num_assets];
        sprintf(constraint_string, "w2_%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
    }

    // Integer variable z constraint1
    numnonz = 2;
    sense = GRB_GREATER_EQUAL;
    rhs = 0;
    cind = (int *)calloc(2, sizeof(int));
    cval = (double *)calloc(2, sizeof(double));
    for (i = 0; i < timeSeries->num_assets; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind[0] = i + 2*timeSeries->num_assets;
        cind[1] = i + 5*timeSeries->num_assets;
        cval[0] = 1;
        cval[1] = -positions[i];
        sprintf(constraint_string, "z1_%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
    }

    // Integer variable z constraint2
    numnonz = 2;
    sense = GRB_LESS_EQUAL;
    rhs = 0;
    cind = (int *)calloc(2, sizeof(int));
    cval = (double *)calloc(2, sizeof(double));
    for (i = 0; i < timeSeries->num_assets; i++){
        constraint_string = (char *)calloc(15, sizeof(char));
        cind[0] = i + 3*timeSeries->num_assets;
        cind[1] = i + 5*timeSeries->num_assets;
        cval[0] = 1;
        cval[1] = -ub[i + 3*timeSeries->num_assets];
        sprintf(constraint_string, "z2_%d", i);
        retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, constraint_string);
    }

    retcode = GRBupdatemodel(model);
    if (retcode) goto BACK;

    retcode = GRBwrite(model, "PCA-factor-long-short.lp");
    if (retcode) goto BACK;

    retcode = GRBoptimize(model);
    if (retcode) goto BACK;

    retcode = GRBgetdblattrarray(model, GRB_DBL_ATTR_X, 0, n, x);
    if (retcode) goto BACK;

    long_short_portfolio = fopen("long_short_portfolio.txt", "w");
    fprintf(long_short_portfolio, "%d\n", timeSeries->num_assets);
    turnover = 0;
    sum_long = 0;
    sum_short = 0;
    for (j = 0; j < timeSeries->num_assets; j++){
        turnover += x[j + 2*timeSeries->num_assets];
        sum_long += x[j + timeSeries->num_assets] - x[j + 2*timeSeries->num_assets] + positions[j];
        sum_short += x[j + 3*timeSeries->num_assets];
    }

    fprintf(long_short_portfolio, "Total long value is %lf\n", sum_long);
    fprintf(long_short_portfolio, "Total short value is %lf\n", sum_short);
    fprintf(long_short_portfolio, "Total turnover is %lf\n", turnover);
    for (j = 0; j < timeSeries->num_assets; j++){
        fprintf(long_short_portfolio, "%s = %g\n", names[j], x[j]);
        if (x[j] > 0){
            count++;            
        }
    }
    BACK:
    fclose(long_short_portfolio);
    GRBfreeenv(env);
    printf("exiting optimizer with retcode %d\n", retcode);
    return retcode;
}