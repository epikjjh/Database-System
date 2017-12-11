/*
 *  bpt.c  
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include "bpt.h"
#include "file.h"
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

//#define TEST_MAIN 1

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 256

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// GLOBALS.
extern HeaderPage dbheader[10];
extern int dbfile[10];
static int table_ids[10] = {0,};

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
int order_internal = BPTREE_INTERNAL_ORDER;
int order_leaf = BPTREE_LEAF_ORDER;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = false;

/* Project Buffer : GLOBALS */
Buffer *buf_mgr;
int buf_size = -1;
int clock_hand = 0;
int target_buf = 0;

// FUNCTION PROTOTYPES.

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void find_and_print(int table_id, uint64_t key); 
bool find_leaf(int table_id, uint64_t key, LeafPage* out_leaf_node);

// Insertion.
void start_new_tree(int table_id, uint64_t key, const char* value);
void insert_into_leaf(int talbe_id, LeafPage* leaf_node, uint64_t key, const char* value);
void insert_into_leaf_after_splitting(int table_id, LeafPage* leaf_node, uint64_t key, const char* value);
void insert_into_parent(int table_id, NodePage* left, uint64_t key, NodePage* right);
void insert_into_new_root(int table_id, NodePage* left, uint64_t key, NodePage* right);
int get_left_index(InternalPage* parent, off_t left_offset);
void insert_into_node(InternalPage * parent, int left_index, uint64_t key, off_t right_offset);
void insert_into_node_after_splitting(int table_id, InternalPage* parent, int left_index, uint64_t key, off_t right_offset);

// Deletion.
int get_neighbor_index(int table_id, NodePage* node_page);
void adjust_root(int table_id);
void coalesce_nodes(int table_id, NodePage* node_page, NodePage* neighbor_page,
                      int neighbor_index, int k_prime);
void redistribute_nodes(int table_id, NodePage* node_page, NodePage* neighbor_page,
                          int neighbor_index,
                          int k_prime_index, int k_prime);
void delete_entry(int table_id, NodePage* node_page, uint64_t key);


// FUNCTION DEFINITIONS.

// OUTPUT AND UTILITIES

/* Copyright and license notice to user at startup. 
 */
void license_notice( void ) {
	printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram "
			"http://www.amittai.com\n", Version);
	printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
			"type `show w'.\n"
			"This is free software, and you are welcome to redistribute it\n"
			"under certain conditions; type `show c' for details.\n\n");
}


/* Routine to print portion of GPL license to stdout.
 */
void print_license( int license_part ) {
	int start, end, line;
	FILE * fp;
	char buffer[0x100];

	switch(license_part) {
	case LICENSE_WARRANTEE:
		start = LICENSE_WARRANTEE_START;
		end = LICENSE_WARRANTEE_END;
		break;
	case LICENSE_CONDITIONS:
		start = LICENSE_CONDITIONS_START;
		end = LICENSE_CONDITIONS_END;
		break;
	default:
		return;
	}

	fp = fopen(LICENSE_FILE, "r");
	if (fp == NULL) {
		perror("print_license: fopen");
		exit(EXIT_FAILURE);
	}
	for (line = 0; line < start; line++)
		fgets(buffer, sizeof(buffer), fp);
	for ( ; line < end; line++) {
		fgets(buffer, sizeof(buffer), fp);
		printf("%s", buffer);
	}
	fclose(fp);
}

/* First message to the user.
 */
void usage_1( void ) {
	printf("B+ Tree of Order %d(Internal).\n", order_internal);
    printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
           "5th ed.\n\n"
           "To build a B+ tree of a different order, start again and enter "
           "the order\n"
           "as an integer argument:  bpt <order>  ");
	printf("(%d <= order <= %d).\n", MIN_ORDER, MAX_ORDER);
    printf("To start with input from a file of newline-delimited integers, \n"
           "start again and enter the order followed by the filename:\n"
           "bpt <order> <inputfile> .\n");
}

/* Second message to the user.
 */
void usage_2( void ) {
	printf("Enter any of the following commands after the prompt > :\n"
	"\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
	"\tf <k>  -- Find the value under key <k>.\n"
	"\tp <k> -- Print the path from the root to key k and its associated "
           "value.\n"
	"\tr <k1> <k2> -- Print the keys and values found in the range "
			"[<k1>, <k2>\n"
	"\td <k>  -- Delete key <k> and its associated value.\n"
	"\tx -- Destroy the whole tree.  Start again with an empty tree of the "
           "same order.\n"
	"\tt -- Print the B+ tree.\n"
	"\tl -- Print the keys of the leaves (bottom row of the tree).\n"
	"\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
           "leaves.\n"
	"\tq -- Quit. (Or use Ctl-D.)\n"
	"\t? -- Print this help message.\n");
}

// Open a db file. Create a file if not exist.
int open_table(const char* filename) {
    int i;

    // Table capacitance check.
    for(i = 0; i < 10; i++){
        if(table_ids[i] == 0){
            table_ids[i] = 1;
            break;
        }
    }

    // Over table. ( MAX : 10 )
    if(i == 10){
        return -1;
    }

    dbfile[i] = open(filename, O_RDWR);
    if (dbfile[i] < 0) {
        // Create a new db file
        dbfile[i] = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if (dbfile[i] < 0) {
            assert("failed to create new db file");
            return -1;
        }
        
        memset((dbheader+i), 0, PAGE_SIZE);
        dbheader[i].freelist = 0;
        dbheader[i].root_offset = 0;
        dbheader[i].num_pages = 1;
        dbheader[i].file_offset = 0;
        flush_page_to_buffer(i+1, (Page*)(dbheader + i));
    } else {
        // DB file exist. Load header info
        load_page_from_buffer(i+1, 0, (Page*)(dbheader+i));

        // In case of empty page.
        if(dbheader[i].num_pages == 0){
            dbheader[i].num_pages = 1;
            flush_page_to_buffer(i+1, (Page*)(dbheader + i));
        }
        dbheader[i].file_offset = 0;
    }

    return i+1;
}

// Not used in project buffer : Replaced by close_table function
void close_db(int table_id) {
    close(dbfile[table_id - 1]);
}

/* Helper function for printing the
 * tree out.  See print_tree.
 */
/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */

