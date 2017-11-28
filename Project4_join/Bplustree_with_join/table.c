#include "bptree.h"

/* Open new table
 */
int open_table_low(conn *c, const char *pathname){
	hpage *hp;
	table *t;
	int tid;
	if (access( pathname, F_OK ) != -1){
		tid = open_file(c, pathname);
	}
	else{
		tid = open_file(c, pathname);
		t = &c->tbls[tid];

		hp = alloc_hpage(t);
		set_dirty(hp);
		B(hp)->root = ADDR_NOT_EXIST;
		B(hp)->free = ADDR_NOT_EXIST;
		B(hp)->num_page = 1;
		release_page(t, hp);
	}
	return tid;
}

/* Close the table
 */
void close_table_low(table *t){
	flush_page(t);
	close_file(t);
}
