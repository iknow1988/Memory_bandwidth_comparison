#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "fcyc2.h" /* measurement routines */
#include "clock.h" /* routines to access the cycle counter */

#define MINBYTES (1 << 11)  /* Working set size ranges from 2 KB */
#define MAXBYTES (1 << 26)  /* ... up to 64 MB */
#define MAXSTRIDE 64        /* Strides range from 1 to 64 elems */
#define MAXELEMS MAXBYTES/sizeof(double) 
#define CORES 2

double data[MAXELEMS];

typedef struct {
  double *array;
  int elems;
  double sum;
  int stride;
} threadInfo;

typedef struct {
  double *array;
  int elems;
  double sum;
  int stride;
  double avg;
  double deviation;
} threadInfoStep2;

void init_data(double *data, int n);
void testSingleThread(int elems, int stride);
double run(int size, int stride, double Mhz, int choice);


void *sumThread( void *ud ) {
  threadInfo *ti = (threadInfo *)ud;
  int i;

  for(i=0;i<ti->elems;i += ti->stride) {
    ti->sum += ti->array[i];
  }
}

void *step2( void *ud ) {
  threadInfoStep2 *ti = (threadInfoStep2 *)ud;
  int i;
  double variance, std_deviation, sum = 0, sum1 = 0;

  for(i=0;i<ti->elems;i += ti->stride) {
    ti->sum += ti->array[i];
  }
  
  /*  Compute the sum of all elements */
    for (i = 0; i < ti->elems; i += ti->stride)
    {
        sum = sum + ti->array[i];
    }
    ti->avg = sum / (double)ti->elems;
	
	/*  Compute  variance  and standard deviation  */
    for (i = 0; i < ti->elems; i += ti->stride)
    {
        sum1 = sum1 + pow((ti->array[i] - ti->avg), 2);
    }
    variance = sum1 / (double)ti->elems;
    ti->deviation = sqrt(variance);
}

int uniform_distribution(int rangeLow, int rangeHigh) {
    double myRand = rand()/(1.0 + RAND_MAX); 
    int range = rangeHigh - rangeLow + 1;
    int myRand_scaled = (myRand * range) + rangeLow;
    return myRand_scaled;
}

void testStep2SingleThread(int elems, int stride){
	int i;
	double average, variance, std_deviation, sum = 0, sum1 = 0;
	volatile double avg, dev=0.0;
	double result = 0.0; 
    volatile double sink; 

    for (i = 0; i < elems; i += stride) {
		result += data[i];  
    }
	
    sink = result; /* So compiler doesn't optimize away the loop */
	
	/*  Compute the sum of all elements */
    for (i = 0; i < elems; i += stride)
    {
        sum = sum + data[i];
    }
    average = sum / (float)elems;
	avg = (double) average;
    /*  Compute  variance  and standard deviation  */
    for (i = 0; i < elems; i += stride)
    {
        sum1 = sum1 + pow((data[i] - average), 2);
    }
    variance = sum1 / (float)elems;
    std_deviation = sqrt(variance);
	dev = (double) std_deviation;
}
int main()
{
    int size;
    int stride;
    double Mhz;
	int choice=1;
    //init_data(data, MAXELEMS); /* Initialize each element in data */
    Mhz = mhz(0);
	printf("Please enter your choice\n");
	scanf("%d",&choice);
    printf("Clock frequency is approx. %.1f MHz\n", Mhz);
    printf("Memory mountain (MB/sec)\n");

    printf("\t");
	
    for (stride = 1; stride <= MAXSTRIDE; stride=stride*2)
	printf("s%d\t", stride);
    printf("\n");

    for (size = MAXBYTES; size >= MINBYTES; size >>= 1) {

		if (size > (1 << 20))
			printf("%dm\t", size / (1 << 20));
		else
			printf("%dk\t", size / 1024);

		for (stride = 1; stride <= MAXSTRIDE; stride=stride*2) {
			init_rand_data(data, MAXELEMS);
			printf("%.1f\t", run(size, stride, Mhz, choice));
	    
		}
		printf("\n");
    }
	
    exit(0);
}

void init_data(double *data, int n)
{
    int i;

    for (i = 0; i < n; i++)
	data[i] = i;
}

void init_rand_data(double *data, int n)
{
    int i;

    for (i = 0; i < n; i++)
	{
		data[i] = uniform_distribution(0,100000000);
	}
}

void testSingleThread(int elems, int stride) /* The testSingleThread function */
{
    int i;
    double result = 0.0; 
    volatile double sink; 

    for (i = 0; i < elems; i += stride) {
		result += data[i];  
    }
	
    sink = result; /* So compiler doesn't optimize away the loop */

}

void testStep2MultiThread(int size, int stride)
{
	threadInfoStep2 ti[CORES];
	pthread_t thread[CORES];
	int i=0;
	
	for(i=0;i<CORES;i++)
	{
		ti[i].elems = size/CORES;
		ti[i].array = &( data[i*(size/CORES)] );
		ti[i].sum = 0.0;
		ti[i].deviation = 0.0;
		ti[i].avg = 0.0;
		ti[i].stride=stride;
	}

	for(i=0;i<CORES;i++) {
		pthread_create(&(thread[i]), NULL, step2, &(ti[i]) );
	}

	for(i=0;i<CORES;i++) {
		pthread_join( thread[i], NULL );
	}
}

void testMultiThread(int size, int stride)
{
	threadInfo ti[CORES];
	pthread_t thread[CORES];
	int i=0;
	
	for(i=0;i<CORES;i++)
	{
		ti[i].elems = size/CORES;
		ti[i].array = &( data[i*(size/CORES)] );
		ti[i].sum = 0.0;
		ti[i].stride=stride;
	}

	for(i=0;i<CORES;i++) {
		pthread_create(&(thread[i]), NULL, sumThread, &(ti[i]) );
	}

	for(i=0;i<CORES;i++) {
		pthread_join( thread[i], NULL );
	}
}

double run(int size, int stride, double Mhz, int choice)
{   
    double cycles;
    int elems = size / sizeof(double);   
	if(choice==1){
		testSingleThread(elems, stride);
		cycles = fcyc2(testSingleThread, elems, stride, 0);
		return (size/stride) / (cycles / Mhz);
	}
	else if(choice==2){
		testMultiThread(elems, stride);
		cycles = fcyc2(testMultiThread, elems, stride, 0);
		return (size/stride) / (cycles / Mhz);
	}	
	else if(choice==3){
		testStep2SingleThread(elems, stride);
		cycles = fcyc2(testStep2SingleThread, elems, stride, 0);
		return (3*(size/stride)) / (cycles / Mhz);
	}	
	else if(choice==4){
		testStep2MultiThread(elems, stride);
		cycles = fcyc2(testStep2MultiThread, elems, stride, 0);
		return (3*(size/stride)) / (cycles / Mhz);
	}	
	else {
		testSingleThread(elems, stride);
		cycles = fcyc2(testSingleThread, elems, stride, 0);
		return (size/stride) / (cycles / Mhz);
	}
}


