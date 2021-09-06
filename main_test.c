
/**
 * @file
 *
 * Explores memory management at the C runtime level.
 *
 * Author: Rozita Teymourzadeh , Allison Wong, Mathew Malensek
 *
 * To use (one specific command):
 * LD_PRELOAD=$(pwd)/allocator.so command
 * ('command' will run with your allocator)
 *
 * To use (all following commands):
 * export LD_PRELOAD=$(pwd)/allocator.so
 * (Everything after this point will use your custom allocator -- be careful!)
  */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

/**
 * void main()
 *
 * Test Driver
 *
 * @param void
 * @return void
  */
int main(void)
{
	char *string = malloc(sizeof(char)* 1024);
	strcpy(string, "HELLO CLASS!");
	fprintf(stderr, "%s\n", string);
	fprintf(stderr, "%s\n", "REALLOC CASE 1: Now reallocing a smaller size");
	string = realloc(string, sizeof(char)* 100);
	fprintf(stderr, "%s\n", "REALLOC CASE 2: Now reallocing a bigger size");
	string = realloc(string, sizeof(char)* 5000);
	fprintf(stderr, "%s\n", "REALLOC CASE 3: Now reallocing same size");
	string = realloc(string, sizeof(char)* 5000);

	free(string);

	//LOG("%s\n", string);

}