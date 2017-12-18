#include <stddef.h>
#include <inttypes.h>

#define BPTREE_INTERNAL_ORDER       249 //4
#define BPTREE_LEAF_ORDER           32  //4

#define PAGE_SIZE                   4096

#define SIZE_KEY                    8
#define SIZE_VALUE                  120
#define SIZE_RECORD                 (SIZE_KEY + SIZE_VALUE)

#define BPTREE_MAX_NODE             (1024 * 1024) // for queue

#define OUTPUT_ORDER                16
#define SIZE_LOG                    280

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

/* Project Join */
typedef struct _Result {
    uint64_t key1;
    char value1[SIZE_VALUE];
    uint64_t key2;
    char value2[SIZE_VALUE];
} Result;

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
    off_t page_lsn;
    char reserved[PAGE_SIZE - 32];

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
            char reserved_1[8];
            off_t page_lsn;
            char reserved_2[112 - 32];
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
            char reserved_1[8];
            off_t page_lsn;
            char reserved_2[120 - 32];
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
            char reserved_1[8];
            off_t page_lsn;
        };
        char space[PAGE_SIZE];
    };

    // in-memory data
    off_t file_offset;
} NodePage;

/* Project Join */
#define RESULT_KEY1(n, i)      ((n)->results[(i)].key1)
#define RESULT_VALUE1(n, i)      ((n)->results[(i)].value1)
#define RESULT_KEY2(n, i)      ((n)->results[(i)].key2)
#define RESULT_VALUE2(n, i)      ((n)->results[(i)].value2)
typedef struct _OutputPage {
    union {
        Result results[OUTPUT_ORDER];
        char space[PAGE_SIZE];
    };
    
    //index
    off_t file_offset;
} OutputPage;

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

int replace_page(FILE *file);

// Flush function
void flush_page_to_buffer(int table_id, Page* page);

/* Project Join */
int join_table(int table_id_1, int table_id_2, char *pathname);

void table_info(int table_id, uint64_t *num_keys, uint64_t *min_key, uint64_t *max_key);

void write_output_buffer(FILE *file, uint64_t key1, char *value1, uint64_t key2, char *value2);

int load_output_page(Page *page);

void flush_output_page(FILE *file, Page *page);

void sync_buffer(FILE *file);

/* Project recovery */
typedef struct _LogRecord{
    struct{
        off_t lsn;              // End offset of a current log record.
        off_t prev_lsn;         // LSN of the previous log record.
        int xid;                // Indicates the transaction that triggers current log record.
        int type;               /* The type of current log record.
                                    BEGIN : 0
                                    UPDATE : 1
                                    COMMIT : 2
                                    ABORT : 3 */

        int table_id;           // Indicates the data file.
        int pnum;               // Page that contains th modified area.
        int offset;             // Start offset of the modified area within a page.
        int length;             // The length of modified area.
        char old_image[120];    // Old contents of the modified area.
        char new_image[120];    // New contents of the modified area.
    };
}LogRecord;

// Allocate transaction structure and initialize it.
// Return 0 if success, otherwise return non-zero value.
int begin_transaction();

// Return 0 if success, otherwise return non-zero value.
// User can get response once all modification of transaction are flushed to a log file.
// If user get successful return, that means your database can recover committed transaction after system crash.
int commit_transaction();

// Return 0 if success, otherwise return non-zero value.
// All affected modification should be canceled and return to old state.
int abor_transaction();

// Find the matching key and modify the value, where value size <= 120 Bytes.
// Retrun 0 if success, otherwise return non-zero value.
int update(int table_id, int64_t key, char *value);

void create_log(int type, int table_id, int pnum, int offset, int length, char *old_image, char *new_image);

void flush_log(int size);

void recovery();

void execute_wal();
