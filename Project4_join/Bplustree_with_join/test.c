#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define DEF_DB_PATH "./db"
#define DEF_NUM_BUF 5
#define UNUSED(x) (void)(x)


int init_db(uint64_t buf_size);
int shutdown_db();
int open_table(char *pathname);
int close_table(int table_id);
int insert(int table_id, int64_t key, char *value);
char *find(int table_id, int64_t key);
int delete(int table_id, int64_t key);

#define NUM_CHARSET 62
char charset[NUM_CHARSET+1] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";


int table_id;

void set_val(char *buf){
	for (int i = 0; i < 119; i++){
		buf[i] = charset[rand()%NUM_CHARSET];
	}
	buf[119] = 0;
}

int shell(){
	char instruction;
	int64_t k;
	char v[120];
	char *find_ret;
	int ret;

	printf("> ");
	while (scanf("%c", &instruction) != EOF) {
		switch (instruction) {
		case 'd':
			ret = scanf("%lu", &k);
			UNUSED(ret);
			if (delete(table_id, k) != 0){
				printf("Delete Failed\n");
			}
			break;
		case 'i':
			memset(v, 0, sizeof(v));
			ret = scanf("%lu %s", &k, v);
			UNUSED(ret);
			if (insert(table_id, k, v) != 0){
				printf("Insert Failed\n");
			}
			break;
		case 'f':
			ret = scanf("%lu", &k);
			UNUSED(ret);
			if ((find_ret = find(table_id, k)) == NULL){
				printf("Not found\n");
			} else{
				printf("%lu %s\n", k, find_ret);
				free(find_ret);
			}
			break;
		case 'q':
			while (getchar() != (int)'\n');
			return 0;
			break;
		}
		while (getchar() != (int)'\n');
		printf("> ");
	}
	printf("\n");
	return 0;
}

int main(){
	srand((unsigned int)time(NULL));
	init_db(DEF_NUM_BUF);
	table_id = open_table(DEF_DB_PATH);
	shell();
	close_table(table_id);
	shutdown_db();
	return 0;
}
