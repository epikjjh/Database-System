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

void sequential_small_test();
void sequential_medium_test();
void sequential_large_test();
void random_small_test();
void random_medium_test();
void random_large_test();

// MAIN
int main( int argc, char ** argv ) {
	uint64_t input_key;
    char input_value[SIZE_VALUE];
	char instruction;

    //license_notice();
	//usage_1();  
	//usage_2();

    open_db("test.db");
	//printf("> ");
	while (scanf("%c", &instruction) != EOF) {
		switch (instruction) {
		case 'i':
			scanf("%" PRIu64 " %s", &input_key, input_value);
			insert(input_key, input_value);
			//print_tree();
			break;
        case 'd':
			scanf("%" PRIu64 "", &input_key);
			delete(input_key);
			//print_tree();
			break;
		case 'f':
		//case 'p':
			scanf("%" PRIu64 "", &input_key);
			find_and_print(input_key);
            fflush(stdout);
			break;
		case 'q':
			while (getchar() != (int)'\n');
			return EXIT_SUCCESS;
			break;
		case 't':
			//print_tree();
			break;
        // Sequential test case
        case 's':
            sequential_small_test();
            sequential_medium_test();
            sequential_large_test();
            break;
        // Random test case
        case 'r':
            random_small_test();
            random_medium_test();
            random_large_test();
            break;
        default:
			//usage_2();
			break;
		}
		while (getchar() != (int)'\n');
		//printf("> ");
	}
	//printf("\n");

    close_db();

	return EXIT_SUCCESS;
}
/* Sequential */
// 5000
void sequential_small_test(int table){
    char input_value[SIZE_VALUE] = " A";
    char* test_value = NULL;
    int i;
    clock_t start, end;

    start = clock();

    for(i = 1; i <= 5000; i++){
        input_value[0] = (i%10) + '0'; 
        insert(i, input_value); 
        test_value = find(i);
        if(strcmp(input_value, test_value) != 0){
            printf("Test Fail\n");
            return;
        }
        free(test_value);
    }

    end = clock();

    printf("Test Success\n");
    printf("Sequential small test : %f seconds\n", (float)(end - start)/CLOCKS_PER_SEC);
}
// 10000
void sequential_medium_test(int table){
    char input_value[SIZE_VALUE] = " A";
    char* test_value = NULL;
    int i;
    clock_t start, end;

    start = clock();

    for(i = 1; i <= 10000; i++){
        input_value[0] = (i%10) + '0'; 
        insert(i, input_value); 
        test_value = find(i);
        if(strcmp(input_value, test_value) != 0){
            printf("Test Fail\n");
            return;
        }
        free(test_value);
    }

    end = clock();

    printf("Test Success\n");
    printf("Sequential medium test : %f seconds\n", (float)(end - start)/CLOCKS_PER_SEC);
}
// 100000
void sequential_large_test(int table){
    char input_value[SIZE_VALUE] = " A";
    char* test_value = NULL;
    int i;
    clock_t start, end;

    start = clock();

    for(i = 1; i <= 100000; i++){
        input_value[0] = (i%10) + '0'; 
        insert(i, input_value); 
        test_value = find(i);
        if(strcmp(input_value, test_value) != 0){
            printf("Test Fail\n");
            return;
        }
        free(test_value);
    }

    end = clock();

    printf("Test Success\n");
    printf("Sequential large test : %f seconds\n", (float)(end - start)/CLOCKS_PER_SEC);

}
/* Random */
// 5000
void random_small_test(int table){

}
// 50000
void random_medium_test(int table){

}
// 500000
void random_large_test(int table){

}
