#include <stdio.h>
#include <stdlib.h>


#include "gurobi_c++.h"

int main(void)
{
  int retcode = 0;
  GRBenv   *env = NULL;
  GRBmodel *model = NULL;
  int n, j;
  double *obj      = NULL;
  double *lb       = NULL;
  double *ub       = NULL;
  double *x;
  int *qrow, *qcol, Nq;
  double *qval;
  int *cind;
  double rhs;
  char sense;
  double *cval;
  int numnonz;

  char **names;


  n = 9; /** 7 'x' variables, 2 factor variables **/


  retcode = GRBloadenv(&env, "factormodel.log");
  if (retcode) goto BACK;

 /* Create initial model */
  retcode = GRBnewmodel(env, &model, "second", n, 
                      NULL, NULL, NULL, NULL, NULL);
  if (retcode) goto BACK;

  names = (char **) calloc(n, sizeof(char *));

  /** next we create the remaining attributes for the n columns **/
  obj     = (double *) calloc (n, sizeof(double));
  ub     = (double *) calloc (n, sizeof(double));
  lb     = (double *) calloc (n, sizeof(double));
  x     = (double *) calloc (n, sizeof(double));


  for(j = 0; j < 7; j++){
    names[j] = (char *)calloc(3, sizeof(char));
    if(names[j] == NULL){
      retcode = 1; goto BACK;
    }
    sprintf(names[j],"x%d", j);
  }
  for(j = 7; j < 9; j++){
    names[j] = (char *)calloc(3, sizeof(char));
    if(names[j] == NULL){
		  retcode = 1; goto BACK;
    }
    sprintf(names[j],"y%d", j - 7);
  }
  obj[0] = -.233; obj[1] = -3.422; obj[2] = -.1904; obj[3] = -.5411;
  obj[4] = -.045; obj[5] = -1.271; obj[6] = -0.955;
  /** calloc initializes memory to zero, so all other obj[j] are zero **/

  /**next, the upper bounds on the x variables **/
  ub[0] = 0.6; ub[1] = 0.8; ub[2] = 0.8; ub[3] = 0.5;
  ub[4] = 0.5; ub[5] = 0.26; ub[6] = 0.99;
  
  /** the upper bounds on the two factor variables -- we make them large **/
  ub[7] = 100; ub[8] = 100;
  /** the lower bounds on the factor variables **/
  lb[7] = -100; lb[8] = -100;

  /* initialize variables */
  for(j = 0; j < n; j++){
    retcode = GRBsetstrattrelement(model, "VarName", j, names[j]);
    if (retcode) goto BACK;

    retcode = GRBsetdblattrelement(model, "Obj", j, obj[j]);
    if (retcode) goto BACK;

    retcode = GRBsetdblattrelement(model, "LB", j, lb[j]);
    if (retcode) goto BACK;

    retcode = GRBsetdblattrelement(model, "UB", j, ub[j]);
    if (retcode) goto BACK;
  }

  /** next, the quadratic -- there are 11 nonzeroes: 7 residual variances plus the 2x2
									factor covariance matrix**/

  Nq = 11; 
  qrow = (int *) calloc(Nq, sizeof(int));  /** row indices **/
  qcol = (int *) calloc(Nq, sizeof(int));  /** column indices **/
  qval = (double *) calloc(Nq, sizeof(double));  /** values **/

  if( ( qrow == NULL) || ( qcol == NULL) || (qval == NULL) ){
    printf("could not create quadratic\n");
    retcode = 1; goto BACK;
  }

  qval[0] = 10.0; qrow[0] = 0; qcol[0] = 0;
  qval[1] = 20.0; qrow[1] = 1; qcol[1] = 1;
  qval[2] = 30.0; qrow[2] = 2; qcol[2] = 2;
  qval[3] = 40.0; qrow[3] = 3; qcol[3] = 3;
  qval[4] = 50.0; qrow[4] = 4; qcol[4] = 4;
  qval[5] = 60.0; qrow[5] = 5; qcol[5] = 5;
  qval[6] = 70.0; qrow[6] = 6;   qcol[6] = 6;  

  qval[7] = 100.0; qrow[7] = 7; qcol[7] = 7;							
  qval[8] = 200.0; qrow[8] = 8; qcol[8] = 8;
  qval[9] = 0.1; qrow[9] = 7; qcol[9] = 8;
  qval[10] = 0.1; qrow[10] = 8; qcol[10] = 7;	 

  retcode = GRBaddqpterms(model, 11, qrow, qcol, qval);
  if (retcode) goto BACK;

  /** now we will add one constraint at a time **/
  /** we need to have a couple of auxiliary arrays **/

  cind = (int *)calloc(n, sizeof(int));  /** n is over the top since no constraint is totally dense;
					     but it's not too bad here **/
  cval= (double *)calloc(n, sizeof(double));

  /** two factor constraints, first one is next**/
  cval[0] = 1.508; cval[1] = .7802; cval[2] = 1.8796;
  cval[3] = 4.256;  cval[4] = 1.335; cval[5] = 2.026; cval[6] = 1.909;
  cval[7] = -1;

  for(j = 0; j < 7; j++) cind[j] = j;
  cind[7] = 7;

  numnonz = 8;
  rhs = 0;
  sense = GRB_EQUAL;

  retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "first_constraint");
  if (retcode) goto BACK;

  /** second factor constraint **/

  cval[0] = 4.228; cval[1] = 1.2945; cval[2] = .827;
  cval[3] = 2.149;  
  cval[4] = 2.353; cval[5] = 0.3026; cval[6] = 1.487;
  
  cval[7] = -1;
  for(j = 0; j < 7; j++) cind[j] = j; /** redundant! but let's keep it here so that we know it 
					 will be used **/
  cind[7] = 8;

  numnonz = 8;
  rhs = 0;
  sense = GRB_EQUAL;

  retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "second_constraint");
  if (retcode) goto BACK;


  /** sum of x variables = 1 **/
  cval[0] = 1.0; cval[1] = 1.0; cval[2] = 1.0;
  cval[3] = 1.0;  cval[4] = 1.0; cval[5] = 1.0; cval[6] = 1.0;

  for(j = 0; j < 7; j++) cind[j] = j;

  numnonz = 7;
  rhs = 1.0;
  sense = GRB_EQUAL;

  retcode = GRBaddconstr(model, numnonz, cind, cval, sense, rhs, "convexity");
  if (retcode) goto BACK;


  retcode = GRBupdatemodel(model);
  if (retcode) goto BACK;

  /** optional: write the problem **/

  retcode = GRBwrite(model, "factorqp.lp");
  if (retcode) goto BACK;


  retcode = GRBoptimize(model);
  if (retcode) goto BACK;


  /** get solution **/


  retcode = GRBgetdblattrarray(model,
                               GRB_DBL_ATTR_X, 0, n,
                               x);
  if(retcode) goto BACK;

  /** now let's see the values **/

  for(j = 0; j < n; j++){
    printf("%s = %g\n", names[j], x[j]);
  }

  GRBfreeenv(env);


 BACK:
  printf("\nexiting with retcode %d\n", retcode);
  return retcode;
}