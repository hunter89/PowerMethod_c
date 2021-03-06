#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PowerMethod.h"

// read data from text file to timeSeries structure
int readit(char *filename, timeSeries *timeSeries)
{
	int readcode = 0, fscancode;
	FILE *datafile = NULL;
	char buffer[100];
    int num_assets, days;
     

	datafile = fopen(filename, "r");
	if (!datafile){
		printf("cannot open file %s\n", filename);
		readcode = 2;  goto BACK;
	}

	printf("reading data file %s\n", filename);
	fscanf(datafile, "%s", buffer);
	fscanf(datafile, "%s", buffer);
	timeSeries->num_assets = atoi(buffer);
	fscanf(datafile, "%s", buffer);
	fscanf(datafile, "%s", buffer);
	timeSeries->days = atoi(buffer);

	timeSeries->prices = (double *)calloc(timeSeries->num_assets*timeSeries->days, sizeof(double));
	for(int i = 0; i < timeSeries->num_assets; i++){
		for (int j = 0; j < timeSeries->days; j++){
			fscancode = fscanf(datafile, "%s", buffer);
			if (fscancode == EOF){
				break;
			}
			timeSeries->prices[timeSeries->days*i + j] = atof(buffer);
		}
	}

	printf("Number of assets are %d\n", timeSeries->num_assets);
	printf("Number of days are %d\n", timeSeries->days);

	fclose(datafile);

	BACK:
		return readcode;
}

int readportfolio(const char *portfolioFile, double *positions)
{
	int readcode = 0;
	FILE *portfile = NULL;
	char buffer[100];
	int num_assets;
	portfile = fopen(portfolioFile, "r");
	if (!portfile){
		printf("can't open file %s\n", portfolioFile);
		readcode = 2;
		goto BACK;
	}

	fscanf(portfile, "%s", buffer);
	num_assets = atoi(buffer);
	for (int i = 0; i < num_assets; i++)
	{
		fscanf(portfile, "%s", buffer);
		fscanf(portfile, "%s", buffer);
		fscanf(portfile, "%s", buffer);
		positions[i] = atof(buffer);
		//printf("%f\n", positions[i]);
	}

	BACK:
	return readcode;
}