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
extern HeaderPage dbheader;
extern int dbfile;

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

// FUNCTION PROTOTYPES.

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void find_and_print(uint64_t key); 
bool find_leaf(uint64_t key, LeafPage* out_leaf_node);

// Insertion.
void start_new_tree(uint64_t key, const char* value);
void insert_into_leaf(LeafPage* leaf_node, uint64_t key, const char* value);
void insert_into_leaf_after_splitting(LeafPage* leaf_node, uint64_t key, const char* value);
void insert_into_parent(NodePage* left, uint64_t key, NodePage* right);
void insert_into_new_root(NodePage* left, uint64_t key, NodePage* right);
int get_left_index(InternalPage* parent, off_t left_offset);
void insert_into_node(InternalPage * parent, int left_index, uint64_t key, off_t right_offset);
void insert_into_node_after_splitting(InternalPage* parent, int left_index, uint64_t key, off_t right_offset);

// Deletion.
int get_neighbor_index(NodePage* node_page);
void adjust_root();
void coalesce_nodes(NodePage* node_page, NodePage* neighbor_page,
                      int neighbor_index, int k_prime);
void redistribute_nodes(NodePage* node_page, NodePage* neighbor_page,
                          int neighbor_index,
                          int k_prime_index, int k_prime);
void delete_entry(NodePage* node_page, uint64_t key);


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
int open_db(const char* filename) {
    dbfile = open(filename, O_RDWR);
    if (dbfile < 0) {
        // Create a new db file
        dbfile = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if (dbfile < 0) {
            assert("failed to create new db file");
            return -1;
        }
        
        memset(&dbheader, 0, PAGE_SIZE);
        dbheader.freelist = 0;
        dbheader.root_offset = 0;
        dbheader.num_pages = 1;
        dbheader.file_offset = 0;
        flush_page((Page*)&dbheader);
    } else {
        // DB file exist. Load header info
        load_page(0, (Page*)&dbheader);
        dbheader.file_offset = 0;
    }

    return 0;
}

