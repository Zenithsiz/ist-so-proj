#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include "fs/operations.h"

#define MAX_INPUT_SIZE 100

void applyCommands()
{

	char line[MAX_INPUT_SIZE];

	while (fgets(line, sizeof(line), stdin))
	{
		char token, type;
		char name[MAX_INPUT_SIZE];

		int numTokens = sscanf(line, "%c %s %c", &token, name, &type);
		if (numTokens < 2)
		{
			fprintf(stderr, "Error: invalid command\n");
			return;
		}

		int searchResult;
		switch (token)
		{
		case 'c':
			switch (type)
			{
			case 'f':
				printf("Create file: %s\n", name);
				if (create(name, T_FILE) == FAIL)
					printf("Create: could not create file %s\n", name);
				else
					printf("Create: %s successfully created\n", name);
				break;
			case 'd':
				printf("Create directory: %s\n", name);
				if (create(name, T_DIRECTORY) == FAIL)
					printf("Create: could not create directory %s\n", name);
				else
					printf("Create: %s successfully created\n", name);
				break;
			default:
				fprintf(stderr, "Error: invalid node type\n");
				continue;
			}
			break;
		case 'l':
			searchResult = lookup(name);
			if (searchResult == FAIL)
				printf("Search: %s not found\n", name);
			else
				printf("Search: %s found\n", name);
			break;
		case 'd':
			printf("Delete: %s\n", name);
			if (delete (name) == FAIL)
				printf("Delete: could not delete %s\n", name);
			else
				printf("Delete: %s successfully deleted\n", name);
			break;
		default:
		{
			/* error */
			fprintf(stderr, "Error: command to apply\n");
			continue;
		}
		}
	}
}

int main(int argc, char *argv[])
{

	/* init filesystem */
	init_fs();

	/* process input and print tree */
	applyCommands();
	print_tecnicofs_tree(stdout);

	/* release allocated memory */
	destroy_fs();
	exit(EXIT_SUCCESS);
}
