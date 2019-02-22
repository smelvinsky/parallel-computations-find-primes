/**
 *  @file   cuda.cu
 *  @author Damian Smela <damian.a.smela@gmail.com>
 *  @date   10.02.2019
 *  @brief  CUDA example - finds all the primes in the large list of positive 
 *          integers.
 */

/********************************* INCLUDES ***********************************/

#include <cuda.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>

/********************************* DEFINES ************************************/

#define FILE_LINE_BUFF_SIZE (128U)
#define TMP_STR_BUFF_SIZE   (FILE_LINE_BUFF_SIZE)
#define LIST_FILENAME       "prime_list.txt"
#define NUM_OF_THREADS      (4U)

#define NUM_OF_CUDA_BLOCKS  (64U)
#define NUM_OF_CUDA_THREADS (128U)

/***************************** STATIC FUNCTIONS *******************************/

/* Converts ASCII char to corresponding number */
static int ascii_to_num(char ascii_char)
{
    return ((int) ascii_char) - 48;
}


/* Checks if given string is a integer number */
static bool is_number(char num_str[])
{
    char *str_ptr = &num_str[0];

    while(*str_ptr != '\0')
    {
     	if (ascii_to_num(*str_ptr) < 0 || ascii_to_num(*str_ptr) > 9)
        {
            return false;
        }
	    str_ptr++;
    }
    return true;
}


/* Returns the next positive integer from list file */
static int get_next_num_from_file(FILE      *file_ptr, 
                                  char      *line_buff, 
                                  size_t    *line_buff_size)
{
    int num_of_line_chars_tmp;

    memset(line_buff, 0, *line_buff_size);
    num_of_line_chars_tmp = (int) getline(&line_buff, line_buff_size, file_ptr);
    line_buff[num_of_line_chars_tmp - 1] = '\0';                                /* Erase '\n' char from string*/

    if (!is_number(line_buff))
    {
     	return -1;
    }
    else
    {
     	return atoi(line_buff);
    }
}


/* Resolve number of CUDA blocks and threads to cover all the remaining ints
 * to process */
static void resolve_cuda_threads(const int  ints_remaining,
                                 int        *num_of_cuda_blocks,
                                 int        *num_of_cuda_threads)
{
    /* Change the defaults only if there is lower number of data to process
     * than threads in a block */
    if (ints_remaining < NUM_OF_CUDA_BLOCKS * NUM_OF_CUDA_THREADS)
    {
        /* Load the defaults */
        *num_of_cuda_blocks = NUM_OF_CUDA_BLOCKS;
        *num_of_cuda_threads = NUM_OF_CUDA_THREADS;

        /* Check if the next iteration can be run in one-shot */
        for (int i = *num_of_cuda_threads; i > 0; i--)
        {
            if (ints_remaining % i == 0)
            {
                /* If there is enough blocks */
                if (ints_remaining / i < NUM_OF_CUDA_BLOCKS)
                {
                    /* The last iteration can be run in a one shot */
                    *num_of_cuda_blocks = ints_remaining / i;
                    *num_of_cuda_threads = i;
                    return;
                }
            }
        }

        /* The code reaches here if there is no way to run the code in
         * a one-shot */
        
        /* Calculate how many full blocks can run the next iteration 
         * and modify the defaults for the next iteration */
        *num_of_cuda_blocks = ints_remaining / NUM_OF_CUDA_THREADS;
    }
}                                                    

/******************************* CUDA KERNELS *********************************/

__global__ void is_prime_kernel(int *int_list)
{
    /* Get the unique thread index */
    int index = threadIdx.x + blockIdx.x * blockDim.x;

    /* Check if the integer of this index is prime */
    for (int i = 2; i < *(int_list + index); i++)
    {
        if (*(int_list + index) % i == 0)
        {
            /* If it's not a prime then change its value to -1 
             * so it wont be written to the file */
            *(int_list + index) = -1;
            break;
        }
    }
}

/*********************************** MAIN *************************************/

