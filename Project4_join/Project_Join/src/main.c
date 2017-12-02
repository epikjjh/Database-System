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

void input_table(int table_num, uint64_t size, char *input_value);
void check(int table_num, uint64_t size);

// MAIN
int main( int argc, char ** argv ) {
    char input_value_1[SIZE_VALUE], input_value_2[SIZE_VALUE];
    uint64_t size_1, size_2;
    int table_1 = 0, table_2 = 0;

    printf("Welcome to join test\n");

    init_db(5);
    
    /* Prepare database : table 1 */
    table_1 = open_table("table1.db");
       
    printf("Input size and each value for table 1 : ");
    scanf("%" PRIu64 " %s", &size_1, input_value_1);
    input_table(table_1, size_1, input_value_1);

    close_table(table_1);

    /* Prepare database : table 2 */

    table_2 = open_table("table2.db");

    printf("Input size and each value for table 2 : ");
    scanf("%" PRIu64 " %s", &size_2, input_value_2);
    input_table(table_2, size_2, input_value_2);

    close_table(table_2);

    /* Reopen two table and start join */
    table_1 = open_table("table1.db");
    table_2 = open_table("table2.db");

    /* Join */
    join_table(table_1, table_2, "result.txt");    
    
    close_table(table_1);
    close_table(table_2);

	return EXIT_SUCCESS;
}
void input_table(int table_num, uint64_t size, char *input_value){
    uint64_t i;

    for(i = 0; i < size; i++){
        insert(table_num, i, input_value);
    }
}
void check(int table_num, uint64_t size){
    uint64_t i;

    for(i = 0; i < size; i++){
	    find_and_print(table_num, i);
    }
}
