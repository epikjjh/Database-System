#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "file.h"

HeaderPage dbheader[10];
int dbfile[10];

// Get free page to use.
// If no more free page exist, expand file
off_t get_free_page(int table_id) {
    off_t freepage_offset;
    
    freepage_offset = dbheader[table_id - 1].freelist;
    if (freepage_offset == 0) {
        // No more free page, expand file as twice
        expand_file(dbheader[table_id - 1].num_pages);
        freepage_offset = dbheader[table_id - 1].freelist;
    }
   
    FreePage freepage;
    load_page_from_buffer(table_id, freepage_offset, (Page*)&freepage);
    dbheader[table_id - 1].freelist = freepage.next;
    
    dirty_on(table_id, dbheader[table_id - 1].file_offset);
    
    return freepage_offset;
}

// Put free page to the free list
void put_free_page(int table_id, off_t page_offset) {
    FreePage freepage;
    memset(&freepage, 0, PAGE_SIZE);

    freepage.next = dbheader[table_id - 1].freelist;
    freepage.file_offset = page_offset;
    // Flush? -> Later
    // Check it later
    //flush_page(table_id, (Page*)&freepage);
    dirty_on(table_id, freepage.file_offset);
    
    dbheader[table_id - 1].freelist = page_offset;

    dirty_on(table_id, dbheader[table_id - 1].file_offset);
}

// Expand file pages and prepend them to the free list
void expand_file(int table_id, size_t cnt_page_to_expand) {
    off_t offset = dbheader[table_id - 1].num_pages * PAGE_SIZE;

    if (dbheader[table_id - 1].num_pages > 1024 * 1024) {
        // Test code: do not expand over than 4GB
        assert("Test: you are already having a DB file over than 4GB");
    }
    
    int i;
    for (i = 0; i < cnt_page_to_expand; i++) {
        put_free_page(offset);
        dbheader[table_id - 1].num_pages++;
        offset += PAGE_SIZE;
    }

    dirty_on(table_id, dbheader[table_id - 1].file_offset);
}

void load_page(int table_id, off_t offset, Page* page) {
    lseek(dbfile[table_id - 1], offset, SEEK_SET);
    read(dbfile[table_id - 1], page, PAGE_SIZE);
    page->file_offset = offset;
}

void flush_page(int table_id, Page* page) {
    lseek(dbfile[table_id - 1], page->file_offset, SEEK_SET);
    write(dbfile[table_id - 1], page, PAGE_SIZE);
}
