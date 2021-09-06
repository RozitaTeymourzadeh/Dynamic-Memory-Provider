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
#include "allocator.h"

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
	FILE *fp = stderr;

	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string0 = malloc_name(500, "ALLOCATION 0");

	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string1 = malloc_name(1000, "ALLOCATION 1");

	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string2 = malloc_name(250, "ALLOCATION 2");

	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string3 = malloc_name(294, "ALLOCATION 3");

	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string4 = malloc_name(400, "ALLOCATION 4");

	fputs("--------------------------\n", fp);
	fputs("---Expecting 0, 1, 2, 3, 4---\n", fp);
	fputs("--------------------------\n", fp);
	save_memory(fp);


	free(string1);
	free(string3);

	fputs("--------------------------\n", fp);
	fputs("---Expecting 0, 2, 4---\n", fp);
	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string5 = malloc_name(600, "ALLOCATION 5");

	fputs("--------------------------\n", fp);
	fputs("---Expecting 0, 5, 2, 4---\n", fp);
	fputs("--------------------------", fp);
	save_memory(fp);


	char *string6 = malloc_name(150, "ALLOCATION 6");

	fputs("--------------------------\n", fp);
	fputs("---Expecting 0, 5, 2, 6, 4---\n", fp);
	fputs("--------------------------", fp);
	save_memory(fp);


	char *string7 = malloc_name(44, "ALLOCATION 7");

	fputs("--------------------------\n", fp);
	fputs("---Expecting 0, 5, 2, 6, 7, 4---\n", fp);
	fputs("--------------------------", fp);
	save_memory(fp);

	free(string0);
	free(string2);
	free(string4);
	free(string5);
	free(string6);
	free(string7);



    fclose(fp);

	//LOG("%s\n", string);

}