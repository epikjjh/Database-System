#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <inttypes.h>
#include "bpt.h"
#include "file.h"
#include <string.h>
#include <time.h>

// MAIN
int main( int argc, char ** argv ) {
	uint64_t input_key;
    char input_value[SIZE_VALUE];
	char instruction;
    int table = 0;

    //license_notice();
	//usage_1();  
	//usage_2();

    init_db(5);
    table = open_table("test.db");
	//printf("> ");
	while (scanf("%c", &instruction) != EOF) {
		switch (instruction) {
		case 'i':
			scanf("%" PRIu64 " %s", &input_key, input_value);
			insert(table, input_key, input_value);
			//print_tree(table);
			break;
        case 'd':
			scanf("%" PRIu64 "", &input_key);
			delete(table, input_key);
			//print_tree(table);
			break;
		case 'f':
		//case 'p':
			scanf("%" PRIu64 "", &input_key);
			find_and_print(table, input_key);
            fflush(stdout);
			break;
		case 'q':
			while (getchar() != (int)'\n');
			return EXIT_SUCCESS;
			break;
		case 't':
			//print_tree(table);
            break;
        default:
			//usage_2();
			break;
		}
		while (getchar() != (int)'\n');
		//printf("> ");
	}
	//printf("\n");

    close_table(table);

	return EXIT_SUCCESS;
}
