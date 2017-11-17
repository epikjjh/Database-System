#ifndef __BPT_H__
#define __BPT_H__

int open_table(const char* filename);
char* find(int table_id, uint64_t key);
int insert(int table_id, uint64_t key, const char* value);
int delete(int table_id, uint64_t key);

void print_tree();

/* Project Buffer */
// Function
int init_db(int num_buf);

int close_table(int table_id);

int shutdown_db();
#endif // __BPT_H__
