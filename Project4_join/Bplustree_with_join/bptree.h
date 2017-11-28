#ifndef BPTREE_H
#define BPTREE_H
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "config.h"

#define E_OK 0
#define E_NOT_FOUND 1
#define E_DUP 2
#define E_FULL_TABLE (-1)
#define HPAGE_NUM 0
#define ADDR_NOT_EXIST 0

typedef uint64_t addr;

typedef struct hblock{
	addr free; // 8
	addr root; // 8
	uint64_t num_page; // 8
	uint8_t pad[4072];
} hblock;

typedef struct record{
	int64_t k;
	char v[VALUE_SIZE];
} record;

typedef struct child{
	int64_t k;
	addr v;
} child;

typedef struct nblock{
	addr parent; //8
	int is_leaf; // 4
	int num_keys; // 4
	uint8_t pad[104]; // 104
	union{
		addr sib;
		addr leftmost;
	}u1;
	union{
		child children[NUM_INT_KEY];
		record recs[NUM_LEAF_REC];
	}u2;
#define i_leftmost u1.leftmost
#define l_sib u1.sib
#define i_children u2.children
#define l_recs u2.recs
} nblock;

typedef struct fblock{
	addr next; //8
	uint8_t pad[4088];
} fblock;

typedef struct npage{
	nblock *b;
	int table_id;
	addr offset;
} npage;

typedef struct hpage{
	hblock *b;
	int table_id;
	addr offset;
} hpage;

typedef struct fpage{
	fblock *b;
	int table_id;
	addr offset;
} fpage;

typedef struct bmgr{
	int fd;
} bmgr;

typedef struct page{
	void *b;
	int table_id;
	addr offset;
	int is_dirty;
	int pincnt;
	struct page *lru_next;
	struct page *lru_prev;
	bool is_used; // False when the page is not used from the beginning
} page;

typedef struct bufmgr{
	page *pages;
	page *lru_head;
	page *lru_tail;
	uint64_t num_buf;
	int tot_pincnt;
} bufmgr;

typedef struct table{
	struct conn *c;
	int table_id;
	bmgr bm;
	bool is_used;
} table;

typedef struct conn{
	table tbls[MAX_TABLE];
	bufmgr *bfm;
} conn;

//Connection functions
int open_conn(conn *c, int buf_num);
int close_conn(conn *c);

//Table functions
int open_table_low(conn *c, const char *pathname);
void close_table_low(table *t);

//Disk functions
int open_file(conn *c, const char *file_path);
void close_file(table *t);
void extend_file(table *t, hpage *hp);
void read_block(table *t, void *p, addr ad);
addr alloc_block(table *t);
void free_block(table *t, void *b);
void write_block(table *t, void *b, addr ad);
void panic(const char *str) __attribute((noreturn));

// Buffer managing functions
void pop_from_lru(table *t, page *p);
void push_to_lru(table *t, page *p);
page *evict_page(table *t);
page *alloc_page(table *t, addr ad);
page *get_page(table *t, addr ad);
void flush_page(table *t);
int init_bufmgr(conn *c, int buf_num);
void close_bufmgr(conn *c);

//Helper functions
npage *find_leaf(table *t, const int64_t k);
int find_rec(table *t, npage *np, const int64_t k);
int find_low(table *t, const int64_t k, record *r);
void print_tree(table *t);
int cut( int length );
npage *get_root(table *t);
npage *get_child(table *t, npage *np, int idx);
npage *get_parent(table *t, npage *np);
npage *get_npage(table *t, addr ad);
hpage *get_hpage(table *t);
fpage *get_fpage(table *t, addr ad);
npage *alloc_npage(table *t, addr ad);
hpage *alloc_hpage(table *t);
fpage *alloc_fpage(table *t, addr ad);

//Insert functions
npage *make_node(table *t);
npage *make_leaf(table *t);
void set_root(table *t, npage *np);
int insert_into_leaf(table *t, npage *leaf, const record *r);
int insert_into_new_root(table *t, npage *left, int64_t k, npage *right);
int get_left_index(npage *parent, npage *left);
int insert_into_node(table *t, npage *np, 
		int left_index, int64_t k, npage *right);
int insert_into_node_after_splitting(table *t, npage *np, int left_index, 
		int64_t k, npage *right);
int insert_into_parent(table *t, npage *left, int64_t k, npage *right);
int insert_into_leaf_after_splitting(table *t, npage *np, record *r);
int insert_low( table *t, record *r);

//Delete functions
int remove_entry_from_node(table *t, npage *np, int64_t k, int idx);
int adjust_root(table *t, npage *root);
int coalesce_nodes(table *t, npage *np, npage *neighbor, int neighbor_index, int k_prime);
int get_neighbor_index(table *t, npage *np, npage *parent);
int redistribute_nodes(table *t, npage *np, npage *neighbor, int neighbor_index, 
		int k_prime_index, int64_t k_prime); 
int delete_entry(table *t, npage *np, int64_t k, int idx);
int delete_low(table *t, int64_t k); 

//Macro
#define DEC_RET int ret = 0
#define RET(x) do{\
	if ((ret = (x)) != 0) \
		return ret;\
}while(0)

#define ALIGNED(x) ((x) % BLOCK_SIZE == 0)
#define ALIGN_UP(x, size) (((x-1) & ~((size)-1)) + (size))
#define ALIGN_DOWN(x, size) ((x) & ~((size)-1))
#define B(x) ((x)->b)

#define read_page(t, p) read_block(t, B(p), (p)->offset)
#define write_page(t, p) write_block(t, B(p), (p)->offset)

#define set_dirty(p) do{\
	((page*)(p))->is_dirty = true;\
}while(0)

#define release_page(t, p) do{\
	((page*)(p))->pincnt--;\
	(t)->c->bfm->tot_pincnt--;\
}while(0)

#define update_lru(t, p) do{\
	pop_from_lru(t, p);\
	push_to_lru(t, p);\
}while(0)

#endif /* BPTREE_H */
