/********************************* INCLUDES ***********************************/

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

/********************************* DEFINES ************************************/

#define MAX_INT_NUMBER  (10000U)
#define LIST_FILENAME   "list.txt"

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

/*********************************** MAIN *************************************/
int main(int argc, char **argv)
{
    int num_to_gen;     /* Number of positive integers to generate */
    FILE *file_ptr;     /* File pointer */

    /* Check if there is exactly one argument given */
    if (argc != 2)
    {
     	printf("Program needs exactly one argument"
               " - positive integer list length!\n");
        return 1;
    }

    /* Check if given argument is a positive integer number */
    if (!is_number(argv[1]))
    {
     	printf("%s is not a positive integer number\n", argv[1]);
        return 1;
    }

    /* Convert string argument to a number */
    num_to_gen = atoi(argv[1]);

    printf("Generating list of %d positive integers\n", num_to_gen);

    /* Open & overwrite (or create) a "list.txt" file with write permission */
    file_ptr = fopen(LIST_FILENAME, "w");
    if (file_ptr == NULL)
    {
     	printf("Couldn't open/create \"%s\" file\n", LIST_FILENAME);
        return 1;
    }

    /* Put the info (number of positive integers) in list_len=X format */
    fprintf(file_ptr, "list_len=%d\n", num_to_gen);

    /* Fill the file with rundom numbers from <0, MAX_INT_NUMBER> set */
    for (int i = 0; i < num_to_gen; i++)
    {
     	int rand_num = ((uint32_t) random()) % MAX_INT_NUMBER + 1;
        fprintf(file_ptr, "%d\n", rand_num);
    }

    /* Close the file */
    if (fclose(file_ptr) != 0)
    {
     	printf("Couldn't close \"%s\" file\n", LIST_FILENAME);
        return 1;
    }

    printf("Done...\n");
    return 0;
}
/************************************ EOF *************************************/

