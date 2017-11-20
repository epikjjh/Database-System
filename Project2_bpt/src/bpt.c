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

#include "bpt.h"

// GLOBALS.

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
int order = DEFAULT_ORDER;
int leaf_order = 32;
int internal_order = 249;

/* The queue is used to print the tree in
 * level order, starting from the root
 * printing each entire rank on a separate
 * line, finishing with the leaves.
 */
node * queue = NULL;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = false;

/* This file pointer is for open_db function.
 * It opens existing data file or create on if not existed. 
 */
FILE *fp; 

/*
 * In case of existing file.
 *
 */
int64_t default_offset;

/*
 * Linked list
 *
 */
page_list *head = NULL;

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
    printf("B+ Tree of Order %d.\n", order);
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


/* Brief usage note.
 */
void usage_3( void ) {
    printf("Usage: ./bpt [<order>]\n");
    printf("\twhere %d <= order <= %d .\n", MIN_ORDER, MAX_ORDER);
}


/* Helper function for printing the
 * tree out.  See print_tree.
 */
void enqueue( node * new_node ) {
    node * c;
    if (queue == NULL) {
        queue = new_node;
        queue->next = NULL;
    }
    else {
        c = queue;
        while(c->next != NULL) {
            c = c->next;
        }
        c->next = new_node;
        new_node->next = NULL;
    }
}


/* Helper function for printing the
 * tree out.  See print_tree.
 */
node * dequeue( void ) {
    node * n = queue;
    queue = queue->next;
    n->next = NULL;
    return n;
}


/* Prints the bottom row of keys
 * of the tree (with their respective
 * pointers, if the verbose_output flag is set.
 */
void print_leaves( node * root ) {
    int i;
    node * c = root;
    if (root == NULL) {
        printf("Empty tree.\n");
        return;
    }
    while (!c->is_leaf)
        c = c->pointers[0];
    while (true) {
        for (i = 0; i < c->num_keys; i++) {
            if (verbose_output)
                printf("%lx ", (unsigned long)c->pointers[i]);
            printf("%d ", c->keys[i]);
        }
        if (verbose_output)
            printf("%lx ", (unsigned long)c->pointers[order - 1]);
        if (c->pointers[order - 1] != NULL) {
            printf(" | ");
            c = c->pointers[order - 1];
        }
        else
            break;
    }
    printf("\n");
}


/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height( node * root ) {
    int h = 0;
    node * c = root;
    while (!c->is_leaf) {
        c = c->pointers[0];
        h++;
    }
    return h;
}


/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int path_to_root( node * root, node * child ) {
    int length = 0;
    node * c = child;
    while (c != root) {
        c = c->parent;
        length++;
    }
    return length;
}


/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 * With the verbose_output flag set.
 * the values of the pointers corresponding
 * to the keys also appear next to their respective
 * keys, in hexadecimal notation.
 */
void print_tree( node * root ) {

    node * n = NULL;
    int i = 0;
    int rank = 0;
    int new_rank = 0;

    if (root == NULL) {
        printf("Empty tree.\n");
        return;
    }
    queue = NULL;
    enqueue(root);
    while( queue != NULL ) {
        n = dequeue();
        if (n->parent != NULL && n == n->parent->pointers[0]) {
            new_rank = path_to_root( root, n );
            if (new_rank != rank) {
                rank = new_rank;
                printf("\n");
            }
        }
        if (verbose_output) 
            printf("(%lx)", (unsigned long)n);
        for (i = 0; i < n->num_keys; i++) {
            if (verbose_output)
                printf("%lx ", (unsigned long)n->pointers[i]);
            printf("%d ", n->keys[i]);
        }
        if (!n->is_leaf)
            for (i = 0; i <= n->num_keys; i++)
                enqueue(n->pointers[i]);
        if (verbose_output) {
            if (n->is_leaf) 
                printf("%lx ", (unsigned long)n->pointers[order - 1]);
            else
                printf("%lx ", (unsigned long)n->pointers[n->num_keys]);
        }
        printf("| ");
    }
    printf("\n");
}


/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
void find_and_print(node * root, int key, bool verbose) {
    record * r = find_tree(root, key, verbose);
    if (r == NULL)
        printf("Record not found under key %d.\n", key);
    else 
        printf("Record at %lx -- key %d, value %d.\n",
                (unsigned long)r, key, r->value);
}


/* Finds and prints the keys, pointers, and values within a range
 * of keys between key_start and key_end, including both bounds.
 */
void find_and_print_range( node * root, int key_start, int key_end,
        bool verbose ) {
    int i;
    int array_size = key_end - key_start + 1;
    int returned_keys[array_size];
    void * returned_pointers[array_size];
    int num_found = find_range( root, key_start, key_end, verbose,
            returned_keys, returned_pointers );
    if (!num_found)
        printf("None found.\n");
    else {
        for (i = 0; i < num_found; i++)
            printf("Key: %d   Location: %lx  Value: %d\n",
                    returned_keys[i],
                    (unsigned long)returned_pointers[i],
                    ((record *)
                     returned_pointers[i])->value);
    }
}


/* Finds keys and their pointers, if present, in the range specified
 * by key_start and key_end, inclusive.  Places these in the arrays
 * returned_keys and returned_pointers, and returns the number of
 * entries found.
 */
int find_range( node * root, int key_start, int key_end, bool verbose,
        int returned_keys[], void * returned_pointers[]) {
    int i, num_found;
    num_found = 0;
    node * n = find_leaf( root, key_start, verbose );
    if (n == NULL) return 0;
    for (i = 0; i < n->num_keys && n->keys[i] < key_start; i++) ;
    if (i == n->num_keys) return 0;
    while (n != NULL) {
        for ( ; i < n->num_keys && n->keys[i] <= key_end; i++) {
            returned_keys[num_found] = n->keys[i];
            returned_pointers[num_found] = n->pointers[i];
            num_found++;
        }
        n = n->pointers[order - 1];
        i = 0;
    }
    return num_found;
}


/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
node * find_leaf( node * root, int key, bool verbose ) {
    int i = 0;
    node * c = root;
    if (c == NULL) {
        if (verbose) 
            printf("Empty tree.\n");
        return c;
    }
    while (!c->is_leaf) {
        if (verbose) {
            printf("[");
            for (i = 0; i < c->num_keys - 1; i++)
                printf("%d ", c->keys[i]);
            printf("%d] ", c->keys[i]);
        }
        i = 0;
        while (i < c->num_keys) {
            if (key >= c->keys[i]) i++;
            else break;
        }
        if (verbose)
            printf("%d ->\n", i);
        c = (node *)c->pointers[i];
    }
    if (verbose) {
        printf("Leaf [");
        for (i = 0; i < c->num_keys - 1; i++)
            printf("%d ", c->keys[i]);
        printf("%d] ->\n", c->keys[i]);
    }
    return c;
}


/* Finds and returns the record to which
 * a key refers.
 */
