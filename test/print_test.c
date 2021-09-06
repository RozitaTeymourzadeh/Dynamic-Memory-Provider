/**
 * @file
 *
 * Explores memory management at the C runtime level.
 *
 * Author: Rozita Teymourzadeh , Allison Wong
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

	FILE *fp;
	fp = stderr;
    // fp = fopen("output.txt","a");

    fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string = malloc(sizeof(char)* 1024);
	strcpy(string, "DONE WITH MALLOC STRING 1");
	fprintf(stderr, "%s\n", string);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	char *string2 = malloc(sizeof(char)* 1024);

	strcpy(string, "DONE WITH MALLOC STRING 2");
	fprintf(stderr, "%s\n", string);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	fprintf(stderr, "%s\n", "REALLOC CASE 1: Now reallocing a smaller size");
	string2 = realloc(string2, sizeof(char)* 100);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	fprintf(stderr, "%s\n", "REALLOC CASE 2: Now reallocing a bigger size");
	string2 = realloc(string2, sizeof(char)* 5000);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	fprintf(stderr, "%s\n", "REALLOC CASE 3: Now reallocing same size");
	string2 = realloc(string2, sizeof(char)* 5000);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	fprintf(stderr, "%s\n", "REALLOC CASE 4: Now reallocing size 0");
	string2 = realloc(string2, 0);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	free(string);

	fputs("--------------------------", fp);
	save_memory(fp);


	fprintf(stderr, "%s\n", "NAMED MALLOC");
	char *named = malloc_name(sizeof(char)* 1024, "Named Block");
	strcpy(named, "DONE WITH MALLOC OF NAMED BLOCK");
	fprintf(stderr, "%s\n", named);

	fputs("--------------------------\n", fp);
	save_memory(fp);

	free(named);

	fputs("--------------------------", fp);
	save_memory(fp);

    fclose(fp);

	//LOG("%s\n", string);

}