int main(int argc, char **argv)
{
    FILE *file_load_ptr = NULL;                                                 /* File pointer for loaded file */
    FILE *file_gen_ptr = NULL;                                                  /* File pointer for generated file */
    char *filename;                                                             /* Pointer to the string containing file name */
    char *line_buff;                                                            /* Buffer storing temp line of a file */
    size_t buff_size = FILE_LINE_BUFF_SIZE;                                     /* Size of a line buffer (can be realloc()-ed by the getline() func) */
    int num_of_line_chars_tmp;                                                  /* Var storing temp num of chars in the line */
    char str_buff_tmp[TMP_STR_BUFF_SIZE];                                       /* String buffer for temp actions */
    int num_of_ints;                                                            /* Number of integers to analyze */

    int num_of_cuda_blocks = NUM_OF_CUDA_BLOCKS;                                /* Number of cuda blocks */
    int num_of_cuda_threads = NUM_OF_CUDA_THREADS;                              /* Number of cuda threads */                     

    int prime_cnt = 0;                                                          /* Number of primes found */

    float time;                                                                 /* Variables used for measuring time */
    cudaEvent_t start, stop;                                                    /* - || - */

    printf("-------------------------CUDA Example-------------------------\n");
    printf("Finds all the primes in the large list of positive integers\n\n");

    /* Check if there is exactly one argument given */
    if (argc != 2)
    {
     	printf("Program needs exactly one argument - filename of the list " 
               "containing positive integer numbers!\n");
        return 1;
    }

    filename = argv[1];

    /* Open the ./"filename" file with read permission */
    file_load_ptr = fopen(filename, "r");
    if (file_load_ptr == NULL)
    {
     	printf("Couldn't open \"%s\" file\n", filename);
        return 2;
    }

    /* Allocate buffers for string parsing operations */
    line_buff = (char *) malloc(buff_size);
    memset(line_buff, 0, buff_size);
    memset(str_buff_tmp, 0, TMP_STR_BUFF_SIZE);

    /* Read the first line from file */
    num_of_line_chars_tmp = (int) getline(&line_buff, 
                                          &buff_size, 
                                          file_load_ptr);

    /* Check if the first line fits the "list_len=X" pattern */
    if ((num_of_line_chars_tmp < 11) || 
        (strncmp(line_buff, "list_len=", 9) != 0))
    {
        printf("Error in the first line - \"%s\" not matching" 
               "\"list_len=X\" pattern!\n", line_buff);

        return 3;
    }
    
    /* Check if the first line contains a number of integers */
    strncpy(str_buff_tmp, &line_buff[9], (size_t) num_of_line_chars_tmp - 10);
    if (!is_number(str_buff_tmp))
    {
     	printf("Error in the first line - \"%s\" is not a correct "
               "list length value\n", str_buff_tmp);

        return 4;
    }

    /* Store the number of integers in this variable */
    num_of_ints = atoi(str_buff_tmp);
    printf("Loading list of %d integer numbers...\n", num_of_ints);

    /* Allocate memory for all the integers */
    int *int_list = (int *) malloc(sizeof(int) * num_of_ints);

    /* Fill the memory with the integers from file */
    for (int i = 0; i < num_of_ints; i++)
    {
     	int_list[i] = get_next_num_from_file(file_load_ptr, 
                                             line_buff, 
                                             &buff_size);
    }

    /* Create a "LIST_FILENAME" file with write permission */
    file_gen_ptr  = fopen(LIST_FILENAME, "w");
    if (file_gen_ptr == NULL)
    {
     	printf("Couldn't open/create \"%s\" file\n", LIST_FILENAME);
        return 5;
    }

    /************** CUDA **************/

    /* Pointer to the current slice of the list */
    int *host_int_list_ptr = &int_list[0];

    /* Pointer to the space for device copy of the list */
    int *cuda_int_list;

    /* Remaining ints to process */    
    int ints_remaining = num_of_ints;

    /* Start timer */
    cudaEventCreate(&start);
    cudaEventCreate(&stop);
    cudaEventRecord(start, 0);

    /* Allocate the space for device copy of the list-slice */
    cudaMalloc((void **)&cuda_int_list, 
               num_of_cuda_blocks * num_of_cuda_threads * sizeof(int));

    /* As long as the number of remaining ints is equal or greater than the 
     * total number of parallel instances use all of them to compute */
    while (ints_remaining > 0)
    {
        /* Resolve the number of blocks and threads that will be used */
        resolve_cuda_threads(ints_remaining,
                             &num_of_cuda_blocks,
                             &num_of_cuda_threads);

        /* Copy the slice of the list to the device */
        cudaMemcpy(cuda_int_list,
                   host_int_list_ptr,
                   num_of_cuda_blocks * num_of_cuda_threads * sizeof(int),
                   cudaMemcpyHostToDevice);
                   
        /* Run the kernel on all threads */
        is_prime_kernel<<<num_of_cuda_blocks, 
                          num_of_cuda_threads>>>(cuda_int_list);

        /* Copy the results back to the host */
        cudaMemcpy(host_int_list_ptr,
                   cuda_int_list,
                   num_of_cuda_blocks * num_of_cuda_threads * sizeof(int),
                   cudaMemcpyDeviceToHost);

        /* Update the host list pointer */
        host_int_list_ptr += num_of_cuda_blocks * num_of_cuda_threads;

        /* Update the number of remaining ints */
        ints_remaining -= (num_of_cuda_blocks * num_of_cuda_threads);
    }

    /* Free the device's memory */
    cudaFree(cuda_int_list);

    /* Write data to the file */
    for (int i = 0; i < num_of_ints; i++)
    {
        if (int_list[i] != -1)
        {
            prime_cnt++;
            fprintf(file_gen_ptr, "%d\n", int_list[i]);
        }
    }

    /* Put the number of primes int the first line of generated file */
    rewind(file_gen_ptr);
    fprintf(file_gen_ptr, "primes_found=%d(open-mp)\n", prime_cnt);

    cudaEventRecord(stop, 0);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&time, start, stop);
    printf("Elapsed: %f ms\n", time);
    printf("%d primes found...\n\r", prime_cnt);

    /**********************************/    

    /* Close the files and free the memory */    
    if (fclose(file_load_ptr) != 0)
    {
     	printf("Couldn't close \"%s\" file\n", filename);
        return 6;
    }

    if (fclose(file_gen_ptr) != 0)
    {
     	printf("Couldn't close \"%s\" file\n", LIST_FILENAME);
        return 7;
    }
    
    free(line_buff);
    free(int_list);

    printf("Done...\n");
    return 0;
}

/************************************ EOF *************************************/
