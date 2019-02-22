/**
 *  @file   openmp.c
 *  @author Damian Smela <damian.a.smela@gmail.com>
 *  @date   10.02.2019
 *  @brief  OpenMP example - finds all the primes in the large list of positive 
 *          integers.
 */

/********************************* INCLUDES ***********************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include <omp.h>
#include "my_timers.h"

/********************************* DEFINES ************************************/

#define FILE_LINE_BUFF_SIZE (128U)
#define TMP_STR_BUFF_SIZE   (FILE_LINE_BUFF_SIZE)
#define LIST_FILENAME       "prime_list.txt"
#define NUM_OF_THREADS      (4U)

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
    FILE *file_gen_ptr = NULL;                                                  /* File pointer for generated file */
    char *filename;                                                             /* Pointer to the string containing file name */
    char *line_buff;                                                            /* Buffer storing temp line of a file */
    size_t buff_size = FILE_LINE_BUFF_SIZE;                                     /* Size of a line buffer (can be realloc()-ed by the getline() func) */
    int ret_val = 0;                                                            /* Return value */
    int num_of_line_chars_tmp;                                                  /* Var storing temp num of chars in the line */

    char str_buff_tmp[TMP_STR_BUFF_SIZE];                                       /* String buffer for temp actions */
    int num_of_ints;                                                            /* Number of integers to analyze */

    printf("------------------------OpenMP example------------------------\n");
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

        ret_val = 3;
        goto cleanup;
    }
    
    /* Check if the first line contains a number of integers */
    strncpy(str_buff_tmp, &line_buff[9], (size_t) num_of_line_chars_tmp - 10);
    if (!is_number(str_buff_tmp))
    {
     	printf("Error in the first line - \"%s\" is not a correct "
               "list length value\n", str_buff_tmp);

        ret_val = 4;
        goto cleanup;
    }

    /* Store the number of integers in this variable */
    num_of_ints = atoi(str_buff_tmp);
    printf("Loading list of %d integer numbers...\n", num_of_ints);

    /* Allocate memory for all the integers */
    int *int_list = malloc(sizeof(int) * num_of_ints);

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
        ret_val = 5;
        goto cleanup;
    }

    /*********** OPEN MP ***********/

    int i;
    int j;
    int prime_cnt = 0;
    bool prime;

    omp_set_num_threads(NUM_OF_THREADS);

    start_time();

    #pragma omp parallel for schedule(dynamic) private(i, j, prime) shared(int_list, num_of_ints, prime_cnt)
    for (i = 0; i < num_of_ints; i++)
    {
     	prime = true;

        for (j = 2; j < int_list[i]; j++)
        {
            if (int_list[i] % j == 0)
            {
             	prime = false;
                break;
            }
	    }

	    if (prime == true)
        {
            #pragma omp critical
            {
             	prime_cnt++;
                fprintf(file_gen_ptr, "%d\n", int_list[i]);
            }
	    }
    }

    /********* END OPEN MP *********/

    /* Put the number of primes int the first line of generated file */
    rewind(file_gen_ptr);
    fprintf(file_gen_ptr, "primes_found=%d(open-mp)\n", prime_cnt);

    /* Print the report */
    stop_time();
    print_time("Elapsed:");
    printf("%d primes found...\n\r", prime_cnt);

    /* Close the files and free the memory */    
    cleanup:

    if (fclose(file_load_ptr) != 0)
    {
     	printf("Couldn't close \"%s\" file\n", filename);
        ret_val = 6;
    }

    if (fclose(file_gen_ptr) != 0)
    {
     	printf("Couldn't close \"%s\" file\n", LIST_FILENAME);
        ret_val = 7;
    }
    
    free(line_buff);
    free(int_list);

    printf("Done...\n");
    return ret_val;
}

/************************************ EOF *************************************/
