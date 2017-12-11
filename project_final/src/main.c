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

void input_table(int table_num, uint64_t start, uint64_t end, uint64_t jump, char *input_value);
void check(int table_num, uint64_t size);

// MAIN
int main( int argc, char ** argv ) {
    char input_value_1[SIZE_VALUE], input_value_2[SIZE_VALUE];
    uint64_t start, end, jump;
    int table_1 = 0, table_2 = 0;
    time_t start_t, end_t;

    printf("Welcome to join test\n");

    init_db(5);
    
    /* Prepare database : table 1 */
    table_1 = open_table("table1.db");
       
    printf("Start / End / Jump / Each value for table 1 : ");
    scanf("%" PRIu64 " %" PRIu64 " %" PRIu64 " %s", &start, &end, &jump, input_value_1);
    input_table(table_1, start, end, jump, input_value_1);

    /* Prepare database : table 2 */

    table_2 = open_table("table2.db");

    printf("Start / End / Jump / Each value for table 2 : ");
    scanf("%" PRIu64 " %" PRIu64 " %" PRIu64 " %s", &start, &end, &jump, input_value_2);
    input_table(table_2, start, end, jump, input_value_2);

    //close_table(table_2);

    /* Join */
    start_t = clock();
    join_table(table_1, table_2, "result.txt");    
    end_t = clock();

    /* Print result time */
    printf("Result time : %f\n", (float)(end - start)/(CLOCKS_PER_SEC));
    close_table(table_1);
    close_table(table_2);

    /* Remove database file */
    remove("table1.db");
    remove("table2.db");

	return EXIT_SUCCESS;
}
void input_table(int table_num, uint64_t start, uint64_t end, uint64_t jump, char *input_value){
    uint64_t i;

    if(jump < 0 || start > end){
        printf("Fail!\n");

        return;
    }

    for(i = start; i <= end; i += jump){
        insert(table_num, i, input_value);
        if(jump == 0){
            i++;
        }
    }
}
void check(int table_num, uint64_t size){
    uint64_t i;

    for(i = 0; i < size; i++){
	    find_and_print(table_num, i);
    }
}