void close_db() {
    close(dbfile);
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
void print_tree() {

    int i;
    int front = 0;
    int rear = 0;

    if (dbheader.root_offset == 0) {
		printf("Empty tree.\n");
        return;
    }

    queue[rear] = dbheader.root_offset;
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
        load_page(page_offset, (Page*)&node_page);
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
void find_and_print(uint64_t key) {
    char* value_found = NULL;
	value_found = find(key);
	if (value_found == NULL) {
		printf("Record not found under key %" PRIu64 ".\n", key);
        fflush(stdout);
    }
	else {
		printf("key %" PRIu64 ", value [%s].\n", key, value_found);
        fflush(stdout);
        free(value_found);
    }
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
bool find_leaf(uint64_t key, LeafPage* out_leaf_node) {
    int i = 0;
    off_t root_offset = dbheader.root_offset;

	if (root_offset == 0) {
		return false;
	}
    
    NodePage page;
    load_page(root_offset, (Page*)&page);

	while (!page.is_leaf) {
        InternalPage* internal_node = (InternalPage*)&page;

        i = 0;
		while (i < internal_node->num_keys) {
			if (key >= INTERNAL_KEY(internal_node, i)) i++;
			else break;
		}
        
        load_page(INTERNAL_OFFSET(internal_node, i), (Page*)&page);
	}

    memcpy(out_leaf_node, &page, sizeof(LeafPage));

	return true;
}

/* Finds and returns the record to which
 * a key refers.
 */
// If you want to return a record, use 3rd parameter
char* find(uint64_t key) {
    int i = 0;
    char* out_value;

    LeafPage leaf_node;
    if (!find_leaf(key, &leaf_node)) {
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
void insert_into_leaf(LeafPage* leaf_node, uint64_t key, const char* value) {
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
    flush_page((Page*)leaf_node);
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting(LeafPage* leaf, uint64_t key, const char* value) {

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
    new_leaf.file_offset = get_free_page();

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

    flush_page((Page*)leaf);
    flush_page((Page*)&new_leaf);

	new_key = LEAF_KEY(&new_leaf, 0);

    // insert new key and new leaf to the parent
	insert_into_parent((NodePage*)leaf, new_key, (NodePage*)&new_leaf);
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
void insert_into_node_after_splitting(InternalPage* old_node, int left_index, uint64_t key, off_t right_offset) {
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
    new_node.file_offset = get_free_page();

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
        load_page(INTERNAL_OFFSET(&new_node, i), (Page*)&child_page);
        child_page.parent = new_node.file_offset;
	    flush_page((Page*)&child_page);
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
    flush_page((Page*)&new_node);
    flush_page((Page*)old_node);

	/* Insert a new key into the parent of the two
	 * nodes resulting from the split, with
	 * the old node to the left and the new to the right.
	 */
	insert_into_parent((NodePage*)old_node, k_prime, (NodePage*)&new_node);
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
void insert_into_parent(NodePage* left, uint64_t key, NodePage* right) {
	//off_t left_index;
    InternalPage parent_node;

    /* Case: new root. */
	if (left->parent == 0) {
		insert_into_new_root(left, key, right);
        return;
    }

    load_page(left->parent, (Page*)&parent_node);

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
        flush_page((Page*)&parent_node);
        return;
    }

	/* Harder case:  split a node in order 
	 * to preserve the B+ tree properties.
	 */

	return insert_into_node_after_splitting(&parent_node, left_index, key, right->file_offset);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(NodePage* left, uint64_t key, NodePage* right) {
    // make new root node
    InternalPage root_node;
    memset(&root_node, 0, sizeof(InternalPage));
    root_node.file_offset = get_free_page();
    INTERNAL_KEY(&root_node, 0) = key;
    INTERNAL_OFFSET(&root_node, 0) = left->file_offset;
    INTERNAL_OFFSET(&root_node, 1) = right->file_offset;
    root_node.num_keys++;
    root_node.parent = 0;
    root_node.is_leaf = 0;
    left->parent = root_node.file_offset;
    right->parent = root_node.file_offset;

    flush_page((Page*)&root_node);
    flush_page((Page*)left);
    flush_page((Page*)right);

    dbheader.root_offset = root_node.file_offset;
    flush_page((Page*)&dbheader);
}
/* First insertion:
 * start a new tree.
 */
void start_new_tree(uint64_t key, const char* value) {
    LeafPage root_node;
    
    off_t root_offset = get_free_page();
    root_node.file_offset = root_offset;

    root_node.parent = 0;
    root_node.is_leaf = 1;
    root_node.num_keys = 1;
    LEAF_KEY(&root_node, 0) = key;
    root_node.sibling = 0;
    memcpy(LEAF_VALUE(&root_node, 0), value, SIZE_VALUE);
    
    flush_page((Page*)&root_node);

    dbheader.root_offset = root_offset;
    flush_page((Page*)&dbheader);
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int insert(uint64_t key, const char* value) {
    /* The current implementation ignores
	 * duplicates.
	 */
    char* value_found = NULL;

    if ((value_found = find(key)) != 0) {
        free(value_found);
        return -1;
    }
	/* Case: the tree does not exist yet.
	 * Start a new tree.
	 */

	if (dbheader.root_offset == 0) {
		start_new_tree(key, value);
        fsync(dbfile);
        return 0;
    }
	
    /* Case: the tree already exists.
	 * (Rest of function body.)
	 */

    LeafPage leaf_node;
    find_leaf(key, &leaf_node);

	/* Case: leaf has room for key and pointer.
	 */

	if (leaf_node.num_keys < order_leaf - 1) {
        insert_into_leaf(&leaf_node, key, value);
	} else {
    	/* Case:  leaf must be split.
	     */
        insert_into_leaf_after_splitting(&leaf_node, key, value);
    }
    fsync(dbfile);
    return 0;
}

// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(NodePage* node_page) {

	int i;

	/* Return the index of the key to the left
	 * of the pointer in the parent pointing
	 * to n.  
	 * If n is the leftmost child, this means
	 * return -1.
	 */
    InternalPage parent_node;
    load_page(node_page->parent, (Page*)&parent_node);
	for (i = 0; i <= parent_node.num_keys; i++)
		if (INTERNAL_OFFSET(&parent_node, i) == node_page->file_offset)
			return i - 1;

	// Error state.
    assert("Search for nonexistent pointer to node in parent.");
    return -1;
}

void remove_entry_from_node(NodePage* node_page, uint64_t key) {

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

    flush_page((Page*)node_page);
}

void adjust_root() {

    NodePage root_page;
    load_page(dbheader.root_offset, (Page*)&root_page);

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
        dbheader.root_offset = INTERNAL_OFFSET(root_node, 0);
		
        NodePage node_page;
        load_page(dbheader.root_offset, (Page*)&node_page);
        node_page.parent = 0;

        flush_page((Page*)&node_page);
        flush_page((Page*)&dbheader);
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else {
        dbheader.root_offset = 0;
        flush_page((Page*)&dbheader);
    }

    put_free_page(root_page.file_offset);
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
void coalesce_nodes(NodePage* node_page, NodePage* neighbor_page, int neighbor_index, int k_prime) {

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
            load_page(INTERNAL_OFFSET(neighbor_node, i), (Page*)&child_page);
            child_page.parent = neighbor_node->file_offset;
		    flush_page((Page*)&child_page);
        }

        flush_page((Page*)neighbor_node);

        put_free_page(node->file_offset);
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

        flush_page((Page*)neighbor_node);

        put_free_page(node->file_offset);
	}

    NodePage parent_node;
    load_page(node_page->parent, (Page*)&parent_node);
	delete_entry(&parent_node, k_prime);
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
void redistribute_nodes(NodePage* node_page, NodePage* neighbor_page,
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
            load_page(INTERNAL_OFFSET(node, 0), (Page*)&child_page);
            child_page.parent = node->file_offset;
            flush_page((Page*)&child_page);

			INTERNAL_OFFSET(neighbor_node, neighbor_node->num_keys) = 0;
			INTERNAL_KEY(node, 0) = k_prime;

            InternalPage parent_node;
            load_page(node->parent, (Page*)&parent_node);
            INTERNAL_KEY(&parent_node, k_prime_index) = INTERNAL_KEY(neighbor_node, neighbor_node->num_keys - 1);
            flush_page((Page*)&parent_node);

            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */
            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page((Page*)node_page);
            flush_page((Page*)neighbor_page);

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
            load_page(node->parent, (Page*)&parent_node);
			INTERNAL_KEY(&parent_node, k_prime_index) = LEAF_KEY(node, 0);
            flush_page((Page*)&parent_node);

            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */
            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page((Page*)node_page);
            flush_page((Page*)neighbor_page);
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
            load_page(node->parent, (Page*)&parent_node);
			INTERNAL_KEY(&parent_node, k_prime_index) = LEAF_KEY(neighbor_node, 1);
            flush_page((Page*)&parent_node);
            
            for (i = 0; i < neighbor_node->num_keys - 1; i++) {
			    LEAF_KEY(neighbor_node, i) = LEAF_KEY(neighbor_node, i + 1);
			    memcpy(LEAF_VALUE(neighbor_node, i), LEAF_VALUE(neighbor_node, i + 1), SIZE_VALUE);
            }

            /* n now has one more key and one more pointer;
             * the neighbor has one fewer of each.
             */
            node->num_keys++;
            neighbor_node->num_keys--;
            
            flush_page((Page*)node_page);
            flush_page((Page*)neighbor_page);

		}
		else {
            InternalPage* node = (InternalPage*)node_page;
            InternalPage* neighbor_node = (InternalPage*)neighbor_page;

			INTERNAL_KEY(node, node->num_keys) = k_prime;
			INTERNAL_OFFSET(node, node->num_keys + 1) = INTERNAL_OFFSET(neighbor_node, 0);
            
            NodePage child_page;
            load_page(INTERNAL_OFFSET(node, node->num_keys + 1), (Page*)&child_page);
            child_page.parent = node->file_offset;
			flush_page((Page*)&child_page);

            InternalPage parent_node;
            load_page(node->parent, (Page*)&parent_node);
            INTERNAL_KEY(&parent_node, k_prime_index) = INTERNAL_KEY(neighbor_node, 0);
            flush_page((Page*)&parent_node);

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
            
            flush_page((Page*)node_page);
            flush_page((Page*)neighbor_page);

		}
    }
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void delete_entry(NodePage* node_page, uint64_t key) {

	int min_keys;
	off_t neighbor_offset;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;

	// Remove key and pointer from node.

	remove_entry_from_node(node_page, key);
	/* Case:  deletion from the root. 
	 */

	if (dbheader.root_offset == node_page->file_offset) {
		adjust_root();
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

	neighbor_index = get_neighbor_index(node_page);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

    InternalPage parent_node;
    load_page(node_page->parent, (Page*)&parent_node);

	k_prime = INTERNAL_KEY(&parent_node, k_prime_index);
	neighbor_offset = neighbor_index == -1 ? INTERNAL_OFFSET(&parent_node, 1) : 
		INTERNAL_OFFSET(&parent_node, neighbor_index);

	capacity = node_page->is_leaf ? order_leaf : order_internal - 1;

    NodePage neighbor_page;
    load_page(neighbor_offset, (Page*)&neighbor_page);
	/* Coalescence. */

	if (neighbor_page.num_keys + node_page->num_keys < capacity)
		coalesce_nodes(node_page, &neighbor_page, neighbor_index, k_prime);

	/* Redistribution. */

	else
		redistribute_nodes(node_page, &neighbor_page, neighbor_index, k_prime_index, k_prime);


}

/* Master deletion function.
 */
int delete(uint64_t key) {

    char* value_found = NULL;
    if ((value_found = find(key)) == 0) {
        // This key is not in the tree
        free(value_found);
        return -1;
    }

    LeafPage leaf_node;
    find_leaf(key, &leaf_node);

    delete_entry((NodePage*)&leaf_node, key);
    fsync(dbfile);

    return 0;
}