off_t queue[BPTREE_MAX_NODE];
void print_tree(int table_id) {

    int i;
    int front = 0;
    int rear = 0;

    if (dbheader[table_id - 1].root_offset == 0) {
		printf("Empty tree.\n");
        return;
    }

    queue[rear] = dbheader[table_id - 1].root_offset;
    rear++;
    queue[rear] = 0;
    rear++;
    while (front < rear) {
        off_t page_offset = queue[front];
        front++;

        if (page_offset == 0) {
            printf("\n");
            
            if (front == rear) break;

            // next tree level
            queue[rear] = 0;
            rear++;
            continue;
        }
        
        NodePage node_page;
        load_page_from_buffer(table_id, page_offset, (Page*)&node_page);
        if (node_page.is_leaf == 1) {
            // leaf node
            LeafPage* leaf_node = (LeafPage*)&node_page;
            for (i = 0; i < leaf_node->num_keys; i++) {
                printf("%" PRIu64 " ", LEAF_KEY(leaf_node, i));
            }
            printf("| ");
        } else {
            // internal node
            InternalPage* internal_node = (InternalPage*)&node_page;
            for (i = 0; i < internal_node->num_keys; i++) {
                printf("%" PRIu64 " ", INTERNAL_KEY(internal_node, i));
                queue[rear] = INTERNAL_OFFSET(internal_node, i);
                rear++;
            }
            queue[rear] = INTERNAL_OFFSET(internal_node, i);
            rear++;
            printf("| ");
        }
    }
}

