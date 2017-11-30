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

void input_table(int table_num, int num, const char *input_value);
void check(int table_num, int size);

// MAIN
int main( int argc, char ** argv ) {
	uint64_t input_key;
    char input_value[SIZE_VALUE];
	char instruction;
    int table_1 = 0, table_2 = 0, size = 0;

    init_db(5);
    table_1 = open_table("table1.db");
    table_2 = open_table("table2.db");
    
    printf("Welcome to join test\n");

    printf("Input size and each value for table 1 : ");
    scanf("%d %s", &size, input_value);
    input_table(table_1, size, input_value);
    /* Check */
    check(table_1, size);

    printf("Input size and each value for table 2 : ");
    scanf("%d %s", &size, input_value);
    input_table(table_2, size, input_value);
    /* Check */
    check(table_2, size);
    
    close_table(table_1);
    close_table(table_2);

	return EXIT_SUCCESS;
}
void input_table(int table_num, int size, const char *input_value){
    int i;

    for(i = 0; i < size; i++){
        insert(table_num, i, input_value);
    }
}
void check(int table_num, int size){
    int i;

    for(i = 0; i < size; i++){
	    find_and_print(table_num, i);
    }
}
/*
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
	}
*/
