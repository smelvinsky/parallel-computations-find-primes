/**
 *  @file       mpi.c
 *  @author     Damian Smela <damian.a.smela@gmail.com>
 *  @date       10.02.2019
 *  @brief      MPI example - finds all the primes in the large list of positive
 *              integers.
 */

/********************************* INCLUDES ***********************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <mpi.h>
#include "my_timers.h"

/********************************* DEFINES ************************************/

#define FILE_LINE_BUFF_SIZE (128)
#define TMP_STR_BUFF_SIZE   (FILE_LINE_BUFF_SIZE)
#define LIST_FILENAME       "prime_list.txt"

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
                                  
/*********************************** MAIN *************************************/
    
    
int main(int argc, char **argv)
{
    FILE *file_load_ptr = NULL;                                                 /* File pointer for loaded file */
    FILE * file_gen_ptr;                                                        /* File pointer for generated file */
    char *filename;                                                             /* Pointer to the string containing file$
    char *line_buff;                                                            /* Buffer storing temp line of a file */
    size_t buff_size = FILE_LINE_BUFF_SIZE;                                     /* Size of a line buffer (can be realloc$
    int ret_val = 0;                                                            /* Return value */
    int num_of_line_chars_tmp;                                                  /* Var storing temp num of chars in the $
    char str_buff_tmp[TMP_STR_BUFF_SIZE];                                       /* String buffer for temp actions */
     
    int num_of_ints;                                                            /* Number of all integers to analyze */
    int num_of_sub_ints;                                                        /* Number of integers in a sub list */

    int *int_list;                                                              /* Full list of ints */
    int *sub_int_list;                                                          /* Sub list of ints */
    
    int num_of_processes;                                                       /* Number of processes */
    int rank;                                                                   /* Rank of a current thread */
    int global_prime_cnt = 0;                                                   /* Number of all primes in a list */ 
    int local_prime_cnt = 0;                                                    /* Number of primes in a sub list */ 
    
    /* Initialize the MPI execution environment */
    MPI_Init(NULL, NULL);

    /* Determines the size of the group associated with a communicator */
    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
     
    /* Determines the rank of the calling process in the communicator */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    /* This part is being performed only by the root process */
    if (rank == 0)
    {
        printf("------------------------------MPI-----------------------------\n");
        printf("Finds all the primes in the large list of positive integers\n\n");

        /* Check if there is exactly one argument given */
        if (argc != 2)
        {
            printf("Program needs exactly one argument - filename of list "
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
            printf("Error in the first line - \"%s\" not matching "
                   "\"list_len=X\" pattern!\n", line_buff);
    
            ret_val = 3;
            goto cleanup;
        }

        /* Check if the first line contains a number of integers */
        strncpy(str_buff_tmp, &line_buff[9], (size_t) num_of_line_chars_tmp - 10);
        if (!is_number(str_buff_tmp))
        {
            printf("Error in the first line - \"%s\" is not a correct list length value\n", str_buff_tmp);
            ret_val = 4;
            goto cleanup;
        }
            
        /* Store the number of integers in this variable */
        num_of_ints = atoi(str_buff_tmp);
        printf("Loading list of %d integer numbers...\n", num_of_ints);
            
        /* Allocate memory for all the integers */
        int_list = (int *) malloc(sizeof(int) * num_of_ints);
         
        /* Fill the memory with the integers from file */
        for (int i = 0; i < num_of_ints; i++)
        {
            int_list[i] = get_next_num_from_file(file_load_ptr,
                                                 line_buff,
                                                 &buff_size);
        }
            
        file_gen_ptr  = fopen(LIST_FILENAME, "w");
        if (file_gen_ptr == NULL)
        {
            printf("Couldn't open/create \"%s\" file\n", LIST_FILENAME);
            ret_val = 5;
            goto cleanup;
        }
    }

    /************* MPI *************/
    start_time();
         
    /* Root broadcasts num_of_ints to all processes */
    MPI_Bcast(&num_of_ints, 1, MPI_INT, 0, MPI_COMM_WORLD);
                                                 
    /* Allocate sub list for every thread */
    num_of_sub_ints = num_of_ints / num_of_processes;
    sub_int_list = (int *) malloc(sizeof(int) * num_of_sub_ints);
        
    /* Scatter data from root to all the processes */
    MPI_Scatter(int_list,       num_of_sub_ints,    MPI_INT,
                sub_int_list,   num_of_sub_ints,    MPI_INT,
                0,              MPI_COMM_WORLD);
         
    /* Count and write local primes to the file */
    bool prime;
    for (int i = 0; i < num_of_sub_ints; i++)
    {
        prime = true;
        for (int j = 2; j < sub_int_list[i]; j++)
        {
            if (sub_int_list[i] % j == 0)        
            {
                prime = false;
                break;
            }
        }
        if (prime == true)
        {
            local_prime_cnt++;
        }
        else
        {
            sub_int_list[i] = -1;
        }
    }
        
    /* Reduce the prime_cnt value */
    MPI_Reduce(&local_prime_cnt, &global_prime_cnt, 1,
               MPI_INT,          MPI_SUM,           0,
               MPI_COMM_WORLD);
                
    /* Gather all the data together */
    MPI_Gather(sub_int_list,     num_of_sub_ints,     MPI_INT,
               int_list,         num_of_sub_ints,     MPI_INT,
               0,                MPI_COMM_WORLD);
            
    /* Wait for all threads */
    MPI_Barrier(MPI_COMM_WORLD);
         
    stop_time();
    /*******************************/
     
    cleanup:

    /* Only root does the cleanup */
    if (rank == 0)
    {
        /* Root writes calculated data to the file  */
        fprintf(file_gen_ptr, "primes_found=%d(mpi)\n", global_prime_cnt);
        for (int i = 0; i < num_of_ints; i++)
        {
            if (int_list[i] != -1)
            {
                fprintf(file_gen_ptr, "%d\n", int_list[i]);
            }
        }
        
        /* Print the report */
        print_time("Elapsed:");
        printf("%d primes found...\n\r", global_prime_cnt);
    
        /* Close all the files and free the memory */
        if (fclose(file_load_ptr) != 0)
        {
            printf("Couldn't close \"%s\" file\n", filename);
            ret_val = 4;
        }
         
        if (fclose(file_gen_ptr) != 0)
        {
            printf("Couldn't close \"%s\" file\n", LIST_FILENAME);
            return 1;
        }
    
        free(line_buff);
     
        printf("Done...\n");
    }
        
    /* Finalize the MPI environment */ 
    MPI_Finalize();
            
    return ret_val;
}
         
/************************************ EOF *************************************/



