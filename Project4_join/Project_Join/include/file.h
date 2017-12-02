#include <stddef.h>
#include <inttypes.h>

#define BPTREE_INTERNAL_ORDER       249 //4
#define BPTREE_LEAF_ORDER           32  //4

#define PAGE_SIZE                   4096

#define SIZE_KEY                    8
#define SIZE_VALUE                  120
#define SIZE_RECORD                 (SIZE_KEY + SIZE_VALUE)

#define BPTREE_MAX_NODE             (1024 * 1024) // for queue

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct _Record {
    uint64_t key;
    char value[SIZE_VALUE];
} Record;

typedef struct _InternalRecord {
    uint64_t key;
    off_t offset;
} InternalRecord;

typedef struct _Page {
    char bytes[PAGE_SIZE];
    
    // in-memory data
    off_t file_offset;
} Page;

typedef struct _FreePage {
    off_t next;
    char reserved[PAGE_SIZE - 8];

    // in-memory data
    off_t file_offset;
} FreePage;

typedef struct _HeaderPage {
    off_t freelist;
    off_t root_offset;
    uint64_t num_pages;
    char reserved[PAGE_SIZE - 24];

    // in-memory data
    off_t file_offset;
} HeaderPage;

#define INTERNAL_KEY(n, i)    ((n)->irecords[(i)+1].key)
#define INTERNAL_OFFSET(n, i) ((n)->irecords[(i)].offset)
typedef struct _InternalPage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
            char reserved[112 - 16];
            InternalRecord irecords[BPTREE_INTERNAL_ORDER];
        };
        char space[PAGE_SIZE];
    };
    // in-memory data
    off_t file_offset;
} InternalPage;

#define LEAF_KEY(n, i)      ((n)->records[(i)].key)
#define LEAF_VALUE(n, i)    ((n)->records[(i)].value)
typedef struct _LeafPage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
            char reserved[120 - 16];
            off_t sibling;
            Record records[BPTREE_LEAF_ORDER-1];
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    off_t file_offset;
} LeafPage;

typedef struct _NodePage {
    union {
        struct {
            off_t parent;
            int is_leaf;
            int num_keys;
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    off_t file_offset;
} NodePage;

// Open a db file. Create a file if not exist.
int open_table(const char* filename);

// Close a db file
// Not used in project buffer : Replaced by close_table function
void close_db(int table_id);

// Get free page to use
off_t get_free_page(int table_id);

// Put free page to the free list
void put_free_page(int table_id, off_t page_offset);

// Expand file size and prepend pages to the free list
void expand_file(int table_id, size_t cnt_page_to_expand);

// Load file page into the in-memory page
void load_page(int table_id, off_t offset, Page* page);

// Flush page into the file
void flush_page(int table_id, Page* page);

extern HeaderPage dbheader[10];

/* Project Buffer */
// Buffer structure
typedef struct _Buffer{
    Page *frame;
    int table_id;
    off_t page_offset;
    int is_dirty;
    // LRU clock structure
    int refbit;
} Buffer;

// Load function
void load_page_from_buffer(int table_id, off_t offset, Page* page);

// For load function
Page* check_buffer_for_load(int table_id, off_t offset);

// For flush function
int check_buffer_for_flush(int table_id, off_t offset);


int replace_page(int table_id);

// Flush function
void flush_page_to_buffer(int table_id, Page* page);


/* Project Join */
int join_table(int table_id_1, int table_id_2, char *pathname);

void table_info(int table_id, int *num_keys, int *max_key);
