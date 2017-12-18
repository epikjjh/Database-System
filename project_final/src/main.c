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
    char input_value_1[SIZE_VALUE], input_value_2[SIZE_VALUE];
    uint64_t start, end, jump;
    int table_1 = 0, table_2 = 0, table_10 = 0;
    time_t start_t, end_t;

    printf("Welcome to recovery test\n");

    init_db(1);
    
    table_1 = open_table("DATA1");
    table_2 = open_table("DATA2");

    insert(table_1, 1, "A");
    insert(table_1, 2, "B");
    insert(table_2, 1, "C");
    insert(table_2, 2, "D");

    //find_and_print(table_1, 1);
    //find_and_print(table_1, 2);
    //find_and_print(table_2, 1);
    //find_and_print(table_2, 2);

    begin_transaction();
    update(table_1, 1, "table1_A");
    update(table_1, 2, "table1_B");
    update(table_2, 1, "table2_C");
    update(table_2, 2, "table2_D");
    commit_transaction();

    //find_and_print(table_1, 1);
    //find_and_print(table_1, 2);
    //find_and_print(table_2, 1);
    //find_and_print(table_2, 2);

    close_table(table_1);
    close_table(table_2);

    remove("DATA1");
    remove("DATA2");

	return EXIT_SUCCESS;
}