record * find_tree( node * root, int key, bool verbose ) {
    int i = 0;
    node * c = find_leaf( root, key, verbose );
    if (c == NULL) return NULL;
    for (i = 0; i < c->num_keys; i++)
        if (c->keys[i] == key) break;
    if (i == c->num_keys) 
        return NULL;
    else
        return (record *)c->pointers[i];
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

/* Creates a new record to hold the value
 * to which a key refers.
 */
record * make_record(int value) {
    record * new_record = (record *)malloc(sizeof(record));
    if (new_record == NULL) {
        perror("Record creation.");
        exit(EXIT_FAILURE);
    }
    else {
        new_record->value = value;
    }
    return new_record;
}


/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
node * make_node( void ) {
    node * new_node;
    new_node = malloc(sizeof(node));
    if (new_node == NULL) {
        perror("Node creation.");
        exit(EXIT_FAILURE);
    }
    new_node->keys = malloc( (order - 1) * sizeof(int) );
    if (new_node->keys == NULL) {
        perror("New node keys array.");
        exit(EXIT_FAILURE);
    }
    new_node->pointers = malloc( order * sizeof(void *) );
    if (new_node->pointers == NULL) {
        perror("New node pointers array.");
        exit(EXIT_FAILURE);
    }
    new_node->is_leaf = false;
    new_node->num_keys = 0;
    new_node->parent = NULL;
    new_node->next = NULL;
    return new_node;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
node * make_leaf( void ) {
    node * leaf = make_node();
    leaf->is_leaf = true;
    return leaf;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(node * parent, node * left) {

    int left_index = 0;
    while (left_index <= parent->num_keys && 
            parent->pointers[left_index] != left)
        left_index++;
    return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
node * insert_into_leaf( node * leaf, int key, record * pointer ) {

    int i, insertion_point;

    insertion_point = 0;
    while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
        insertion_point++;

    for (i = leaf->num_keys; i > insertion_point; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->pointers[i] = leaf->pointers[i - 1];
    }
    leaf->keys[insertion_point] = key;
    leaf->pointers[insertion_point] = pointer;
    leaf->num_keys++;
    return leaf;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
node * insert_into_leaf_after_splitting(node * root, node * leaf, int key, record * pointer) {

    node * new_leaf;
    int * temp_keys;
    void ** temp_pointers;
    int insertion_index, split, new_key, i, j;

    new_leaf = make_leaf();

    temp_keys = malloc( order * sizeof(int) );
    if (temp_keys == NULL) {
        perror("Temporary keys array.");
        exit(EXIT_FAILURE);
    }

    temp_pointers = malloc( order * sizeof(void *) );
    if (temp_pointers == NULL) {
        perror("Temporary pointers array.");
        exit(EXIT_FAILURE);
    }

    insertion_index = 0;
    while (insertion_index < order - 1 && leaf->keys[insertion_index] < key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf->keys[i];
        temp_pointers[j] = leaf->pointers[i];
    }

    temp_keys[insertion_index] = key;
    temp_pointers[insertion_index] = pointer;

    leaf->num_keys = 0;

    split = cut(order - 1);

    for (i = 0; i < split; i++) {
        leaf->pointers[i] = temp_pointers[i];
        leaf->keys[i] = temp_keys[i];
        leaf->num_keys++;
    }

    for (i = split, j = 0; i < order; i++, j++) {
        new_leaf->pointers[j] = temp_pointers[i];
        new_leaf->keys[j] = temp_keys[i];
        new_leaf->num_keys++;
    }

    free(temp_pointers);
    free(temp_keys);

    new_leaf->pointers[order - 1] = leaf->pointers[order - 1];
    leaf->pointers[order - 1] = new_leaf;

    for (i = leaf->num_keys; i < order - 1; i++)
        leaf->pointers[i] = NULL;
    for (i = new_leaf->num_keys; i < order - 1; i++)
        new_leaf->pointers[i] = NULL;

    new_leaf->parent = leaf->parent;
    new_key = new_leaf->keys[0];

    return insert_into_parent(root, leaf, new_key, new_leaf);
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
node * insert_into_node(node * root, node * n, 
        int left_index, int key, node * right) {
    int i;

    for (i = n->num_keys; i > left_index; i--) {
        n->pointers[i + 1] = n->pointers[i];
        n->keys[i] = n->keys[i - 1];
    }
    n->pointers[left_index + 1] = right;
    n->keys[left_index] = key;
    n->num_keys++;
    return root;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
node * insert_into_node_after_splitting(node * root, node * old_node, int left_index, 
        int key, node * right) {

    int i, j, split, k_prime;
    node * new_node, * child;
    int * temp_keys;
    node ** temp_pointers;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_pointers = malloc( (order + 1) * sizeof(node *) );
    if (temp_pointers == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = malloc( order * sizeof(int) );
    if (temp_keys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = old_node->pointers[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  
    split = cut(order);
    new_node = make_node();
    old_node->num_keys = 0;
    for (i = 0; i < split - 1; i++) {
        old_node->pointers[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
    }
    old_node->pointers[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
    for (++i, j = 0; i < order; i++, j++) {
        new_node->pointers[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->num_keys++;
    }
    new_node->pointers[j] = temp_pointers[i];
    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++) {
        child = new_node->pointers[i];
        child->parent = new_node;
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(root, old_node, k_prime, new_node);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
node * insert_into_parent(node * root, node * left, int key, node * right) {

    int left_index;
    node * parent;

    parent = left->parent;

    /* Case: new root. */

    if (parent == NULL)
        return insert_into_new_root(left, key, right);

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's pointer to the left 
     * node.
     */

    left_index = get_left_index(parent, left);


    /* Simple case: the new key fits into the node. 
     */

    if (parent->num_keys < order - 1)
        return insert_into_node(root, parent, left_index, key, right);

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(root, parent, left_index, key, right);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
node * insert_into_new_root(node * left, int key, node * right) {

    node * root = make_node();
    root->keys[0] = key;
    root->pointers[0] = left;
    root->pointers[1] = right;
    root->num_keys++;
    root->parent = NULL;
    left->parent = root;
    right->parent = root;
    return root;
}



/* First insertion:
 * start a new tree.
 */
node * start_new_tree(int key, record * pointer) {

    node * root = make_leaf();
    root->keys[0] = key;
    root->pointers[0] = pointer;
    root->pointers[order - 1] = NULL;
    root->parent = NULL;
    root->num_keys++;
    return root;
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
node * insert_tree( node * root, int key, int value ) {

    record * pointer;
    node * leaf;

    /* The current implementation ignores
     * duplicates.
     */

    if (find_tree(root, key, false) != NULL)
        return root;

    /* Create a new record for the
     * value.
     */
    pointer = make_record(value);


    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (root == NULL) 
        return start_new_tree(key, pointer);


    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leaf = find_leaf(root, key, false);

    /* Case: leaf has room for key and pointer.
     */

    if (leaf->num_keys < order - 1) {
        leaf = insert_into_leaf(leaf, key, pointer);
        return root;
    }


    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(root, leaf, key, pointer);
}




// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( node * n ) {

    int i;

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    for (i = 0; i <= n->parent->num_keys; i++)
        if (n->parent->pointers[i] == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int key, node * pointer) {

    int i, num_pointers;

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (n->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (n->pointers[i] != pointer)
        i++;
    for (++i; i < num_pointers; i++)
        n->pointers[i - 1] = n->pointers[i];


    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}


node * adjust_root(node * root) {

    node * new_root;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0)
        return root;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        new_root = root->pointers[0];
        new_root->parent = NULL;
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

    free(root->keys);
    free(root->pointers);
    free(root);

    return new_root;
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!n->is_leaf) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }

    root = delete_entry(root, n->parent, k_prime, n);
    free(n->keys);
    free(n->pointers);
    free(n); 
    return root;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, 
        int k_prime_index, int k_prime) {  

    int i;
    node * tmp;

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
node * delete_entry( node * root, node * n, int key, void * pointer ) {

    int min_keys;
    node * neighbor;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, pointer);

    /* Case:  deletion from the root. 
     */

    if (n == root) 
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (n->num_keys >= min_keys)
        return root;

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

    neighbor_index = get_neighbor_index( n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = n->parent->keys[k_prime_index];
    neighbor = neighbor_index == -1 ? n->parent->pointers[1] : 
        n->parent->pointers[neighbor_index];

    capacity = n->is_leaf ? order : order - 1;

    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
 */
node * delete_tree(node * root, int key) {

    node * key_leaf;
    record * key_record;

    key_record = find_tree(root, key, false);
    key_leaf = find_leaf(root, key, false);
    if (key_record != NULL && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, key_record);
        free(key_record);
    }
    return root;
}


void destroy_tree_nodes(node * root) {
    int i;
    if (root->is_leaf)
        for (i = 0; i < root->num_keys; i++)
            free(root->pointers[i]);
    else
        for (i = 0; i < root->num_keys + 1; i++)
            destroy_tree_nodes(root->pointers[i]);
    free(root->pointers);
    free(root->keys);
    free(root);
}


node * destroy_tree(node * root) {
    destroy_tree_nodes(root);
    return NULL;
}
//////////////////////////////////////////////////////////
//List manager
void init_page_list(){
    head = (page_list *)malloc(sizeof(page_list));
    page_list *new = (page_list *)malloc(sizeof(page_list));
    
    // Offset 0 - 4095 is for header page.
    head->offset = 4096;
    head->is_free = true;
    head->next = new;
    new->offset = 0;
    new->is_free = false;
    new->next = NULL;
}
/*
 * Search first free page. If there is no free page, make new one.
 */
int64_t scan_free_page(){
    int64_t last_offset;
    int64_t next_fp_offset = 0;
    page_list *pointer;
    page_list *new;

    for(pointer = head; pointer->offset != 0 && pointer->next != NULL; pointer = pointer->next){
        if(pointer->is_free == true){
            return pointer->offset;
        }
        last_offset = pointer->offset;
    }
    /* If there is no free page, create new free page. */
    // Modify page list.
    pointer->offset = last_offset + 4096;
    pointer->is_free = true;
    new = (page_list *)malloc(sizeof(page_list));
    new->offset = 0;
    new->is_free = false;
    pointer->next = new;
    new->next = NULL;

    // Create free page. Always this free page is last one.
    fseeko(fp, default_offset + pointer->offset, SEEK_SET);
    fwrite(&next_fp_offset, 8, 1, fp);
    
    return pointer->offset; 
}
/*
 * Find in-use page number.
 */
int64_t scan_use_page(){
    int64_t count = 0;
    page_list *pointer;

    for(pointer = head; pointer->offset != 0 && pointer->next != NULL; pointer = pointer->next){
        if(pointer->is_free == false){
            count++;
        }
    }
    
    return count;
}


/*
 * Change free page to using page.
 */
void change_free_page(int64_t offset){
    page_list *pointer;

    for(pointer = head; pointer->offset != 0 && pointer->next != NULL; pointer = pointer->next){
        if(pointer->offset == offset){
            pointer->is_free = false;
        }
    }    
}

/*
 * Change using page to free page.
 */
void vacuum_using_page(int64_t offset){
    page_list *pointer;
    // New free page will be used.

    for(pointer = head; pointer->offset != 0 && pointer->next != NULL; pointer = pointer->next){
        if(pointer->offset == offset){
            pointer->is_free = true;
        }
    }
}

///////////////////////////////////////////////////////////////
//Page manager
/*
 * Initialize header page.
 */
void init_header_page(){
    int64_t fp_offset = 4096;
    // Root page is not created yet.
    int64_t rp_offset = 0;
    int64_t pnum = 1;

    /* Seek from default offset.
     * If new file, default offset will be 0.
     * Otherwise, default offset will be another number.
     */
    fseeko(fp, default_offset, SEEK_SET);
    fwrite(&fp_offset, 8, 1, fp);
    fwrite(&rp_offset, 8, 1, fp);
    fwrite(&pnum, 8, 1, fp);
}
void modify_header_page(int64_t rp_offset){
    int64_t fp_offset = scan_free_page();
    int64_t pnum = scan_use_page() + 1;

    // Move file pointer to header page location.
    fseeko(fp, default_offset, SEEK_SET); 
    fwrite(&fp_offset, 8, 1, fp);
    fwrite(&rp_offset, 8, 1, fp); 
    fwrite(&pnum, 8, 1, fp);
}

////////////////////////////////////////////////////////////////
//API
/*
 * Open existing data file using pathname or create one if not existed.
 * If success, return 0. Otherwise, return non-zero value.
 */
int open_db(char *pathname){
    fp = fopen(pathname, "wb+");
    if(fp == NULL){
        fprintf(stderr, "Error : file path\n");
        exit(EXIT_FAILURE); 
    }
    // Calculating default offset.
    // ftello function uses off_t type.
    fseeko(fp, 0, SEEK_END);
    default_offset = ftello(fp);

    // Initialize page list.
    init_page_list();

    // Initialize header page.
    init_header_page();

    // Synchronize
    fflush(fp);

    return 0;
}
//////////////////////////////////////////////////////////
//INSERTION
int64_t init_leaf_page(int64_t offset){
    int64_t p_offset = 0;
    int64_t rs_offset = 0;
    int is_leaf = 1;
    int num_keys = 0;

    fseeko(fp, default_offset + offset, SEEK_SET);
    // Parent page offset : initially zero.
    fwrite(&p_offset, 8, 1, fp);
    // Is Leaf : True.
    fwrite(&is_leaf, 4, 1, fp);
    // Number of keys : zero.
    fwrite(&num_keys, 4, 1, fp);
    
    // Move file pointer.
    fseeko(fp, default_offset + offset + 120, SEEK_SET);
    // Right sibiling page offset : zero(Rightmost leaf page).
    fwrite(&rs_offset, 8, 1, fp);

    return offset;
}
int64_t init_internal_page(int64_t offset, int64_t p_offset){
    // Setting page header.
    int is_leaf = 0, num_keys = 0;

    fseeko(fp, default_offset + offset, SEEK_SET);
    // Parent page offset.
    fwrite(&p_offset, 8, 1, fp);
    // Is Leaf : Fales.
    fwrite(&is_leaf, 4, 1, fp);
    // Number of keys : zero.
    fwrite(&num_keys, 4, 1, fp);

    return offset;
}
int get_left_offset(int64_t p_offset, int64_t l_offset){
    int64_t left_offset = 0, target_offset;
    int num_keys;

    // Setting number of keys. : Parent page
    fseeko(fp, default_offset + p_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    while(left_offset <= num_keys){
        // Setting offset.
        if(left_offset == 0)
            fseeko(fp, default_offset + p_offset + 120, SEEK_SET);
        else
            fseeko(fp, default_offset + p_offset + 128 + (left_offset-1)*16 + 8, SEEK_SET);
        fread(&target_offset, 8, 1, fp);

        if(l_offset == target_offset){
            break;
        }
        left_offset++;
    }
    
    return left_offset;
}
int64_t insert_into_leaf_page(int64_t leaf_offset, int64_t key, char *value){
    int i, insertion_point, num_keys;
    int64_t target_key, temp_key;
    char temp_value[120];
    

    insertion_point = 0;

    // Setting number of keys. : Leaf page
    fseeko(fp, default_offset + leaf_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    while(insertion_point < num_keys){
        // Setting target key. : Leaf page
        fseeko(fp, default_offset + leaf_offset + 128 + insertion_point*128, SEEK_SET);
        fread(&target_key, 8, 1, fp);
        if(key <= target_key){
            break;
        }
        insertion_point++;
    }

    // Move existing memory.
    for(i = num_keys; i > insertion_point; i--){
        fseeko(fp, default_offset + leaf_offset + 128 + (i-1)*128, SEEK_SET);
        fread(&temp_key, 8, 1, fp);
        fread(&temp_value, 1, 120, fp);

        fseeko(fp, default_offset + leaf_offset + 128 + (i)*128, SEEK_SET);
        fwrite(&temp_key, 8, 1, fp);
        fwrite(&temp_value, 1, 120, fp);
    }

    // Write new key & value.
    fseeko(fp, default_offset + leaf_offset + 128 + insertion_point*128, SEEK_SET);
    fwrite(&key, 8, 1, fp);
    fwrite(value, 1, 120, fp);
    
    // Modify number of keys.
    num_keys++;
    fseeko(fp, default_offset + leaf_offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);

    return leaf_offset;
}
int64_t insert_into_leaf_page_after_splitting(int64_t rp_offset, int64_t leaf_offset, int64_t key, char *value){
    int64_t new_leaf_offset, *temp_keys, target_key, new_key, temp_offset = 0, p_offset;
    int insertion_index, split, i, j, num_keys, new_num_keys = 0,zero = 0;
    char **temp_values;
    char zero_v[120];

    // Initialize zero.
    for(i = 0; i < 120; i++){
        zero_v[i] = 0;
    }

    // Make new leaf page.
    new_leaf_offset = init_leaf_page(scan_free_page());

    // After using free page, change free page which is used and modify header page.
    change_free_page(new_leaf_offset);
    modify_header_page(rp_offset);

    // Make temp key & value.
    temp_keys = (int64_t *)malloc(leaf_order * sizeof(int64_t));
    temp_values =(char **)malloc(leaf_order * sizeof(char *));
    
    for(i = 0; i < leaf_order; i++){
        temp_values[i] = (char *)malloc(sizeof(char) * 120);
    }

    insertion_index = 0;

    /* Setting insertion index. */
    while(insertion_index < leaf_order -1){
        // Setting target key. : Leaf page
        fseeko(fp, default_offset + leaf_offset + 128 + 128*insertion_index, SEEK_SET);
        fread(&target_key, 8, 1, fp);
        if(target_key >= key){
            break;
        }
        insertion_index++;
    }

    /* Move existing key & value to temporal page. */
    // Setting number of keys. : Leaf page
    fseeko(fp, default_offset + leaf_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);
    for(i = 0, j = 0; i < num_keys; i++, j++){
        if(j == insertion_index)
            j++;
        fseeko(fp, default_offset + leaf_offset + 128 + 128*i, SEEK_SET);
        fread((temp_keys+j), 8, 1, fp);
        fread(*(temp_values+j), 1, 120, fp);
    }
    // Insert new key & value.
    temp_keys[insertion_index] = key;
    memmove(temp_values[insertion_index], value, 120);

    // Setting existing number of keys to zero : Leaf page.
    num_keys = 0;
    fseeko(fp, default_offset + leaf_offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);

    split = cut(leaf_order - 1);

    /* Move temporal key & value to existing leaf page and new leaf page. */
    for(i = 0; i < split; i++){
        fseeko(fp, default_offset + leaf_offset + 128 + 128*i, SEEK_SET);
        fwrite((temp_keys+i), 8, 1, fp);
        fwrite(*(temp_values+i), 1, 120, fp);
        num_keys++; 
    }
    // Setting number of keys. : Existing leaf page.
    fseeko(fp, default_offset + leaf_offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);

    for(i = split, j = 0; i < leaf_order; i++, j++){
        fseeko(fp, default_offset + new_leaf_offset + 128 + 128*j, SEEK_SET);
        fwrite((temp_keys+i), 8, 1, fp);
        fwrite(*(temp_values+i), 1, 120, fp);
        new_num_keys++;
    }
    // Setting number of keys. : New leaf page.
    fseeko(fp, default_offset + new_leaf_offset + 12, SEEK_SET);
    fwrite(&new_num_keys, 4, 1, fp);

    // Free temporal keys & values.
    free(temp_keys);
    for(i = 0; i < leaf_order; i++){
        free(temp_values[i]);
    }
    free(temp_values);

    /* Change right sibiling offset. */
    // Setting right sibiling offset. : leaf page & new leaf page
    fseeko(fp, default_offset + leaf_offset + 120, SEEK_SET);
    fread(&temp_offset, 8, 1, fp);

    fseeko(fp, default_offset + new_leaf_offset + 120, SEEK_SET);
    fwrite(&temp_offset, 8, 1, fp);

    fseeko(fp, default_offset + leaf_offset + 120, SEEK_SET);
    fwrite(&new_leaf_offset, 8, 1, fp);

    // Reinitialize value which is not used.
    for(i = num_keys; i < leaf_order - 1; i++){
        fseeko(fp, default_offset + leaf_offset + 128 + 128*i, SEEK_SET);
        fwrite(&zero, 8, 1, fp);
        fwrite(zero_v, 1, 120, fp);
    }
    for(i = new_num_keys; i < leaf_order - 1; i++){
        fseeko(fp, default_offset + new_leaf_offset + 128 + 128*i, SEEK_SET);
        fwrite(&zero, 8, 1, fp);
        fwrite(zero_v, 1, 120, fp);
    }

    /* Setting parent & new key. */
    // Setting parent offset.
    fseeko(fp, default_offset + leaf_offset, SEEK_SET);
    fread(&p_offset, 8, 1, fp);
    fseeko(fp, default_offset + new_leaf_offset, SEEK_SET);
    fwrite(&p_offset, 8, 1, fp);
    // Setting new key.
    fseeko(fp, default_offset + new_leaf_offset + 128, SEEK_SET);
    fread(&new_key, 8, 1, fp);

    // Return
    return insert_into_parent_page(rp_offset, leaf_offset, new_key, new_leaf_offset);
}
int64_t insert_into_internal_page(int64_t rp_offset, int64_t p_offset, int64_t left_offset, int64_t key, int64_t r_offset){
    int64_t temp_key, temp_offset;
    int num_keys, i;

    // Setting number of keys. : Parent page
    fseeko(fp, default_offset + p_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    // Move existing memory.
    for(i = num_keys; i > left_offset; i--){
        // Key
        fseeko(fp, default_offset + p_offset + 128 + (i-1)*16, SEEK_SET);
        fread(&temp_key, 8, 1, fp);

        fseeko(fp, default_offset + p_offset + 128 + i*16, SEEK_SET);
        fwrite(&temp_key, 8, 1, fp);

        // Offset
        if(i == 0){
            fseeko(fp, default_offset + p_offset + 120, SEEK_SET);
            fread(&temp_offset, 8, 1, fp);
            fseeko(fp, default_offset + p_offset + 128 + 8, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);
        }
        else{
            fseeko(fp, default_offset + p_offset + 128 + (i-1)*16 + 8, SEEK_SET );
            fread(&temp_offset, 8, 1, fp);
            fseeko(fp, default_offset + p_offset + 128 + i*16 + 8, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);
        }
    }
    // Write new key & offset.
    fseeko(fp, default_offset + p_offset + 128 + (left_offset)*16, SEEK_SET);
    fwrite(&key, 8, 1, fp);
    fwrite(&r_offset, 8, 1, fp);

    // Modify number of keys. : Parent page
    num_keys++;
    fseeko(fp, default_offset + p_offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);

    return rp_offset;
}
int64_t insert_into_internal_page_after_splitting(int64_t rp_offset, int64_t old_offset, int left_offset, int64_t key, int64_t r_offset){
    int i, j, split, old_num_keys, new_num_keys;
    int64_t k_prime, *temp_keys, *temp_offsets, new_internal_offset, child_offset, p_offset;

    /* Create temporal keys & values. */
    temp_keys = (int64_t *)malloc(internal_order * sizeof(int64_t));
    temp_offsets = (int64_t *)malloc((internal_order + 1) * sizeof(int64_t));

    /* Move existing key & value to temporal key & offset. */
    // Setting number of keys : Old internal page
    fseeko(fp, default_offset + old_offset + 12, SEEK_SET);
    fread(&old_num_keys, 4, 1, fp);

    // Move offsets.
    for(i = 0, j = 0; i < old_num_keys + 1; i++, j++){
        if(j == left_offset + 1)
            j++;
        if(i == 0)
            fseeko(fp, default_offset + old_offset + 120, SEEK_SET);
        else
            fseeko(fp, default_offset + old_offset + 128 + 8 + 16*(i-1), SEEK_SET);

        fread((temp_offsets+j), 8, 1, fp);
    }
    // Move keys.
    for(i = 0, j = 0; i < old_num_keys; i++, j++){
        if(j == left_offset)
            j++;
        fseeko(fp, default_offset + old_offset + 128 + 16*i, SEEK_SET);
        fread((temp_keys+j), 8, 1, fp);
    }
    temp_offsets[left_offset + 1] = r_offset;
    temp_keys[left_offset] = key;

    /* Create new internal page then move temporal key & value to existing internal page and new internal page. */
    // Setting parent offset.
    fseeko(fp, default_offset + old_offset, SEEK_SET);
    fread(&p_offset, 8, 1, fp);

    split = cut(internal_order);
    // Setting new internal page's parent offset. 
    new_internal_offset = init_internal_page(scan_free_page(), p_offset);
    // Change free page.
    change_free_page(new_internal_offset);
    // Modify header page.
    modify_header_page(rp_offset);

    // Setting number of keys. : New internal page
    fseeko(fp, default_offset + new_internal_offset + 12, SEEK_SET);
    fread(&new_num_keys, 4, 1, fp);

    old_num_keys = 0;

    for(i = 0; i < split - 1; i++){
        if(i == 0){
            fseeko(fp, default_offset + old_offset + 120, SEEK_SET);
            fwrite((temp_offsets + i), 8, 1, fp);
            fseeko(fp, default_offset + old_offset + 128, SEEK_SET);
            fwrite((temp_keys + i), 8, 1, fp);
        }
        else{
            fseeko(fp, default_offset + old_offset + 128 + 16*(i-1) + 8, SEEK_SET);    
            fwrite((temp_offsets + i), 8, 1, fp);
            fseeko(fp, default_offset + old_offset + 128 + 16*(i), SEEK_SET);
            fwrite((temp_keys + i), 8, 1, fp);
        }
        old_num_keys++;
    }
    fseeko(fp, default_offset + old_offset + 128 + 16*(i-1) + 8, SEEK_SET);
    fwrite((temp_offsets + i), 8, 1, fp);
    k_prime = temp_keys[split - 1];

    for(++i, j = 0; i < internal_order; i++, j++){
        if(j == 0){
            fseeko(fp, default_offset + new_internal_offset + 120, SEEK_SET);
            fwrite((temp_offsets + i), 8, 1, fp);
            fseeko(fp, default_offset + new_internal_offset + 128, SEEK_SET);
            fwrite((temp_keys + i), 8, 1, fp);
        }
        else{
            fseeko(fp, default_offset + new_internal_offset + 128 + 16*(j-1) + 8, SEEK_SET);
            fwrite((temp_offsets + i), 8, 1, fp);
            fseeko(fp, default_offset + new_internal_offset + 128 + 16*(j), SEEK_SET);
            fwrite((temp_keys + i), 8, 1, fp);
        }
        new_num_keys++;
    }
    fseeko(fp, default_offset + new_internal_offset + 128 + 16*(j-1) + 8, SEEK_SET);
    fwrite((temp_offsets + i), 8, 1, fp);

    // Free temporal key & values.
    free(temp_keys);
    free(temp_offsets);
    
    // Parent setting : Already done in above.

    /* Change number of keys : New & Old */
    fseeko(fp, default_offset + old_offset + 12, SEEK_SET);
    fwrite(&old_num_keys, 4, 1, fp);
    fseeko(fp, default_offset + new_internal_offset + 12, SEEK_SET);
    fwrite(&new_num_keys, 4, 1, fp);

    // Child setting
    for(i = 0; i <= new_num_keys; i++){
        // Setting child offset.
        if(i == 0){
            fseeko(fp, default_offset + new_internal_offset + 120, SEEK_SET);
        }
        else{
            fseeko(fp, default_offset + new_internal_offset + 128 + 16*(i-1) + 8, SEEK_SET);
        }
        fread(&child_offset, 8, 1, fp);
        fseeko(fp, default_offset + child_offset, SEEK_SET);
        fwrite(&new_internal_offset, 8, 1, fp);
    }

    // Return
    return insert_into_parent_page(rp_offset, old_offset, k_prime, new_internal_offset);
}
int64_t insert_into_parent_page(int64_t rp_offset, int64_t l_offset, int64_t key, int64_t r_offset){
    int64_t left_offset, p_offset;
    int num_keys;
    
    // Setting left page's parent page offset.
    fseeko(fp, default_offset + l_offset, SEEK_SET);
    fread(&p_offset, 8, 1, fp);

    /* Case : new root. */

    if(p_offset == 0){
        return insert_into_new_root_page(l_offset, key, r_offset);
    }
    /* Case : leaf of node. (Remainder of function body.) */

    // Find the parent page's offset to the left page.
    left_offset = get_left_offset(p_offset, l_offset);

    /* Simple case : the new key fits into the page. */
    // Setting number of keys. : parent page
    fseeko(fp, default_offset + p_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    if(num_keys < internal_order -1)
        return insert_into_internal_page(rp_offset, p_offset, left_offset, key, r_offset);
        
    /* Harder case : split a page in order to preserve the B+ tree properties. */
    // Return
    return insert_into_internal_page_after_splitting(rp_offset, p_offset, left_offset, key, r_offset);
}
int64_t insert_into_new_root_page(int64_t l_offset, int64_t key, int64_t r_offset){
    int64_t new_rp_offset = scan_free_page();
    int64_t p_offset = 0; 
    int is_leaf = 0, num_keys = 1;


    // After using free page, change free page which is used and modify header page.
    change_free_page(new_rp_offset);
    modify_header_page(new_rp_offset);

    // Move file pointer to new root page.
    fseeko(fp, default_offset + new_rp_offset, SEEK_SET);
    // First, set page header.
    fwrite(&p_offset, 8, 1, fp);
    fwrite(&is_leaf, 4, 1, fp);
    fwrite(&num_keys, 4, 1, fp);

    /* Move file pointer & write offset. */
    fseeko(fp, default_offset + new_rp_offset + 120, SEEK_SET);
    // Insert left page offset.
    fwrite(&l_offset, 8, 1, fp);

    /* Move file pointer & write key + offset. */
    fseeko(fp, default_offset + new_rp_offset + 128, SEEK_SET);
    // Insert key.
    fwrite(&key, 8, 1, fp);
    // Insert right page offset.
    fwrite(&r_offset, 8, 1, fp);

    // Move file pointer & modify parent page offset : left page's page header section.
    fseeko(fp, default_offset + l_offset, SEEK_SET);
    fwrite(&new_rp_offset, 8, 1, fp);

    // Move file pointer & modify parent page offset : right page's page header section.
    fseeko(fp, default_offset + r_offset, SEEK_SET);
    fwrite(&new_rp_offset, 8, 1, fp);

    return new_rp_offset;
}
/*
 * After calling this function, modify header page.
 */
int64_t start_new_tree_page(int64_t key, char *value){
    // Find free page, and change to root page(leaf page).
    int64_t rp_offset = scan_free_page();
    int num_keys;

    // Initialize leaf page.
    init_leaf_page(rp_offset);
    // Change free page to using page.
    change_free_page(rp_offset);
    // Modify header page.
    modify_header_page(rp_offset);

    // Move file pointer & write key + value.
    fseeko(fp, default_offset + rp_offset + 128, SEEK_SET);
    fwrite(&key, 8, 1, fp);
    fwrite(value, 1, 120, fp);

    // Move file pointer & modify number of keys.
    fseeko(fp, default_offset + rp_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);
    num_keys++;
    fseeko(fp, default_offset + rp_offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);

    // Return root new page offset.
    return rp_offset;
}

/*
 * Insert input key/value(record) to data file at the right place.
 * If success, return 0. Otherwise, return non-zero value.
 */
int insert(int64_t key, char *value){
    int64_t rp_offset,lp_offset;
    int num_keys;

    // Setting root page offset.
    fseeko(fp, default_offset + 8, SEEK_SET); 
    fread(&rp_offset, 8, 1, fp);

    // Ignore duplicates.
    if(find(key) != NULL)
        // Insert fail. Return non-zero value.
        return 1;

    // Do not need to make record.
    
    /* Case : the tree does not exist yet.
     * Create new root page.
     */
    if(rp_offset == 0){
        // Create new root page.
        rp_offset = start_new_tree_page(key, value);
        // Modify header page. -> Already done.
        // Synchronize
        fflush(fp);

        return 0;
    }
    
    // Case : the tree already exists.
    // Find leaf page by key.

    lp_offset = find_leaf_page(rp_offset, key);

    // Case : leaf has room for key and pointer.
    
    // Setting number of keys.
    fseeko(fp, default_offset+lp_offset+12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    if(num_keys < leaf_order - 1){
        // Insert into leaf page.
        insert_into_leaf_page(lp_offset, key, value);
        // Synchronize
        fflush(fp);

        return 0;
    }

    // Case : leaf must be split
    // Insert into leaf after splitting.
    insert_into_leaf_page_after_splitting(rp_offset, lp_offset, key, value);
    // Synchronize
    fflush(fp);
    return 0;
}

//////////////////////////////////////////////////////////
//Find
/*
 * Determine whether it is leaf page or not.
 */
int leaf_page_judge(int64_t offset){
    int is_leaf;

    // Read is leaf value.
    fseeko(fp, default_offset+offset+8, SEEK_SET); 
    fread(&is_leaf, 4, 1, fp);

    if(is_leaf)
        return 1;
    else
        return 0;
}

/*
 *
 */
int64_t find_leaf_page(int64_t rp_offset, int64_t key){
    int64_t offset = rp_offset, target_key, temp;
    int num_keys,i = 0;

    // Empty tree.
    if(offset == 0){
        return offset;
    }
    while(!leaf_page_judge(offset)){
        i = 0;
        // Setting number of keys.
        fseeko(fp, default_offset+offset+12, SEEK_SET);
        fread(&num_keys, 4, 1, fp);
        
        while(i < num_keys){
            // Setting target key.
            fseeko(fp, default_offset+offset+128+(16*i), SEEK_SET);
            fread(&target_key, 8, 1, fp);

            if(key >= target_key)
                i++;
            else
                break;
        }
        // Change offset.
        if(i==0){
            fseeko(fp, default_offset+offset+120, SEEK_SET);
            fread(&temp, 8, 1, fp);
        }
        else{
            fseeko(fp, default_offset+offset+128+(16*(i-1))+8, SEEK_SET);
            fread(&temp, 8, 1, fp);
        }
        offset = temp;
    }

    return offset;
}
/*
 * Find the record containing input key.
 * If found matching key, return matched value string. Otherwise, return NULL.
 */
char * find(int64_t key){
    int64_t rp_offset,lp_offset,target_key;
    int i = 0, num_keys;
    char *value;

    // Memory allocation
    value = (char*)malloc(120*sizeof(char));

    // Setting root page offset. (From header page)
    fseeko(fp, default_offset + 8, SEEK_SET);
    fread(&rp_offset, 8, 1, fp);    

    // Setting leaf page offset.
    lp_offset = find_leaf_page(rp_offset, key);

    // In case of empty tree.
    if(lp_offset == 0){
        free(value);
        return NULL;
    }

    // Setting number of keys.
    fseeko(fp, default_offset+lp_offset+12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    for(i = 0; i < num_keys; i++){
        // Setting target key.
        fseeko(fp, default_offset+lp_offset+128+(i*128), SEEK_SET);
        fread(&target_key, 8, 1, fp);

        if(target_key == key)
            break;
    }
    if(i == num_keys){
        free(value);
        return NULL;
    }
    else{
        // Setting value.
        fseeko(fp, default_offset+lp_offset+128+(i*128)+8, SEEK_SET);
        fread(value, 1, 120, fp);

        return value;
    }

}

///////////////////////////////////////////////////////////
//DELETE
int get_neighbor_page_index(int64_t offset){
    int i,num_keys;
    int64_t p_offset, target_offset;

    // Setting parent page offset.
    fseeko(fp, default_offset + offset, SEEK_SET);
    fread(&p_offset, 8, 1, fp);
    
    // Setting number of keys. : Parent page of offset
    fseeko(fp, default_offset + p_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    // Parent page offset : always internal page!
    for(i = 0; i <= num_keys; i++){
        // Setting target offset.
        if(i == 0){
            fseeko(fp, default_offset + p_offset + 120, SEEK_SET);
        }
        else{
            fseeko(fp, default_offset + p_offset + 128 + 16*(i-1) + 8, SEEK_SET);
        }
        fread(&target_offset, 8, 1, fp);

        if(target_offset == offset){
            return i -1;
        }
    }

    // Error state.
    exit(EXIT_FAILURE);
}
int64_t remove_entry_from_page(int64_t offset, int64_t key, char *value, int64_t key_offset){
    int i, num_off_values, is_leaf, num_keys;
    int64_t target_key, target_offset, temp_offset, temp_key;
    char target_value[120], temp_value[120];
    char zero[120];
    int64_t zero_64 = 0;

    // Initialize zero
    for(i = 0; i < 120; i++){
        zero[i] = 0;
    }

    // Setting is leaf : Offset page
    fseeko(fp, default_offset + offset + 8, SEEK_SET);
    fread(&is_leaf, 4, 1, fp);

    // Setting number of keys : Offset page
    fseeko(fp, default_offset + offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    /* Remove the key and shift other keys accordingly. */
    i = 0;
    // Case 1 : leaf page
    if(is_leaf){
        // Setting target key : Offset page
        fseeko(fp, default_offset + offset + 128 + 128*i, SEEK_SET);
        fread(&target_key, 8, 1, fp);
        while(target_key != key){
            i++;
            fseeko(fp, default_offset + offset + 128 + 128*i, SEEK_SET);
            fread(&target_key, 8, 1, fp);
        }
        for(++i; i < num_keys; i++){
            fseeko(fp, default_offset + offset + 128 + 128*i, SEEK_SET);
            fread(&temp_key, 8, 1, fp);

            fseeko(fp, default_offset + offset + 128 + 128*(i-1), SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
        }
    }
    // Case 2 : internal page
    else{
        // Setting target key : Offset page
        fseeko(fp, default_offset + offset + 128 + 16*i, SEEK_SET);
        fread(&target_key, 8, 1, fp);
        while(target_key != key){
            i++;
            fseeko(fp, default_offset + offset + 128 + 16*i, SEEK_SET);
            fread(&target_key, 8, 1, fp);
        }
        for(++i; i < num_keys; i++){
            fseeko(fp, default_offset + offset + 128 + 16*i, SEEK_SET);
            fread(&temp_key, 8, 1, fp);

            fseeko(fp, default_offset + offset + 128 + 16*(i-1), SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
        }
    }

    /* Remove the offset or value and shift other offsets or values accordingly. */
    // First determine number of offsets or values.
    num_off_values = is_leaf ? num_keys : num_keys + 1; 
    i = 0;
    // Case1 : leaf page
    if(is_leaf){
        // Setting target value
        fseeko(fp, default_offset + offset + 128 + 128*i + 8, SEEK_SET);
        fread(&target_value, 1, 120, fp);
        while(!strcmp(target_value, value)){
            i++;
            fseeko(fp, default_offset + offset + 128 + 128*i + 8, SEEK_SET);
            fread(&target_value, 1, 120, fp); 
        }
        for(++i; i < num_off_values; i++){
            fseeko(fp, default_offset + offset + 128 + 128*i + 8, SEEK_SET);
            fread(&temp_value, 1, 120, fp);

            fseeko(fp, default_offset + offset + 128 + 128*(i-1) + 8, SEEK_SET);
            fwrite(&temp_value, 1, 120, fp);

        }
    }
    // Case 2 : internal page
    else{
        // Setting target offset
        fseeko(fp, default_offset + offset + 120, SEEK_SET);
        fread(&target_offset, 8, 1, fp);
        while(key_offset != target_offset){
            i++;
            fseeko(fp, default_offset + offset + 128 + 16*(i-1) + 8, SEEK_SET);
            fread(&target_offset, 8, 1, fp);
        }
        for(++i; i < num_off_values; i++){
            if(i == 1){
                fseeko(fp, default_offset + offset + 128 + 16*(i-1) + 8, SEEK_SET);
                fread(&temp_offset, 8, 1, fp);

                fseeko(fp, default_offset + offset + 120, SEEK_SET);
                fwrite(&temp_offset, 8, 1, fp);
            }
            else{
                fseeko(fp, default_offset + offset + 128 + 16*(i-1) + 8, SEEK_SET);
                fread(&temp_offset, 8, 1, fp);

                fseeko(fp, default_offset + offset + 128 + 16*(i-2) + 8, SEEK_SET);
                fwrite(&temp_offset, 8, 1, fp);
            }
        }
    }

    // One key fewer.
    num_keys--;
    // Modify page header.
    fseeko(fp, default_offset + offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);

    // Set the other values or offsets to NULL for tidiness.
    if(is_leaf){
        for(i = num_keys; i < leaf_order - 1; i++){
            fseeko(fp, default_offset + offset + 128 + 128*i + 8, SEEK_SET);
            fwrite(zero, 1, 120, fp);
        }
    }
    else{
        for(i = num_keys + 1; i < internal_order; i++){
            fseeko(fp, default_offset + offset + 128 + 16*(i-1) + 8, SEEK_SET);
            fwrite(&zero_64, 8, 1, fp);
        }
    }

    return offset;
}
int64_t adjust_root_page(int64_t rp_offset){
    int64_t new_rp_offset, p_offset = 0;
    int num_keys, is_leaf;

    /* Case : nonempty root. Nothing to be done */
    // Setting number of keys : Root page
    fseeko(fp, default_offset + rp_offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    if(num_keys > 0)
        return rp_offset;

    /* Case : empty root. */
    // Setting is_leaf
    fseeko(fp, default_offset + rp_offset + 8, SEEK_SET);
    fread(&is_leaf, 4, 1, fp);

    if(!is_leaf){
        // Not leaf page : Internal page
        // If it has a child, promote the first child as the new root.
        fseeko(fp, default_offset + rp_offset + 120, SEEK_SET);
        fread(&new_rp_offset, 8, 1, fp);

        // Change parent offset : New root page
        fseeko(fp, default_offset + new_rp_offset, SEEK_SET);
        fwrite(&p_offset, 8, 1, fp);

        // Modify header page.
        modify_header_page(new_rp_offset);
    }

    // If it is a leaf, the the whole tree is empty.
    else{
        new_rp_offset = 0;
        // Modify header page.
        modify_header_page(new_rp_offset);
    }
    
    return new_rp_offset;
}
int64_t coalesce_pages(int64_t rp_offset, int64_t offset, int64_t neighbor_offset, int neighbor_index, int64_t k_prime){
    int i, j, neighbor_insertion_index, offset_end, is_leaf, num_keys = 0, neighbor_num_keys = 0;
    int64_t temp_key, temp_offset, p_offset;
    char temp_value[120];
    char zero[4096];
    
    // Initialize zero
    for(i = 0; i < 4096; i++){
        zero[i] = 0;
    }

    /* Swap neighbor with node if node is on the extreme left and neighbor is to its right. */
    if(neighbor_index == -1){
        temp_offset = offset;
        offset = neighbor_offset;
        neighbor_offset = temp_offset;
    }

    // Setting is leaf. : Offset page
    fseeko(fp, default_offset + offset + 8, SEEK_SET);
    fread(&is_leaf, 4, 1, fp);

    // Setting number of keys : Neighbor page
    fseeko(fp, default_offset + neighbor_offset + 12, SEEK_SET);
    fread(&neighbor_num_keys, 4, 1, fp);
    // Setting number of keys : Offset page
    fseeko(fp, default_offset + offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    neighbor_insertion_index = neighbor_num_keys;

    /* Case : nonleaf page */
    if(!is_leaf){
        /* Append k_prime */
        fseeko(fp, default_offset + neighbor_offset + 128 + 16*(neighbor_insertion_index), SEEK_SET);
        fwrite(&k_prime, 8, 1, fp);

        neighbor_num_keys++;

        offset_end = num_keys;

        for(i = neighbor_insertion_index + 1, j = 0; j < offset_end; i++, j++){
            /* Setting temp key & offset from offset page. */
            // Offset
            if(j == 0){
                fseeko(fp, default_offset + offset + 120, SEEK_SET);
                fread(&temp_offset, 8, 1, fp);
            }
            else{
                fseeko(fp, default_offset + offset + 128 + 16*(j-1) + 8, SEEK_SET);
                fread(&temp_offset, 8, 1, fp);
            }
            // Key
            fseeko(fp, default_offset + offset + 128 + 16*j, SEEK_SET);
            fread(&temp_key, 8, 1, fp);
            
            /* Move temp key & offset to neighbor page. */
            // Offset
            fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i-1) + 8, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);    
            // Key
            fseeko(fp, default_offset + neighbor_offset + 128 + 16*i, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
            
            neighbor_num_keys++;
            num_keys--;
        }
        /* The number of offsets is always one more than the number fo keys. */
        fseeko(fp, default_offset + offset + 128 + 16*(j-1) + 8, SEEK_SET);
        fread(&temp_offset, 8, 1, fp);

        fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i-1) + 8, SEEK_SET);
        fwrite(&temp_offset, 8, 1, fp);

        /* All children must now point up to the same parent. */

        for(i = 0; i < neighbor_num_keys + 1; i++){
            if(i == 0){
                fseeko(fp, default_offset + neighbor_offset + 120, SEEK_SET);
                fread(&temp_offset, 8, 1, fp);
            }
            else{
                fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i-1) + 8, SEEK_SET);
                fread(&temp_offset, 8, 1, fp);
            }

            // Modify page header.
            fseeko(fp, default_offset + temp_offset, SEEK_SET);
            fwrite(&neighbor_offset, 8, 1, fp);
        }
        // Modify number of keys.
        fseeko(fp, default_offset + offset + 12, SEEK_SET);
        fwrite(&num_keys, 4, 1, fp);

        fseeko(fp, default_offset + neighbor_offset + 12, SEEK_SET);
        fwrite(&neighbor_num_keys, 4, 1, fp);
    }
    /* Case : leaf page */
    else{
        for(i = neighbor_insertion_index, j = 0; j < num_keys; i++, j++){
            /* Setting temp key & value from offset page. */
            fseeko(fp, default_offset + offset + 128 + j*128, SEEK_SET);
            fread(&temp_key, 8, 1, fp);
            fread(temp_value, 1, 120, fp);

            /* Move temp key & value to neighbor page. */
            fseeko(fp, default_offset + neighbor_offset + 128 + i*128, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
            fwrite(temp_value, 1, 120 , fp);

            // Increase number of keys.
            neighbor_num_keys++;
        }
        /* Setting right sibiling page offset. */
        fseeko(fp, default_offset + offset + 120, SEEK_SET);
        fread(&temp_offset, 8, 1, fp);
        fseeko(fp, default_offset + neighbor_offset + 120, SEEK_SET);
        fwrite(&temp_offset, 8, 1, fp);

        // Modify number of keys.
        fseeko(fp, default_offset + neighbor_offset + 12, SEEK_SET);
        fwrite(&neighbor_num_keys, 4, 1, fp);
    }
    // Setting parent offset : Offset page.
    fseeko(fp, default_offset + offset, SEEK_SET);
    fread(&p_offset, 8, 1, fp);

    delete_entry_from_page(rp_offset, p_offset, k_prime, NULL, offset);

    // Delete offset page.
    fseeko(fp, default_offset + offset, SEEK_SET);
    fwrite(zero, 1, 4096, fp);
    vacuum_using_page(offset);

    return rp_offset;
}
int64_t redistribute_pages(int64_t rp_offset, int64_t offset, int64_t neighbor_offset, int neighbor_index, int k_prime_index, int64_t k_prime){
    int i, is_leaf, num_keys, neighbor_num_keys;
    int64_t temp_offset, temp_key, p_offset, zero_64 = 0;
    char temp_value[120], zero[120];

    // Initialize zero
    for(i = 0; i < 120; i++){
        zero[i] = 0;
    }

    // Setting is_leaf : Offset page
    fseeko(fp, default_offset + offset + 8, SEEK_SET);
    fread(&is_leaf, 4, 1, fp);
    // Setting number of keys : Offset page
    fseeko(fp, default_offset + offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);
    // Setting number of keys : Neighbor page
    fseeko(fp, default_offset + neighbor_offset + 12, SEEK_SET);
    fread(&neighbor_num_keys, 4, 1, fp);

    /* Case : offset page has a neighbor to the left. */
    if(neighbor_index != -1){
        // Case : internal page
        if(!is_leaf){
            fseeko(fp, default_offset + offset + 128 + 16*(num_keys-1) + 8, SEEK_SET);
            fread(&temp_offset, 8, 1, fp);

            fseeko(fp, default_offset + offset + 128 + 16*(num_keys) + 8, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);

            for(i = num_keys; i > 0; i--){
                /* Move temporal key & offset */
                // Key
                fseeko(fp, default_offset + offset + 128 + 16*(i-1), SEEK_SET);
                fread(&temp_key, 8, 1, fp);
                fseeko(fp, default_offset + offset + 128 + 16*i, SEEK_SET);
                fwrite(&temp_key, 8, 1, fp);

                // Offset
                if(i - 1 == 0){
                    fseeko(fp, default_offset + offset + 120, SEEK_SET);
                    fread(&temp_offset, 8, 1, fp);
                }
                else{
                    fseeko(fp, default_offset + offset + 128 + 16*(i-2) + 8, SEEK_SET);
                    fread(&temp_offset, 8, 1, fp);
                }
                fseeko(fp, default_offset + offset + 128 + 16*(i-1) + 8, SEEK_SET);
                fwrite(&temp_offset, 8, 1, fp);
            }
            // Setting temp offset : Neighbor page
            fseeko(fp, default_offset + neighbor_offset + 128 + 16*(neighbor_num_keys - 1) + 8, SEEK_SET);
            fread(&temp_offset, 8, 1, fp);

            // Move temp offset to offset page.
            fseeko(fp, default_offset + offset + 120, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);

            // Setting parent
            fseeko(fp, default_offset + temp_offset, SEEK_SET);
            fwrite(&offset, 8, 1, fp);

            // Reinitialize : Neighbor page
            fseeko(fp, default_offset + neighbor_offset + 128 + 16*(neighbor_num_keys -1) + 8, SEEK_SET);
            fwrite(&zero_64, 8, 1, fp);

            // Setting key : Offset page
            fseeko(fp, default_offset + offset + 128, SEEK_SET);
            fwrite(&k_prime, 8, 1, fp);

            /* Setting parent's offset : Offset page's parent */
            // Setting parent offset
            fseeko(fp, default_offset + offset, SEEK_SET);
            fread(&p_offset, 8, 1, fp);

            // Read from neighbor page.
            fseeko(fp, default_offset + neighbor_offset + 128 + 16*(neighbor_num_keys-1), SEEK_SET);
            fread(&temp_key, 8, 1, fp);

            // Write to offset page's parent page.
            fseeko(fp, default_offset + p_offset + 128 + 16*(k_prime_index), SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
        }
        // Case : leaf page
        else{
            for(i = num_keys; i > 0; i--){
                /* Move temporal key & value */
                fseeko(fp, default_offset + offset + 128 + 128*(i-1), SEEK_SET);
                fread(&temp_key, 8, 1, fp);
                fread(temp_value, 1, 120, fp);

                fseeko(fp, default_offset + offset + 128 + 128*i, SEEK_SET);
                fwrite(&temp_key, 8, 1, fp);
                fwrite(temp_value, 1, 120, fp);
            }
            // Setting temp value from neighbor page.
            fseeko(fp, default_offset + neighbor_offset + 128 + 128*(neighbor_num_keys - 1) + 8, SEEK_SET);
            fread(temp_value, 1, 120, fp);
            // Move temp value to offset page.
            fseeko(fp, default_offset + offset + 128 + 8, SEEK_SET);
            fwrite(temp_value, 1, 120, fp);
            // Reinitialize neighbor page.
            fseeko(fp, default_offset + neighbor_offset + 128 + 128*(neighbor_num_keys - 1) + 8, SEEK_SET);
            fwrite(zero, 1, 120, fp);

            // Setting temp key from neighbor page.
            fseeko(fp, default_offset + neighbor_offset + 128 + 128*(neighbor_num_keys - 1), SEEK_SET);
            fread(&temp_key, 8, 1, fp);
            // Move temp key of offset page.
            fseeko(fp, default_offset + offset + 128, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);

            /* Parent page setting */
            // Setting parent offset.
            fseeko(fp, default_offset + offset, SEEK_SET);
            fread(&p_offset, 8, 1, fp);

            fseeko(fp, default_offset + p_offset + 128 + 128*k_prime_index, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
        }
    }

    /* Case : offset page is the leftmost child. */
    else{
        // Case : Leaf page
        if(is_leaf){
            // Move neighbor's key & value to offset page.
            fseeko(fp, default_offset + neighbor_offset + 128, SEEK_SET);
            fread(&temp_key, 8, 1, fp);
            fread(&temp_value, 1, 120, fp);

            fseeko(fp, default_offset + offset + 128 + 128*num_keys, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
            fwrite(&temp_value, 1, 120, fp);

            /* Parent page setting */
            // Setting parent offset.
            fseeko(fp, default_offset + offset, SEEK_SET);
            fread(&p_offset, 8, 1, fp);

            // Read from neighbor page. : Key
            fseeko(fp, default_offset + neighbor_offset + 128 + 128, SEEK_SET);
            fread(&temp_key, 8, 1, fp);

            fseeko(fp, default_offset + p_offset + 128 + 128*k_prime_index, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);

            for(i = 0; i < neighbor_num_keys - 1; i++){
                fseeko(fp, default_offset + neighbor_offset + 128 + 128*(i+1), SEEK_SET);
                fread(&temp_key, 8, 1, fp);
                fread(temp_value, 1, 120, fp);

                fseeko(fp, default_offset + neighbor_offset + 128 + 128*i, SEEK_SET);
                fwrite(&temp_key, 8, 1, fp);
                fwrite(temp_value, 1, 120, fp);
            }
        }
        // Case : Internal page
        else{
            // Write k_prime : Offset page
            fseeko(fp, default_offset + offset + 128 + 16*num_keys, SEEK_SET);
            fwrite(&k_prime, 8, 1, fp);

            // Move neighbor's offset to offset page.
            fseeko(fp, default_offset + neighbor_offset + 120, SEEK_SET);
            fread(&temp_offset, 8, 1, fp);

            fseeko(fp, default_offset + offset + 128 + 16*(num_keys) + 8, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);

            /* Setting parent */
            fseeko(fp, default_offset + temp_offset, SEEK_SET);
            fwrite(&offset, 8, 1, fp);

            // Setting parent offset.
            fseeko(fp, default_offset + offset, SEEK_SET);
            fread(&p_offset, 8, 1, fp);

            // Read from neighbor page : Key
            fseeko(fp, default_offset + neighbor_offset + 128, SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);
            // Write to offset page's parent page
            fseeko(fp, default_offset + p_offset + 128 + 16*(k_prime_index), SEEK_SET);
            fwrite(&temp_key, 8, 1, fp);

            for(i = 0; i < neighbor_num_keys - 1; i++){
                // Key
                fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i+1), SEEK_SET);
                fread(&temp_key, 8, 1, fp);

                fseeko(fp, default_offset + neighbor_offset + 128 + 16*i, SEEK_SET);
                fwrite(&temp_key, 8, 1, fp);

                // Offset
                if(i == 0){
                    fseeko(fp, default_offset + neighbor_offset + 128 + 8, SEEK_SET);
                    fread(&temp_offset, 8, 1, fp);

                    fseeko(fp, default_offset + neighbor_offset + 120, SEEK_SET);
                    fwrite(&temp_offset, 8, 1, fp);
                }
                else{
                    fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i) + 8, SEEK_SET);
                    fread(&temp_offset, 8, 1, fp);

                    fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i-1) + 8, SEEK_SET);
                    fwrite(&temp_offset, 8, 1, fp);
                }
            }
            // Move offset : neighbor page
            fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i) + 8, SEEK_SET);
            fread(&temp_offset, 8, 1, fp);

            fseeko(fp, default_offset + neighbor_offset + 128 + 16*(i-1) + 8, SEEK_SET);
            fwrite(&temp_offset, 8, 1, fp);
        }
    }
    num_keys++;
    neighbor_num_keys--;
    /* Modify pafe header. */
    // Offset page
    fseeko(fp, default_offset + offset + 12, SEEK_SET);
    fwrite(&num_keys, 4, 1, fp);
    // Neighbor page
    fseeko(fp, default_offset + neighbor_offset + 12, SEEK_SET);
    fwrite(&neighbor_num_keys, 4, 1, fp);

    return rp_offset;
}
int64_t delete_entry_from_page(int64_t rp_offset, int64_t offset, int64_t key, char *value, int64_t key_offset){
    int min_keys = 0, is_leaf = 0, num_keys = 0, neighbor_num_keys = 0, k_prime_index = 0, neighbor_index = 0, capacity = 0;
    int64_t k_prime, neighbor_offset, p_offset;

    // Remove key and page offset from page.
    offset = remove_entry_from_page(offset, key, value, key_offset);

    /* Case : deletion from the root. */
    if(offset == rp_offset)
        return adjust_root_page(rp_offset);

    /* Case : deletion from a node below the root. */

    // Determine minimum allowable size of page, to be perserved after deletion.
    // Setting is leaf : offset
    fseeko(fp, default_offset + offset + 8, SEEK_SET);
    fread(&is_leaf, 4, 1, fp);
    min_keys = is_leaf ? cut(leaf_order - 1) : cut(internal_order) - 1; 

    /* Case : Simple case. Paage stays at or above minimum. */
    // Setting number of keys : offset
    fseeko(fp, default_offset + offset + 12, SEEK_SET);
    fread(&num_keys, 4, 1, fp);

    if(num_keys >= min_keys)
        return rp_offset;

    /* Case : page falls below minimum. Either coalescence or redistribution is needed */

    /* Find the appropriate neighbor page and the key in the parnet */
    neighbor_index = get_neighbor_page_index(offset);
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

    /* Setting k_prime from parent page of given offset. */
    // Parent page : Always internal page.
    // Setting parent page offset.
    fseeko(fp, default_offset + offset, SEEK_SET);
    fread(&p_offset, 8, 1, fp);
    // Setting k_prime.
    fseeko(fp, default_offset + p_offset + 128 + 16*k_prime_index, SEEK_SET);
    fread(&k_prime, 8, 1, fp);

    /* Setting neighbor offset */ 
    if(neighbor_index == -1){
        fseeko(fp, default_offset + p_offset + 128 + 8, SEEK_SET);
        fread(&neighbor_offset, 8, 1, fp);
    }
    else{
        if(neighbor_index == 0){
            fseeko(fp, default_offset + p_offset + 120, SEEK_SET);
            fread(&neighbor_offset, 8, 1, fp);
        }
        else{
            fseeko(fp, default_offset + p_offset + 128 + 16*(neighbor_index-1) + 8, SEEK_SET);
            fread(&neighbor_offset, 8, 1, fp);
        }
    }
    // Setting number of keys : neighbor
    fseeko(fp, default_offset + neighbor_offset + 12, SEEK_SET);
    fread(&neighbor_num_keys, 4, 1, fp);

    // Setting capacity
    capacity = is_leaf ? leaf_order : internal_order - 1;

    /* Coalescence. */
    if(neighbor_num_keys + num_keys < capacity){
        return coalesce_pages(rp_offset, offset, neighbor_offset, neighbor_index, k_prime);
    }
    /* Redistribution. */
    else{
        return redistribute_pages(rp_offset, offset, neighbor_offset, neighbor_index, k_prime_index, k_prime);
    }
}
/*
 * Find the matching record and delete it if found.
 * If success, return 0. Otherwise, return non-zero value.
 */
int delete(int64_t key){
    char key_value[120];
    int64_t key_leaf_offset, rp_offset;

    // Setting root page offset. (From header page)
    fseeko(fp, default_offset + 8, SEEK_SET);
    fread(&rp_offset, 8, 1, fp);

    /* Fail */ 
    if(find(key) == NULL){
        return -1;
    }
    memmove(key_value, find(key), 120);
    key_leaf_offset = find_leaf_page(rp_offset, key);

    /* Success */
    if(key_value != NULL && key_leaf_offset != 0){
        delete_entry_from_page(rp_offset, key_leaf_offset, key, key_value, 0);
        // Synchronize
        fflush(fp);
        return 0;
    }
    /* Fail */
    return -1;
}
