#include "bptree.h"

static conn c;

int init_db(uint64_t num_buf){
	DEC_RET;
	RET(open_conn(&c, num_buf));
	return ret;
}

int shutdown_db(){
	close_conn(&c);
	return 0;
}

int open_table(char *pathname){
	int table_id = open_table_low(&c, pathname);

#ifdef VERBOSE_TREE
	if (table_id >= 0)
		print_tree(&c.tbls[table_id]);
#endif
	return table_id;
}

int close_table(int table_id){
	close_table_low(&c.tbls[table_id]);
	return 0;
}

int insert(int table_id, int64_t key, char *value){
	DEC_RET;
	record r;
	r.k = key;
	memcpy(r.v, value, VALUE_SIZE);
	RET(insert_low(&c.tbls[table_id], &r));
#ifdef DEBUG_TREE
	printf("%d\n", c.bfm->tot_pincnt);
#endif
#ifdef VERBOSE_TREE
	print_tree(&c.tbls[table_id]);
#endif
	return ret;
}

char *find(int table_id, int64_t key){
	record r;
	char *ret = (char *)malloc(sizeof(char)*VALUE_SIZE);
	if (find_low(&c.tbls[table_id], key, &r) != 0){
		return NULL;
	}
#ifdef DEBUG_TREE
	printf("%d\n", c.bfm->tot_pincnt);
#endif
	memcpy(ret, r.v, VALUE_SIZE); 
	return ret;
}

int delete(int table_id, int64_t key){
	DEC_RET;
	RET(delete_low(&c.tbls[table_id], key));
#ifdef DEBUG_TREE
	printf("%d\n", c.bfm->tot_pincnt);
#endif
#ifdef VERBOSE_TREE
	print_tree(&c.tbls[table_id]);
#endif
	return ret;
}
