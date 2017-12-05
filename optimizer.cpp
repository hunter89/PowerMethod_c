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


    // number of variables are num_assets + num of factor variables i.e. number if eigen values
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
    for (j = 0; j < timeSeries->num_assets; j++){
        
        if (x[j] > 0){
            count++;
            fprintf(optimized_portfolio, "%s = %g\n", names[j], x[j]);
        }
    }
    printf("count is %d\n", count);

    

    BACK:
    GRBfreeenv(env);
    printf("exiting with retcode %d\n", retcode);
    return retcode;
}