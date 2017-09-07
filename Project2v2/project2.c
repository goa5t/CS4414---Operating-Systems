#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
	int count; //number of threads
	pthread_mutex_t m; // mutex
    	pthread_cond_t cv; // condition variable

} barrier_t;

    //Globals
	int * input;
	int * output;
 	size_t size = 0;
	pthread_t * allThreads;
	int threads;
	barrier_t ba;



void barrier_init(barrier_t *b, int inThreads){
	b->count = inThreads;
	    //mutex init
    	pthread_mutex_init(&b->m, NULL);
	pthread_cond_init(&b->cv, NULL);
}
void barrier_wait(barrier_t *b){
	pthread_mutex_lock(&b->m);
	if(b->count == 1) // meaning it's the last thread
		pthread_cond_broadcast(&b->cv);
	else
	{
		b->count --; // decrement the number of threads
		pthread_cond_wait(&b->cv, &b->m);
	}
	pthread_mutex_unlock(&b->m);	
}

void barrier_destroy(barrier_t *b){
	pthread_cond_destroy(&b->cv);
	pthread_mutex_destroy(&b->m);

}

//Reading functions
int * readIn(char *argv[], size_t size){

	int * ret;

	int lines=size;
    int i=0;
	int c;
    FILE *f;
    ret = malloc(sizeof(int) * lines);
    f = fopen(argv[1], "r");
    do
    {
        c = fscanf(f, "%d\n", &ret[i]);
        if(feof(f))
            break;
        i++;
        
    }while(1);
        fclose(f);
	return ret;
}

size_t getSize(char *argv[]){
    size_t lines=0;
    int c=0;
    FILE *f;
    f = fopen(argv[1], "r");
    if(f==NULL){
        printf("Error in opening file.");
        return NULL;
    }
    do
    {
        c = fgetc(f);
        if(feof(f))
            break;
        if(c=='\n')
            lines++;
    }while(1);
    fclose(f);
    return lines;
    
}

int numThreads (size_t size){
    return size / 2;
}

void printArray(int * in, size_t size){
    printf("in printArray, size = %d\n", size);
    int i=0;
    for(i=0;i<size; i++)
        printf("in[%d] = %d\n", i, in[i]);
}

// End of read/print functions
//Start of Threaded/calculation functions

void * compare (void * arg){
    	int i = 2 * (int) arg; //the starting point in the array, based on the thread #
    	int tmp = 0;
   	 if(input[i] > input[i+1])
       		tmp = input[i];
    	else
        	tmp = input[i+1];
	output[(int)arg] = tmp;
	//calling barrier_wait in order to check threads
	barrier_wait(&ba);
}
 
int main(int argc, char *argv[]){
    int rounds = 0;
    int tindex;
	
    //Read in vales from input file
    size = getSize(argv);
	input = malloc(sizeof(int) * size);
	if(size==0){
		printf("There are no numbers in the file.\n");
		exit(0);
	}
	input = readIn(argv, size);

    while(size > 1){
        	threads = numThreads(size);
        	barrier_init(&ba, threads+1);

       		allThreads = malloc(sizeof(pthread_t) * threads);
		output = malloc(sizeof(int) * threads);

        	for(tindex=0; tindex<threads; tindex++)
        		pthread_create(&allThreads, NULL, compare, (void*) tindex);

		barrier_wait(&ba);

		free(input);
		input = malloc(sizeof(int) * threads);
		memcpy(input, output, sizeof(int) * threads);
		free(output);
		//printArray(input, threads);
		size = size / 2;
		barrier_destroy(&ba);
	}
	printf("The largest number is %d.\n", input[0]);
	return 0;
}
