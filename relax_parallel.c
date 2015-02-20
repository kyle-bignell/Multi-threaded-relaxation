#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

// Stores the data required for a thread
struct thread_data
{
	double** left_array;
	double** right_array;
	int* direction;
	int* complete;
	pthread_barrier_t* serial_complete;
	pthread_barrier_t* parallel_complete;
	int dimension;
	int lower_x;
	int upper_x;
	int lower_y;
	int upper_y;
};

void print_array(struct thread_data* data)
{	
	double** result;
	if (*data->direction == -1)
	{
		result = data->left_array;
	}
	else
	{
		result = data->right_array;
	}
	
	int outer;
	int inner;
	for (outer = 0; outer < data->dimension; outer++)
	{
		for (inner = 0; inner < data->dimension; inner++)
		{
			printf("%f ", result[outer][inner]);
		}
		printf("\n");
	}
}

void recordTime(struct timeval* time)
{
	gettimeofday(time, NULL);
}

double calculateTimePassed(struct timeval start, struct timeval end)
{
	double time = 0.0;
	
	time += (double) (end.tv_sec - start.tv_sec);
	time += ((double) (end.tv_usec - start.tv_usec)) / 1000000;
	
	return time;
}

// Calculates the relaxed values for cells in an array
void* relax(void* args)
{
	struct thread_data* data = (struct thread_data*) args;
	double** input;
	double** output;
	double above;
	double below;
	double left;
	double right;
	double result;
	int cell_x;
	int cell_y;
	
	while(*data->complete == 0)
	{
		// Calculate which direction to use the arrays in
		if (*data->direction == 1)
		{
			input = data->left_array;
			output = data->right_array;
		}
		else
		{
			input = data->right_array;
			output = data->left_array;
		}
		
		// Calculate relaxed values for each cell in the output array
		for (cell_y = data->lower_y; cell_y <= data->upper_y; cell_y++)
		{
			for (cell_x = data->lower_x; cell_x <= data->upper_x; cell_x++) 
			{
				above = input[cell_y - 1][cell_x];
				below = input[cell_y + 1][cell_x];
				left = input[cell_y][cell_x - 1];
				right = input[cell_y][cell_x + 1];

				result = (above + below + left + right) / 4.0;
				output[cell_y][cell_x] = result;
			}
		}

		pthread_barrier_wait(data->parallel_complete);
		
		pthread_barrier_wait(data->serial_complete);
	}

	return 0;
}

// Compares two arrays to see if they differ by more than the precision
int compare(double** input, double ** output, int dimension, double precision)
{
	int outer;
	int inner;
	double difference;

	for (outer = 1; outer < dimension - 1; outer++)
	{
		for (inner = 1; inner < dimension - 1; inner++)
		{
			difference = output[outer][inner] - input[outer][inner];

			if (difference > precision)
			{
				return 0;
			}
		}
	}

	return 1;
}

void initialiseBarrier(pthread_barrier_t* barrier, int size)
{
	if (pthread_barrier_init(barrier, NULL, size))
	{
		fprintf(stderr, "Could not create serial barrier");
		exit(-1);
	}
}

void initialiseArrayValues(double** array, int dimension)
{
	int outer;
	int inner;

	for (outer = 0; outer < dimension; outer++)
	{
		for(inner = 0; inner < dimension; inner++)
		{
			if (outer == 0 || inner == 0)
			{
				array[outer][inner] = 1;
			}
			else
			{
				array[outer][inner] = 0;
			}
		}
	}
}

