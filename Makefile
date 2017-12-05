CC=clang++
CFLAGS=-c -std=c++11
CLIB=-L/Library/gurobi751/mac64/lib/ -lgurobi75 -lgurobi_c++
INC=-I/Library/gurobi751/mac64/include/

all: PowerMethod

PowerMethod: PowerMain.o readit.o eigenCompute.o optimizer.o
		$(CC) $(CLIB) PowerMain.o readit.o eigenCompute.o optimizer.o -o PowerMethod

PowerMain.o: PowerMain.cpp
		$(CC) $(CFLAGS) PowerMain.cpp

readit.o: readit.cpp
		$(CC) $(CFLAGS) readit.cpp

eigenCompute.o: eigenCompute.cpp
		$(CC) $(CFLAGS) eigenCompute.cpp

optimizer.o: optimizer.cpp
		$(CC) $(CFLAGS) $(INC) optimizer.cpp


clean:
		rm -rf PowerMethod *.o
