#ifndef __BPT_H__
#define __BPT_H__

int open_table(const char* filename);
char* find(int table_id, uint64_t key);
int insert(int table_id, uint64_t key, const char* value);
int delete(int table_id, uint64_t key);

void print_tree(int table_id);

/* Project Buffer */
// Function
int init_db(int num_buf);

int close_table(int table_id);

int shutdown_db();

// Load function
void load_page_from_buffer(int table_id, off_t offset, Page* page);

Page* is_in_buffer(int table_id, off_t offset, int type);

int replace_page(int table_id);

// Flush function
void dirty_on(int table_id, Page* page);

#endif // __BPT_H__
