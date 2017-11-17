#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "file.h"

HeaderPage dbheader;
int dbfile;

// Get free page to use.
// If no more free page exist, expand file
off_t get_free_page() {
    off_t freepage_offset;
    
    freepage_offset = dbheader.freelist;
    if (freepage_offset == 0) {
        // No more free page, expand file as twice
        expand_file(dbheader.num_pages);
        freepage_offset = dbheader.freelist;
    }
   
    FreePage freepage;
    load_page(freepage_offset, (Page*)&freepage);
    dbheader.freelist = freepage.next;
    
    flush_page((Page*)&dbheader);
    
    return freepage_offset;
}

// Put free page to the free list
void put_free_page(off_t page_offset) {
    FreePage freepage;
    memset(&freepage, 0, PAGE_SIZE);

    freepage.next = dbheader.freelist;
    freepage.file_offset = page_offset;
    flush_page((Page*)&freepage);
    
    dbheader.freelist = page_offset;

    flush_page((Page*)&dbheader);
}

// Expand file pages and prepend them to the free list
void expand_file(size_t cnt_page_to_expand) {
    off_t offset = dbheader.num_pages * PAGE_SIZE;

    if (dbheader.num_pages > 1024 * 1024) {
        // Test code: do not expand over than 4GB
        assert("Test: you are already having a DB file over than 4GB");
    }
    
    int i;
    for (i = 0; i < cnt_page_to_expand; i++) {
        put_free_page(offset);
        dbheader.num_pages++;
        offset += PAGE_SIZE;
    }

    flush_page((Page*)&dbheader);
}

void load_page(off_t offset, Page* page) {
    lseek(dbfile, offset, SEEK_SET);
    read(dbfile, page, PAGE_SIZE);
    page->file_offset = offset;
}

void flush_page(Page* page) {
    lseek(dbfile, page->file_offset, SEEK_SET);
    write(dbfile, page, PAGE_SIZE);
}
