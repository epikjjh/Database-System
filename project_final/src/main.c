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

    init_db(5);
    
    table_1 = open_table("DATA1");
    table_2 = open_table("DATA2");

    update(table_1, 1, "A");
    find_and_print(table_1, 1);
    insert(table_1, 1, "B");
    find_and_print(table_1, 1);
    update(table_1, 1, "C");
    find_and_print(table_1, 1);

    close_table(table_1);
    close_table(table_2);

    /* Remove database file */
    remove("DATA1");
    remove("DATA2");

	return EXIT_SUCCESS;
}