/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
void find_and_print(int table_id, uint64_t key) {
    char* value_found = NULL;
	value_found = find(table_id, key);
	if (value_found == NULL) {
		printf("Record not found under key %" PRIu64 ".\n", key);
    }
	else {
		printf("key %" PRIu64 ", value [%s].\n", key, value_found);
        free(value_found);
    }
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
bool find_leaf(int table_id, uint64_t key, LeafPage* out_leaf_node) {
    int i = 0;
    off_t root_offset = dbheader[table_id - 1].root_offset;

	if (root_offset == 0) {
		return false;
	}
    
    NodePage page;
    load_page_from_buffer(table_id, root_offset, (Page*)&page);

	while (!page.is_leaf) {
        InternalPage* internal_node = (InternalPage*)&page;

        i = 0;
		while (i < internal_node->num_keys) {
			if (key >= INTERNAL_KEY(internal_node, i)) i++;
			else break;
		}
        
        load_page_from_buffer(table_id, INTERNAL_OFFSET(internal_node, i), (Page*)&page);
	}

    memcpy(out_leaf_node, &page, sizeof(LeafPage));

	return true;
}

/* Finds and returns the record to which
 * a key refers.
 */
// If you want to return a record, use 3rd parameter
char* find(int table_id, uint64_t key) {
    int i = 0;
    char* out_value;

    LeafPage leaf_node;
    if (!find_leaf(table_id, key, &leaf_node)) {
        return NULL;
    }

	for (i = 0; i < leaf_node.num_keys; i++) {
		if (LEAF_KEY(&leaf_node, i) == key) {
            out_value = (char*)malloc(SIZE_VALUE * sizeof(char));
            memcpy(out_value, LEAF_VALUE(&leaf_node, i), SIZE_VALUE);
            return out_value;
        }
    }

    return NULL;
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
	if (length % 2 == 0)
		return length/2;
	else
		return length/2 + 1;
}

// INSERTION
/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(InternalPage* parent, off_t left_offset) {

	int left_index = 0;
	while (left_index <= parent->num_keys && 
			INTERNAL_OFFSET(parent, left_index) != left_offset)
		left_index++;
	return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
void insert_into_leaf(int table_id, LeafPage* leaf_node, uint64_t key, const char* value) {
	int insertion_point;
    int i;

	insertion_point = 0;
	while (insertion_point < leaf_node->num_keys &&
           LEAF_KEY(leaf_node, insertion_point) < key)
		insertion_point++;

    // shift keys and values to the right
    for (i = leaf_node->num_keys - 1; i >= insertion_point; i--) {
        LEAF_KEY(leaf_node, i+1) = LEAF_KEY(leaf_node, i);
        memcpy(LEAF_VALUE(leaf_node, i+1), LEAF_VALUE(leaf_node, i), SIZE_VALUE);
    }
    
    LEAF_KEY(leaf_node, insertion_point) = key;
    memcpy(LEAF_VALUE(leaf_node, insertion_point), value, SIZE_VALUE);
	leaf_node->num_keys++;

    // flush leaf node to the file page
    flush_page_to_buffer(table_id, (Page*)leaf_node);
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting(int table_id, LeafPage* leaf, uint64_t key, const char* value) {

	int insertion_index, split, i, j;
    uint64_t new_key;

    // make a new leaf node
    LeafPage new_leaf;
    new_leaf.is_leaf = true;
    new_leaf.num_keys = 0;

    insertion_index = 0;
    while (insertion_index < order_leaf - 1 && LEAF_KEY(leaf, insertion_index) < key)
		insertion_index++;

	split = cut(order_leaf - 1);

    if (insertion_index < split) {
        // new key is going to inserted to the old leaf
        for (i = split - 1, j = 0; i < order_leaf - 1; i++, j++) {
            LEAF_KEY(&new_leaf, j) = LEAF_KEY(leaf, i);
            memcpy(LEAF_VALUE(&new_leaf, j), LEAF_VALUE(leaf, i), SIZE_VALUE);

            new_leaf.num_keys++;
            leaf->num_keys--;
        }
        // shift keys to make space for new record
        for (i = split - 2; i >= insertion_index; i--) {
            LEAF_KEY(leaf, i+1) = LEAF_KEY(leaf, i);
            memcpy(LEAF_VALUE(leaf, i+1), LEAF_VALUE(leaf, i), SIZE_VALUE);
        }
        LEAF_KEY(leaf, insertion_index) = key;
        memcpy(LEAF_VALUE(leaf, insertion_index), value, SIZE_VALUE);
        leaf->num_keys++;
    } else {
        // new key is going to inserted to the new leaf
        for (i = split, j = 0; i < order_leaf - 1; i++, j++) {
            if (i == insertion_index) {
                // make space for new record
                j++;
            }
            LEAF_KEY(&new_leaf, j) = LEAF_KEY(leaf, i);
            memcpy(LEAF_VALUE(&new_leaf, j), LEAF_VALUE(leaf, i), SIZE_VALUE);

            new_leaf.num_keys++;
            leaf->num_keys--;
        }
        LEAF_KEY(&new_leaf, insertion_index - split) = key;
        memcpy(LEAF_VALUE(&new_leaf, insertion_index - split), value, SIZE_VALUE);
        new_leaf.num_keys++;
    }
   
    // allocate a page for new leaf
    new_leaf.file_offset = get_free_page(table_id);

    // linked-list of leaves
	new_leaf.sibling = leaf->sibling;
	leaf->sibling = new_leaf.file_offset;
   
    // clear garbage records
	for (i = leaf->num_keys; i < order_leaf - 1; i++) {
		LEAF_KEY(leaf, i) = 0;
        memset(LEAF_VALUE(leaf, i), 0, SIZE_VALUE);
    }
	for (i = new_leaf.num_keys; i < order_leaf - 1; i++) {
		LEAF_KEY(&new_leaf, i) = 0;
        memset(LEAF_VALUE(&new_leaf, i), 0, SIZE_VALUE);
    }

	new_leaf.parent = leaf->parent;

    flush_page_to_buffer(table_id, (Page*)leaf);
    flush_page_to_buffer(table_id, (Page*)&new_leaf);

	new_key = LEAF_KEY(&new_leaf, 0);

    // insert new key and new leaf to the parent
	insert_into_parent(table_id, (NodePage*)leaf, new_key, (NodePage*)&new_leaf);
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(InternalPage* n, int left_index, uint64_t key, off_t right_offset) {
    int i;

	for (i = n->num_keys; i > left_index; i--) {
		INTERNAL_OFFSET(n, i + 1) = INTERNAL_OFFSET(n, i);
		INTERNAL_KEY(n, i) = INTERNAL_KEY(n, i - 1);
	}
	INTERNAL_OFFSET(n, left_index + 1) = right_offset;
	INTERNAL_KEY(n, left_index) = key;
	n->num_keys++;
}

/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
void insert_into_node_after_splitting(int table_id, InternalPage* old_node, int left_index, uint64_t key, off_t right_offset) {
    int i, j, split, k_prime;
	uint64_t* temp_keys;
	off_t* temp_pointers;

	/* First create a temporary set of keys and pointers
	 * to hold everything in order, including
	 * the new key and pointer, inserted in their
	 * correct places. 
	 * Then create a new node and copy half of the 
	 * keys and pointers to the old node and
	 * the other half to the new.
	 */

	temp_pointers = malloc( (order_internal + 1) * sizeof(off_t) );
	if (temp_pointers == NULL) {
		perror("Temporary pointers array for splitting nodes.");
		exit(EXIT_FAILURE);
	}
	temp_keys = malloc( order_internal * sizeof(uint64_t) );
	if (temp_keys == NULL) {
		perror("Temporary keys array for splitting nodes.");
		exit(EXIT_FAILURE);
	}

	for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
		if (j == left_index + 1) j++;
		temp_pointers[j] = INTERNAL_OFFSET(old_node, i);
	}

	for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
		if (j == left_index) j++;
		temp_keys[j] = INTERNAL_KEY(old_node, i);
	}

	temp_pointers[left_index + 1] = right_offset;
	temp_keys[left_index] = key;

	/* Create the new node and copy
	 * half the keys and pointers to the
	 * old and half to the new.
	 */  
	split = cut(order_internal);

    InternalPage new_node;
	new_node.num_keys = 0;
    new_node.is_leaf = 0;
    new_node.file_offset = get_free_page(table_id);

    old_node->num_keys = 0;
	for (i = 0; i < split - 1; i++) {
		INTERNAL_OFFSET(old_node, i) = temp_pointers[i];
		INTERNAL_KEY(old_node, i) = temp_keys[i];
		old_node->num_keys++;
	}
	INTERNAL_OFFSET(old_node, i) = temp_pointers[i];
	k_prime = temp_keys[split - 1];
	for (++i, j = 0; i < order_internal; i++, j++) {
		INTERNAL_OFFSET(&new_node, j) = temp_pointers[i];
		INTERNAL_KEY(&new_node, j) = temp_keys[i];
		new_node.num_keys++;
	}
	INTERNAL_OFFSET(&new_node, j) = temp_pointers[i];
	free(temp_pointers);
	free(temp_keys);
	new_node.parent = old_node->parent;
	for (i = 0; i <= new_node.num_keys; i++) {
		NodePage child_page;
        load_page_from_buffer(table_id, INTERNAL_OFFSET(&new_node, i), (Page*)&child_page);
        child_page.parent = new_node.file_offset;
        flush_page_to_buffer(table_id, (Page*)&child_page);
    }

    // clear garbage record
    for (i = old_node->num_keys; i < order_internal - 1; i++) {
        INTERNAL_OFFSET(old_node, i+1) = 0;
        INTERNAL_KEY(old_node, i) = 0;
    }

    for (i = new_node.num_keys; i < order_internal - 1; i++) {
        INTERNAL_OFFSET(&new_node, i+1) = 0;
        INTERNAL_KEY(&new_node, i) = 0;
    }

    // flush old, new node
    flush_page_to_buffer(table_id, (Page*)&new_node);
    flush_page_to_buffer(table_id, (Page*)old_node);

	/* Insert a new key into the parent of the two
	 * nodes resulting from the split, with
	 * the old node to the left and the new to the right.
	 */
	insert_into_parent(table_id, (NodePage*)old_node, k_prime, (NodePage*)&new_node);
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
void insert_into_parent(int table_id, NodePage* left, uint64_t key, NodePage* right) {
	//off_t left_index;
    InternalPage parent_node;

    /* Case: new root. */
	if (left->parent == 0) {
		insert_into_new_root(table_id, left, key, right);
        return;
    }

    load_page_from_buffer(table_id, left->parent, (Page*)&parent_node);

	/* Case: leaf or node. (Remainder of
	 * function body.)  
	 */

	/* Find the parent's pointer to the left 
	 * node.
	 */

	int left_index = get_left_index(&parent_node, left->file_offset);

	/* Simple case: the new key fits into the node. 
	 */

	if (parent_node.num_keys < order_internal - 1) {
		insert_into_node(&parent_node, left_index, key, right->file_offset);
        flush_page_to_buffer(table_id, (Page*)&parent_node);
        return;
    }

	/* Harder case:  split a node in order 
	 * to preserve the B+ tree properties.
	 */

	return insert_into_node_after_splitting(table_id, &parent_node, left_index, key, right->file_offset);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(int table_id, NodePage* left, uint64_t key, NodePage* right) {
    // make new root node
    InternalPage root_node;
    memset(&root_node, 0, sizeof(InternalPage));
    root_node.file_offset = get_free_page(table_id);
    INTERNAL_KEY(&root_node, 0) = key;
    INTERNAL_OFFSET(&root_node, 0) = left->file_offset;
    INTERNAL_OFFSET(&root_node, 1) = right->file_offset;
    root_node.num_keys++;
    root_node.parent = 0;
    root_node.is_leaf = 0;
    left->parent = root_node.file_offset;
    right->parent = root_node.file_offset;

    flush_page_to_buffer(table_id, (Page*)&root_node);
    flush_page_to_buffer(table_id, (Page*)left);
    flush_page_to_buffer(table_id, (Page*)right);

    dbheader[table_id - 1].root_offset = root_node.file_offset;
    flush_page_to_buffer(table_id, (Page*)(dbheader + table_id - 1));
}
/* First insertion:
 * start a new tree.
 */
void start_new_tree(int table_id, uint64_t key, const char* value) {
    LeafPage root_node;
    
    off_t root_offset = get_free_page(table_id);
    root_node.file_offset = root_offset;

    root_node.parent = 0;
    root_node.is_leaf = 1;
    root_node.num_keys = 1;
    LEAF_KEY(&root_node, 0) = key;
    root_node.sibling = 0;
    memcpy(LEAF_VALUE(&root_node, 0), value, SIZE_VALUE);
    
    flush_page_to_buffer(table_id, (Page*)&root_node);

    dbheader[table_id - 1].root_offset = root_offset;
    flush_page_to_buffer(table_id, (Page*)(dbheader + table_id - 1));
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int insert(int table_id, uint64_t key, const char* value) {
    /* The current implementation ignores
	 * duplicates.
	 */
    char* value_found = NULL;

    if ((value_found = find(table_id, key)) != 0) {
        free(value_found);
        return -1;
    }
	/* Case: the tree does not exist yet.
	 * Start a new tree.
	 */

	if (dbheader[table_id - 1].root_offset == 0) {
		start_new_tree(table_id, key, value);
        return 0;
    }
	
    /* Case: the tree already exists.
	 * (Rest of function body.)
	 */

    LeafPage leaf_node;
    find_leaf(table_id, key, &leaf_node);

	/* Case: leaf has room for key and pointer.
	 */

	if (leaf_node.num_keys < order_leaf - 1) {
        insert_into_leaf(table_id, &leaf_node, key, value);
	} else {
    	/* Case:  leaf must be split.
	     */
        insert_into_leaf_after_splitting(table_id, &leaf_node, key, value);
    }
    return 0;
}

// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(int table_id, NodePage* node_page) {

	int i;

	/* Return the index of the key to the left
	 * of the pointer in the parent pointing
	 * to n.  
	 * If n is the leftmost child, this means
	 * return -1.
	 */
    InternalPage parent_node;
    load_page_from_buffer(table_id, node_page->parent, (Page*)&parent_node);
	for (i = 0; i <= parent_node.num_keys; i++)
		if (INTERNAL_OFFSET(&parent_node, i) == node_page->file_offset)
			return i - 1;

	// Error state.
    assert("Search for nonexistent pointer to node in parent.");
    return -1;
}

void remove_entry_from_node(int table_id, NodePage* node_page, uint64_t key) {

	int i;
    int key_idx = 0;

    if (node_page->is_leaf) {
        LeafPage* leaf_node = (LeafPage*)node_page;

        // find a slot of deleting key
        for (i = 0; i < leaf_node->num_keys; i++) {
            if (LEAF_KEY(leaf_node, i) == key) {
                key_idx = i;
                break;
            }
        }
        if (i == leaf_node->num_keys) {
            assert("remove_entry_from_node: no key in this page");
        }

        // shift records
        for (i = key_idx; i < leaf_node->num_keys - 1; i++) {
            LEAF_KEY(leaf_node, i) = LEAF_KEY(leaf_node, i+1);
            memcpy(LEAF_VALUE(leaf_node, i), LEAF_VALUE(leaf_node, i+1), SIZE_VALUE);
        }
        // clear garbage record
        LEAF_KEY(leaf_node, leaf_node->num_keys - 1) = 0;
        memset(LEAF_VALUE(leaf_node, leaf_node->num_keys - 1), 0, SIZE_VALUE);

        leaf_node->num_keys--;

    } else {
        InternalPage* internal_node = (InternalPage*)node_page;

        // find a slot of deleting key
        for (i = 0; i < internal_node->num_keys; i++) {
            if (INTERNAL_KEY(internal_node, i) == key) {
                key_idx = i;
                break;
            }
        }
        if (i == internal_node->num_keys) {
            assert("remove_entry_from_node: no key in this page");
        }

        // shift keys/pointers
        for (i = key_idx; i < internal_node->num_keys - 1; i++) {
            INTERNAL_KEY(internal_node, i) = INTERNAL_KEY(internal_node, i+1);
            INTERNAL_OFFSET(internal_node, i+1) = INTERNAL_OFFSET(internal_node, i+2);
        }
        // clear garbage key/pointer
        INTERNAL_KEY(internal_node, internal_node->num_keys - 1) = 0;
        INTERNAL_OFFSET(internal_node, internal_node->num_keys) = 0;

        internal_node->num_keys--;
    }

    flush_page_to_buffer(table_id, (Page*)node_page);
}

void adjust_root(int table_id) {

    NodePage root_page;
    load_page_from_buffer(table_id, dbheader[table_id - 1].root_offset, (Page*)&root_page);

	/* Case: nonempty root.
	 * Key and pointer have already been deleted,
	 * so nothing to be done.
	 */

	if (root_page.num_keys > 0)
        return;

	/* Case: empty root. 
	 */

	// If it has a child, promote 
	// the first (only) child
	// as the new root.

	if (!root_page.is_leaf) {
        InternalPage* root_node = (InternalPage*)&root_page;
        dbheader[table_id - 1].root_offset = INTERNAL_OFFSET(root_node, 0);
		
        NodePage node_page;
        load_page_from_buffer(table_id, dbheader[table_id - 1].root_offset, (Page*)&node_page);
        node_page.parent = 0;

        flush_page_to_buffer(table_id, (Page*)&node_page);
        flush_page_to_buffer(table_id, (Page*)(dbheader + table_id - 1));
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else {
        dbheader[table_id - 1].root_offset = 0;
        flush_page_to_buffer(table_id, (Page*)(dbheader + table_id - 1));
    }

    put_free_page(table_id, root_page.file_offset);
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
void coalesce_nodes(int table_id, NodePage* node_page, NodePage* neighbor_page, int neighbor_index, int k_prime) {

	int i, j, neighbor_insertion_index, n_end;
	NodePage* tmp;

	/* Swap neighbor with node if node is on the
	 * extreme left and neighbor is to its right.
	 */

	if (neighbor_index == -1) {
		tmp = node_page;
		node_page = neighbor_page;
		neighbor_page = tmp;
	}

	/* Starting point in the neighbor for copying
	 * keys and pointers from n.
	 * Recall that n and neighbor have swapped places
	 * in the special case of n being a leftmost child.
	 */

	neighbor_insertion_index = neighbor_page->num_keys;

	/* Case:  nonleaf node.
	 * Append k_prime and the following pointer.
	 * Append all pointers and keys from the neighbor.
	 */

	if (!node_page->is_leaf) {
        InternalPage* node = (InternalPage*)node_page;
        InternalPage* neighbor_node = (InternalPage*)neighbor_page;

		/* Append k_prime.
		 */

		INTERNAL_KEY(neighbor_node, neighbor_insertion_index) = k_prime;
		neighbor_node->num_keys++;

		n_end = node->num_keys;

		for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
			INTERNAL_KEY(neighbor_node, i) = INTERNAL_KEY(node, j);
			INTERNAL_OFFSET(neighbor_node, i) = INTERNAL_OFFSET(node, j);
			neighbor_node->num_keys++;
			node->num_keys--;
		}

		/* The number of pointers is always
		 * one more than the number of keys.
		 */

		INTERNAL_OFFSET(neighbor_node, i) = INTERNAL_OFFSET(node, j);

		/* All children must now point up to the same parent.
		 */

		for (i = 0; i < neighbor_node->num_keys + 1; i++) {
            NodePage child_page;
            load_page_from_buffer(table_id, INTERNAL_OFFSET(neighbor_node, i), (Page*)&child_page);
            child_page.parent = neighbor_node->file_offset;
            flush_page_to_buffer(table_id, (Page*)&child_page);
        }

        flush_page_to_buffer(table_id, (Page*)neighbor_node);

        put_free_page(table_id, node->file_offset);
	}

	/* In a leaf, append the keys and pointers of
	 * n to the neighbor.
	 * Set the neighbor's last pointer to point to
	 * what had been n's right neighbor.
	 */

	else {
        LeafPage* node = (LeafPage*)node_page;
        LeafPage* neighbor_node = (LeafPage*)neighbor_page;

		for (i = neighbor_insertion_index, j = 0; j < node->num_keys; i++, j++) {
			LEAF_KEY(neighbor_node, i) = LEAF_KEY(node, j);
            memcpy(LEAF_VALUE(neighbor_node, i), LEAF_VALUE(node, j), SIZE_VALUE);
			neighbor_node->num_keys++;
		}
        neighbor_node->sibling = node->sibling;

        flush_page_to_buffer(table_id, (Page*)neighbor_node);

        put_free_page(table_id, node->file_offset);
	}

    NodePage parent_node;
    load_page_from_buffer(table_id, node_page->parent, (Page*)&parent_node);
	delete_entry(table_id, &parent_node, k_prime);
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
void redistribute_nodes(int table_id, NodePage* node_page, NodePage* neighbor_page,
                        int neighbor_index, 
		                int k_prime_index, int k_prime) {  

	int i;

	/* Case: n has a neighbor to the left. 
	 * Pull the neighbor's last key-pointer pair over
	 * from the neighbor's right end to n's left end.
	 */

	if (neighbor_index != -1) {
		if (!node_page->is_leaf) {
            InternalPage* node = (InternalPage*)node_page;
            InternalPage* neighbor_node = (InternalPage*)neighbor_page;
			INTERNAL_OFFSET(node, node->num_keys + 1) = INTERNAL_OFFSET(node, node->num_keys);
            
            for (i = node->num_keys; i > 0; i--) {
		    	INTERNAL_KEY(node, i) = INTERNAL_KEY(node, i - 1);
		    	INTERNAL_OFFSET(node, i) = INTERNAL_OFFSET(node, i - 1);
		    }
            INTERNAL_OFFSET(node, 0) = INTERNAL_OFFSET(neighbor_node, neighbor_node->num_keys);
            NodePage child_page;
            load_page_from_buffer(table_id, INTERNAL_OFFSET(node, 0), (Page*)&child_page);
            child_page.parent = node->file_offset;
            flush_page_to_buffer(table_id, (Page*)&child_page);

			INTERNAL_OFFSET(neighbor_node, neighbor_node->num_keys) = 0;
			INTERNAL_KEY(node, 0) = k_prime;

            InternalPage parent_node;
            load_page_from_buffer(table_id, node->parent, (Page*)&parent_node);
            INTERNAL_KEY(&parent_node, k_prime_index) = INTERNAL_KEY(neighbor_node, neighbor_node->num_keys - 1);
            flush_page_to_buffer(table_id, (Page*)&parent_node);

            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */
            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page_to_buffer(table_id, (Page*)node_page);
            flush_page_to_buffer(table_id, (Page*)neighbor_page);

        } else {
            LeafPage* node = (LeafPage*)node_page;
            LeafPage* neighbor_node = (LeafPage*)neighbor_page;

            for (i = node->num_keys; i > 0; i--) {
			    LEAF_KEY(node, i) = LEAF_KEY(node, i - 1);
			    memcpy(LEAF_VALUE(node, i), LEAF_VALUE(node, i - 1), SIZE_VALUE);
		    }
            memcpy(LEAF_VALUE(node, 0), LEAF_VALUE(neighbor_node, neighbor_node->num_keys - 1), SIZE_VALUE);
			memset(LEAF_VALUE(neighbor_node, neighbor_node->num_keys - 1), 0, SIZE_VALUE);
			LEAF_KEY(node, 0) = LEAF_KEY(neighbor_node, neighbor_node->num_keys - 1);

            InternalPage parent_node;
            load_page_from_buffer(table_id, node->parent, (Page*)&parent_node);
			INTERNAL_KEY(&parent_node, k_prime_index) = LEAF_KEY(node, 0);
            flush_page_to_buffer(table_id, (Page*)&parent_node);

            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */
            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page_to_buffer(table_id, (Page*)node_page);
            flush_page_to_buffer(table_id, (Page*)neighbor_page);
        }
    }

	/* Case: n is the leftmost child.
	 * Take a key-pointer pair from the neighbor to the right.
	 * Move the neighbor's leftmost key-pointer pair
	 * to n's rightmost position.
	 */

	else {  
		if (node_page->is_leaf) {
            LeafPage* node = (LeafPage*)node_page;
            LeafPage* neighbor_node = (LeafPage*)neighbor_page;;

			LEAF_KEY(node, node->num_keys) = LEAF_KEY(neighbor_node, 0);
			memcpy(LEAF_VALUE(node, node->num_keys), LEAF_VALUE(neighbor_node, 0), SIZE_VALUE);

            InternalPage parent_node;
            load_page_from_buffer(table_id, node->parent, (Page*)&parent_node);
			INTERNAL_KEY(&parent_node, k_prime_index) = LEAF_KEY(neighbor_node, 1);
            flush_page_to_buffer(table_id, (Page*)&parent_node);
            
            for (i = 0; i < neighbor_node->num_keys - 1; i++) {
			    LEAF_KEY(neighbor_node, i) = LEAF_KEY(neighbor_node, i + 1);
			    memcpy(LEAF_VALUE(neighbor_node, i), LEAF_VALUE(neighbor_node, i + 1), SIZE_VALUE);
            }

            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */
            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page_to_buffer(table_id, (Page*)node_page);
            flush_page_to_buffer(table_id, (Page*)neighbor_page);

		}
		else {
            InternalPage* node = (InternalPage*)node_page;
            InternalPage* neighbor_node = (InternalPage*)neighbor_page;

			INTERNAL_KEY(node, node->num_keys) = k_prime;
			INTERNAL_OFFSET(node, node->num_keys + 1) = INTERNAL_OFFSET(neighbor_node, 0);
            
            NodePage child_page;
            load_page_from_buffer(table_id, INTERNAL_OFFSET(node, node->num_keys + 1), (Page*)&child_page);
            child_page.parent = node->file_offset;
            flush_page_to_buffer(table_id, (Page*)&child_page);

            InternalPage parent_node;
            load_page_from_buffer(table_id, node->parent, (Page*)&parent_node);
            INTERNAL_KEY(&parent_node, k_prime_index) = INTERNAL_KEY(neighbor_node, 0);
            flush_page_to_buffer(table_id, (Page*)&parent_node);

            for (i = 0; i < neighbor_node->num_keys - 1; i++) {
			    INTERNAL_KEY(neighbor_node, i) = INTERNAL_KEY(neighbor_node, i + 1);
			    INTERNAL_OFFSET(neighbor_node, i) = INTERNAL_OFFSET(neighbor_node, i + 1);

		    }
			INTERNAL_OFFSET(neighbor_node, i) = INTERNAL_OFFSET(neighbor_node, i + 1);


            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */

            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page_to_buffer(table_id, (Page*)node_page);
            flush_page_to_buffer(table_id, (Page*)neighbor_page);

		}
    }
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void delete_entry(int table_id, NodePage* node_page, uint64_t key) {

	int min_keys;
	off_t neighbor_offset;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;

	// Remove key and pointer from node.

	remove_entry_from_node(table_id, node_page, key);
	/* Case:  deletion from the root. 
	 */

	if (dbheader[table_id - 1].root_offset == node_page->file_offset) {
		adjust_root(table_id);
        return;
    }

	/* Case:  deletion from a node below the root.
	 * (Rest of function body.)
	 */

	/* Determine minimum allowable size of node,
	 * to be preserved after deletion.
	 */

	min_keys = node_page->is_leaf ? cut(order_leaf - 1) : cut(order_internal) - 1;

	/* Case:  node stays at or above minimum.
	 * (The simple case.)
	 */

	if (node_page->num_keys >= min_keys)
        return;
	
    /* Case:  node falls below minimum.
	 * Either coalescence or redistribution
	 * is needed.
	 */

	/* Find the appropriate neighbor node with which
	 * to coalesce.
	 * Also find the key (k_prime) in the parent
	 * between the pointer to node n and the pointer
	 * to the neighbor.
	 */

	neighbor_index = get_neighbor_index(table_id, node_page);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

    InternalPage parent_node;
    load_page_from_buffer(table_id, node_page->parent, (Page*)&parent_node);

	k_prime = INTERNAL_KEY(&parent_node, k_prime_index);
	neighbor_offset = neighbor_index == -1 ? INTERNAL_OFFSET(&parent_node, 1) : 
		INTERNAL_OFFSET(&parent_node, neighbor_index);

	capacity = node_page->is_leaf ? order_leaf : order_internal - 1;

    NodePage neighbor_page;
    load_page_from_buffer(table_id, neighbor_offset, (Page*)&neighbor_page);
	/* Coalescence. */

	if (neighbor_page.num_keys + node_page->num_keys < capacity)
		coalesce_nodes(table_id, node_page, &neighbor_page, neighbor_index, k_prime);

	/* Redistribution. */

	else
		redistribute_nodes(table_id, node_page, &neighbor_page, neighbor_index, k_prime_index, k_prime);


}

/* Master deletion function.
 */
int delete(int table_id, uint64_t key) {

    char* value_found = NULL;
    if ((value_found = find(table_id, key)) == 0) {
        // This key is not in the tree
        free(value_found);
        return -1;
    }

    LeafPage leaf_node;
    find_leaf(table_id, key, &leaf_node);

    delete_entry(table_id, (NodePage*)&leaf_node, key);

    return 0;
}

/* Project Buffer */
int init_db(int num_buf){
    int i;
    // Auto intialize
    buf_mgr = (Buffer *)calloc(num_buf, sizeof(Buffer)); 

    for(i = 0; i < num_buf; i++){
        buf_mgr[i].frame = (Page*)malloc(sizeof(Page));
    }

    if(buf_mgr == NULL){
        /* Fail */
        return -1;
    }

    /* Success */
    // Memory allocation
    buf_size = num_buf;

    return 0;
}

int close_table(int table_id){
    int i;

    // Failure case
    if(table_id < 1 || table_id > 10 || buf_size == -1 || buf_mgr == NULL){
        return -1;
    }

    for(i = 0; i < buf_size; i++){
        if(buf_mgr[i].table_id == table_id){
            // If dirty bit is on, write page.
            if(buf_mgr[i].is_dirty == 1){
                flush_page(table_id, buf_mgr[i].frame);
            }
            // Reinitialize : Evict
            memset(buf_mgr[i].frame, 0, sizeof(Page));
            buf_mgr[i].table_id = 0;
            buf_mgr[i].page_offset = 0;
            buf_mgr[i].is_dirty = 0;
            buf_mgr[i].refbit = 0;
        }
    }

    // Discard the table id.
    table_ids[table_id -1] = 0;
    
    // Close file
    close_db(table_id);

    return 0;
}

int shutdown_db(){
    int i,table_id = -1;

    // Failure case
    if(buf_size == -1 || buf_mgr == NULL){
        return -1;
    }

    for(i = 0; i < buf_size; i++){
        // If dirty bit is on, write page.
        if(buf_mgr[i].is_dirty == 1){
            table_id = buf_mgr[i].table_id;
            flush_page(table_id, buf_mgr[i].frame);
        }
        // If buffer is used, memory free should be done.
        if(1 <= buf_mgr[i].table_id && buf_mgr[i].table_id <= 10){
            free(buf_mgr[i].frame);
        }
    }

    // Destroy allocated buffer
    free(buf_mgr);

    return 0;
}
// Load function
void load_page_from_buffer(int table_id, off_t offset, Page* page){
    Page *temp;
    int buf_index = -1;

    temp = check_buffer_for_load(table_id, offset);

    // Page is in buffer pool.
    if(temp != NULL){
        // Load page from buffer pool.
        memcpy(page, temp, sizeof(Page));
        page->file_offset = offset;

        /* Turn ref bit on */
        // Previously done in is_in_buffer function
    }
    // Page is not in buffer pool.
    else{
        buf_index = replace_page(NULL);

        // Load from disk.
        load_page(table_id, offset, page);
        page->file_offset = offset; 

        /* Not busy buffer pool */
        // In case of busy buffer pool, just load page from disk.
        /* Setting new buffer */ 
        // Page index is set already in replace_page function.
        // Memory allocation & Memory copy
        memcpy(buf_mgr[buf_index].frame, page, sizeof(Page));
        buf_mgr[buf_index].table_id = table_id;
        buf_mgr[buf_index].page_offset = offset;
        buf_mgr[buf_index].is_dirty = 0;
        buf_mgr[buf_index].refbit = 1;
    }
}
Page* check_buffer_for_load(int table_id, off_t offset){
    int i;

    if(buf_size == -1){
        // Error case 
        return NULL;
    }

    for(i = 0; i < buf_size; i++){
        if(buf_mgr[target_buf].table_id == table_id && buf_mgr[target_buf].page_offset == offset){

            buf_mgr[target_buf].refbit = 1;

            return buf_mgr[target_buf].frame;
        }
        target_buf = (target_buf + 1) % buf_size;
    }

    // Unmatched case
    return NULL;
}
int check_buffer_for_flush(int table_id, off_t offset){
    int i;

    if(buf_size == -1){
        // Error case 
        return -1;
    }

    for(i = 0; i < buf_size; i++){
        if(buf_mgr[target_buf].table_id == table_id && buf_mgr[target_buf].page_offset == offset){

            buf_mgr[target_buf].refbit = 1;

            return target_buf;
        }
        target_buf = (target_buf + 1) % buf_size;
    }

    // Unmatched case
    return -1;
}

int replace_page(FILE *file){
    /* Used variable : clock_hand, buf_size */
    int target_index = -1;

    // Spin only one cycle.
    while(target_index == -1){
        /* Check refernce bit */
        // Case : reference bit is off.
        if(buf_mgr[clock_hand].refbit == 0){
            // Setting target_index
            target_index = clock_hand;

            // Output buffer
            if(buf_mgr[clock_hand].table_id == -1){
                int i;
                OutputPage *target;

                memcpy((Page *)target, buf_mgr[clock_hand].frame, sizeof(Page));

                for(int i = 0; i < target->file_offset; i++){
                    fprintf(file, "%" PRIu64 ",%s," "%" PRIu64 ",%s\n", RESULT_KEY1(target, i), RESULT_VALUE1(target, i), RESULT_KEY2(target, i), RESULT_VALUE2(target, i) );
                }

                buf_mgr[clock_hand].is_dirty = 0;
            }
            // Regular buffer
            // Check dirty bit.
            if(buf_mgr[clock_hand].is_dirty == 1){
                /* Flush page. */
                flush_page(buf_mgr[clock_hand].table_id, buf_mgr[clock_hand].frame);
            }
            // Reinitialize : Evict
            memset(buf_mgr[clock_hand].frame, 0, sizeof(Page));
            buf_mgr[clock_hand].table_id = 0;
            buf_mgr[clock_hand].page_offset = 0;
            buf_mgr[clock_hand].is_dirty = 0;
            buf_mgr[clock_hand].refbit = 0;
        }

        // Case : reference bit is on.
        // Turn off the reference bit.
        else{
            if(buf_mgr[clock_hand].refbit == 1){
                buf_mgr[clock_hand].refbit = 0;
            }

            // Move clock hand
            clock_hand = (clock_hand + 1) % buf_size;
        }
    }

    return target_index;
}

/* Flush function */
void flush_page_to_buffer(int table_id, Page *page){
    int i, buf_index = -1, index = -1;

    // Check if target page is in buffer
    // Decrease pin count : Done in is_in_buffer function.
    // Dirty bit setting : Done in is_in_buffer function.
    index = check_buffer_for_flush(table_id, page->file_offset);

    // Page is in buffer
    if(index != -1){
        memcpy(buf_mgr[target_buf].frame, page, sizeof(Page));
        buf_mgr[target_buf].is_dirty = 1;
    }
    // Page is not in buffer
    else{
        // Ref bit is already turned on in is_in_buffer.
        buf_index = replace_page(NULL);
        // Setting new buffer // 
        // Page pointer is set already in replace_page function.
        // Memory allocation & Memory copy
        memcpy(buf_mgr[buf_index].frame, page, sizeof(Page));
        buf_mgr[buf_index].table_id = table_id;
        buf_mgr[buf_index].page_offset = page->file_offset;
        // Setting dirty bit
        buf_mgr[buf_index].is_dirty = 1;
        buf_mgr[buf_index].refbit = 1;
    }
}

/* Project Join */
// Return 0 if success, otherwise return -1
// Premise : Given two tables are already open
int join_table(int table_id_1, int table_id_2, char *pathname){
    FILE *r_fp;
    uint64_t num_1 = -1, num_2 = -1, min_1 = -1, min_2 = -1, max_1 = -1, max_2 = -1;
    LeafPage leaf_1, leaf_2;
    off_t comp_sib_1, comp_sib_2;
    int comp_num_1, comp_num_2;
    uint64_t comp_key_1, comp_key_2;

    /* Open file where result table will be written */
    if((r_fp = fopen(pathname, "wt")) == NULL){
        return -1;
    }

    /* Examine information of each table */
    table_info(table_id_1, &num_1, &min_1, &max_1);
    table_info(table_id_2, &num_2, &min_2, &max_2);

    /* Use sort-merge join */

    /* 
        Exception check : if no common key, just return.
        Checking via min - max range.
    */
    if(max_1 < min_2 || min_1 > max_2){
        // Close file pointer
        fclose(r_fp);

        return 0;
    }

    /* 
        Start condition : load initial leaf page of each table.
        -> Use find_leaf function 
    */

    // Case : Cut off table 1
    if(min_1 < min_2){
        // Table 1 : cut off
        // Table 2 : just load first leaf page
        find_leaf(table_id_1, min_2, &leaf_1);
        find_leaf(table_id_2, min_2, &leaf_2);
    }
    // Case : Cut off table 2
    else{
        // Table 1 : just load fisrt leaf page
        // Table 2 : cut off
        find_leaf(table_id_1, min_1, &leaf_1);
        find_leaf(table_id_2, min_1, &leaf_2);
    }

    // Initial condition
    comp_sib_1 = leaf_1.sibling;
    comp_sib_2 = leaf_2.sibling;
    comp_num_1 = 0;
    comp_num_2 = 0;

    /*
        Loop start : terminate condition
        -> Until rightmost sibling & last key ( Checking via number of keys)
    */

    while((comp_sib_1 != 0 || comp_num_1 != leaf_1.num_keys) && (comp_sib_2 != 0 || comp_num_2 != leaf_2.num_keys)){
        /* Compare & produce result */
        comp_key_1 = LEAF_KEY(&leaf_1, comp_num_1);
        comp_key_2 = LEAF_KEY(&leaf_2, comp_num_2);
        
        // Compare
        while(comp_key_1 < comp_key_2){
            // Advance table 1
            comp_num_1++;

            // Case : Change table1's leaf page
            if((comp_num_1 == leaf_1.num_keys) && (comp_sib_1 != 0)){
                // Update leaf page
                load_page_from_buffer(table_id_1, comp_sib_1, (Page*)&leaf_1);

                // Update sibling offset
                comp_sib_1 = leaf_1.sibling;

                // Reinitialize
                comp_num_1 = 0;
            }

            // Terminate comdition
            if(comp_sib_1 == 0 && comp_num_1 ==  leaf_1.num_keys){
                sync_buffer(r_fp);
                fclose(r_fp);
                return 0;
            }

            // Update compare key
            comp_key_1 = LEAF_KEY(&leaf_1, comp_num_1);
        }
        while(comp_key_1 > comp_key_2){
            // Advance table 2
            comp_num_2++;
            
            // Case : Change table2's leaf page
            if((comp_num_2 == leaf_2.num_keys) && (comp_sib_2 != 0)){
                // Update leaf page
                load_page_from_buffer(table_id_2, comp_sib_2, (Page*)&leaf_2);

                // Update sibling offset
                comp_sib_2 = leaf_2.sibling;

                // Reinitialize
                comp_num_2 = 0;
            }

            // Terminate condition
            if(comp_sib_1 == 0 && comp_num_1 == leaf_1.num_keys){
                sync_buffer(r_fp);
                fclose(r_fp);
                return 0;
            }

            // Update compare key
            comp_key_2 = LEAF_KEY(&leaf_2, comp_num_2);
        }

        if(comp_key_1 == comp_key_2){
            /* Produce */
            // Notice that two tables are on unique key condition.
            write_output_buffer(r_fp, comp_key_1, LEAF_VALUE(&leaf_1, comp_num_1), comp_key_2, LEAF_VALUE(&leaf_2, comp_num_2));

            /* Advance each key */
            // Update compare number
            comp_num_1++;
            comp_num_2++;

            // Case : Change table1's leaf page
            if((comp_num_1 == leaf_1.num_keys) && (comp_sib_1 != 0)){
                // Update leaf page
                load_page_from_buffer(table_id_1, comp_sib_1, (Page*)&leaf_1);

                // Update sibling offset
                comp_sib_1 = leaf_1.sibling;

                // Reinitialize
                comp_num_1 = 0;
            }

            // Case : Change table2's leaf page
            if((comp_num_2 == leaf_2.num_keys) && (comp_sib_2 != 0)){
                // Update leaf page
                load_page_from_buffer(table_id_2, comp_sib_2, (Page*)&leaf_2);

                // Update sibling offset
                comp_sib_2 = leaf_2.sibling;

                // Reinitialize
                comp_num_2 = 0;
            }

            // Update compare key : Done in first part of loop
        }
    }
    sync_buffer(r_fp);
    fclose(r_fp);
    return 0;
}
void table_info(int table_id, uint64_t *num_keys, uint64_t *min_key, uint64_t *max_key){
    HeaderPage temp_header;
    NodePage page;
    LeafPage *temp_leaf;
    off_t temp_sibling;

    // Load header page.
    load_page_from_buffer(table_id, 0, (Page*)(&temp_header));

    /* Case : Empty table */
    if(temp_header.root_offset == 0){
        *num_keys = 0;
        *max_key = 0;

        return;
    }
    
    // Load root page.
    load_page_from_buffer(table_id, temp_header.root_offset, (Page*)&page);

    // Search leaf page whcih has the smallest key.
    while(!page.is_leaf){
        InternalPage* internal_node = (InternalPage*)&page;
        
        load_page_from_buffer(table_id, INTERNAL_OFFSET(internal_node, 0), (Page*)&page);
	}

    /* Proceed until last leaf page using sibling.
       Sum each leaf page's number of keys and find maximum key. */

    // First, set initial condition.
    temp_leaf = (LeafPage *)&page;

    *num_keys = temp_leaf->num_keys;
    *min_key = LEAF_KEY(temp_leaf, 0);
    *max_key = LEAF_KEY(temp_leaf, temp_leaf->num_keys - 1);
    temp_sibling = temp_leaf->sibling;

    while(temp_sibling != 0){
        // Load sibling leaf page.
        load_page_from_buffer(table_id, temp_sibling, (Page*)temp_leaf);
        
        *num_keys += temp_leaf->num_keys;
        *max_key = LEAF_KEY(temp_leaf, temp_leaf->num_keys - 1);
        temp_sibling = temp_leaf->sibling;
    }

}
void write_output_buffer(FILE *file, uint64_t key1, char *value1, uint64_t key2, char *value2){
    OutputPage output;

    // Not in buffer
    if(load_output_page((Page*)&output) == -1){
        output.results[0].key1 = key1;
        strcpy(output.results[0].value1, value1);
        output.results[0].key2 = key2;
        strcpy(output.results[0].value2, value2);
        output.file_offset = 1;
    }
    // Is in buffer
    else{
        // Available output.
        if(output.file_offset < OUTPUT_ORDER){
            RESULT_KEY1(&output, output.file_offset) = key1;
            strcpy(RESULT_VALUE1(&output, output.file_offset), value1);
            RESULT_KEY2(&output, output.file_offset) = key2;
            strcpy(RESULT_VALUE2(&output, output.file_offset), value2);

            output.file_offset++;
        }
        // Full output.
        else{
            OutputPage new;

            sync_buffer(file);

            new.results[0].key1 = key1;
            strcpy(new.results[0].value1, value1);
            new.results[0].key2 = key2;
            strcpy(new.results[0].value2, value2);
            new.file_offset = 1;

            flush_output_page(file, (Page *)&new);
            return;
        }
    }

    flush_output_page(file, (Page *)&output);
}
int load_output_page(Page *page){
    Page *temp;

    temp = check_buffer_for_load(-1, 0);

    // Page is in buffer pool.
    if(temp != NULL){
        // Load page from buffer pool.
        memcpy(page, temp, sizeof(Page));

        /* Turn ref bit on */
        // Previously done in is_in_buffer function
        return 0;
    }
    // Page is not in buffer pool.
    else{
        return -1;
    }
}
void flush_output_page(FILE *file, Page *page){
    int i, buf_index = -1, index = -1;

    // Check if target page is in buffer
    // Decrease pin count : Done in is_in_buffer function.
    // Dirty bit setting : Done in is_in_buffer function.
    index = check_buffer_for_flush(-1, 0);

    // Page is in buffer
    if(index != -1){
        memcpy(buf_mgr[target_buf].frame, page, sizeof(Page));
        buf_mgr[target_buf].is_dirty = 1;
    }
    // Page is not in buffer
    else{
        // Ref bit is already turned on in is_in_buffer.
        buf_index = replace_page(file);
        // Setting new buffer // 
        // Page pointer is set already in replace_page function.
        // Memory allocation & Memory copy
        memcpy(buf_mgr[buf_index].frame, page, sizeof(Page));
        buf_mgr[buf_index].table_id = -1;
        buf_mgr[buf_index].page_offset = 0;
        // Setting dirty bit
        buf_mgr[buf_index].is_dirty = 1;
        buf_mgr[buf_index].refbit = 1;
    }
}
void sync_buffer(FILE *file){
    int i, index, end;
    OutputPage target;

    index = check_buffer_for_flush(-1, 0);

    memcpy((Page *)&target, buf_mgr[index].frame, sizeof(Page));
    for(int i = 0; i < target.file_offset; i++){
        fprintf(file, "%" PRIu64 ",%s," "%" PRIu64 ",%s\n", RESULT_KEY1(&target, i), RESULT_VALUE1(&target, i), RESULT_KEY2(&target, i), RESULT_VALUE2(&target, i) );
    }

    // Reinitialize : Evict
    memset(buf_mgr[index].frame, 0, sizeof(Page));
    buf_mgr[index].table_id = 0;
    buf_mgr[index].page_offset = 0;
    buf_mgr[index].is_dirty = 0;
    buf_mgr[index].refbit = 0;
}