int main(int argc, char* argv[])
{
	int index;
	struct timeval serial_start;
	struct timeval serial_end;
	struct timeval parallel_start;
	struct timeval parallel_end;
	double serial_time = 0.0;
	double parallel_time = 0.0;
	
	// Arguments	
	int dimension = strtol(argv[1], NULL, 10);
	int thread_count = strtol(argv[2], NULL, 10);
	double precision = strtod(argv[3], NULL);
	int time_flag = 0;
	int print_flag = 0;
	
	for(index = 4; index < argc; index++)
	{
		// Check for timing flag
		if (strcmp("-t", argv[index]) == 0)
		{
			time_flag = 1;
		}
		// Check for printing flag
		else if (strcmp("-p", argv[index]) == 0)
		{
			print_flag = 1;
		}
	}
	
	// Thread variables
	struct thread_data* data;
	pthread_t threads[thread_count];
	pthread_barrier_t serial_complete;
	pthread_barrier_t parallel_complete;
	double** left_array = NULL;
	double** right_array = NULL;
	double** input;
	double** output;
	int direction = 1;
	int complete = 0;

	
	if(time_flag)
	{
		recordTime(&serial_start);
	}
	
	initialiseBarrier(&serial_complete, thread_count + 1);
	initialiseBarrier(&parallel_complete, thread_count + 1);

	// Allocate array memory
	left_array = (double**) malloc(dimension * sizeof(double*));
	right_array = (double**) malloc(dimension * sizeof(double*));
	for (index = 0; index < dimension; index++)
	{
		left_array[index] = (double*) malloc(dimension * sizeof(double*));
		right_array[index] = (double*) malloc(dimension * sizeof(double*));
	}
	
	initialiseArrayValues(left_array, dimension);
	initialiseArrayValues(right_array, dimension);
	
	// Thread array bound calculations
	int sub_dimension = ((dimension - 2) / thread_count) - 1;
	int overflow = (dimension - 2) % thread_count;
	int lower_y = 1;
	int upper_y = lower_y + sub_dimension;
	
	if (time_flag)
	{
		recordTime(&serial_end);
		serial_time += calculateTimePassed(serial_start, serial_end);
		recordTime(&parallel_start);
	}
	
	// Initialise threads
	for (index = 0; index < thread_count; ++index)
	{
		data = malloc(sizeof(struct thread_data));
		data->dimension = dimension;
		data->left_array = left_array;
		data->right_array = right_array;
		data->direction = &direction;
		data->complete = &complete;
		data->serial_complete = &serial_complete;
		data->parallel_complete = &parallel_complete;
		data->lower_x = 1;
		data->upper_x = dimension - 2;	

		if (overflow > 0)
		{
			// Increment the bound to consume some of the overflow
			upper_y++;
			overflow--;
		}
		
		// Set lower and upper bounds for this thread
		data->lower_y = lower_y;
		data->upper_y = upper_y;

		if(pthread_create(&threads[index], NULL, relax, (void*) data))
		{
			fprintf(stderr, "Could not create thread: %d\n", index + 1);
			exit(-1);
		}

		// Move bounds for next thread
		lower_y = upper_y + 1;
		upper_y = lower_y + sub_dimension;
	}
	
	while (complete == 0)
	{
		// Wait for threads to complete
		pthread_barrier_wait(&parallel_complete);
		
		if (time_flag)
		{
			recordTime(&parallel_end);
			parallel_time += calculateTimePassed(parallel_start, parallel_end);
			recordTime(&serial_start);
		}
		
		// Calculate which direction to use the arrays in
		if (direction == 1)
		{
			input = left_array;
			output = right_array;
		}
		else
		{
			input = right_array;
			output = left_array;
		}
		
		// Compare the differences in the arrays
		if (compare(input, output, dimension, precision))
		{
			complete = 1;
		}
		else
		{
			direction =  -direction;	
		}
		
		// Allow threads to continue
		pthread_barrier_wait(&serial_complete);		

		if (time_flag)
		{
			recordTime(&serial_end);
			parallel_time += calculateTimePassed(serial_start, serial_end);
			recordTime(&parallel_start);
		}
	}
	
	// Wait for threads to finish
	for (index = 0; index < thread_count; index++)
	{
		pthread_join(threads[index], NULL);
	}
	
	if (time_flag)
	{
		recordTime(&parallel_end);
		parallel_time += calculateTimePassed(parallel_start, parallel_end);
	
		printf("Parallel time: %f\n", parallel_time);
		printf("Serial time: %f\n", serial_time);
	}
	
	if (print_flag)
	{
		print_array(data);
	}
	
	// Clean up array memory
	for (index = 0; index < dimension; index++)
	{
		free(left_array[index]);
		free(right_array[index]);
	}
	free(left_array);
	free(right_array);

	return 0;
}
