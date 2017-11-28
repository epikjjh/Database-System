#include "bptree.h"

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
npage *find_leaf(table *t, const int64_t k) {
	hpage *hp = get_hpage(t);
	npage *np;
	npage *next_np;
	nblock *nb;
	int i = 0;

	if (B(hp)->root == ADDR_NOT_EXIST) {
		release_page(t, hp);
		return NULL;
	}

	release_page(t, hp);
	np = get_root(t);
	nb = B(np);
	while (!nb->is_leaf) {
		i = 0;
		while (i < nb->num_keys) {
			if (k >= nb->i_children[i].k) i++;
			else break;
		}
		next_np = get_child(t, np, i);
		release_page(t, np);
		np = next_np;
		nb = B(np);
	}
	return np;
}

int find_rec(table *t, npage *np, const int64_t k){
	nblock *nb = B(np);
	int i;
	for (i = 0; i < nb->num_keys; i++){
		if (nb->l_recs[i].k == k) return i;
	}
	return -1;
}

/* Finds and returns the record to which
 * a key refers.
 */
int find_low(table *t, const int64_t k, record *r){
	hpage *hp = get_hpage(t);
	hblock *hb = B(hp);
	npage *np;
	nblock *nb;
	int idx;

	if (hb->root == ADDR_NOT_EXIST){
		release_page(t, hp);
		return E_NOT_FOUND;
	}
	release_page(t, hp);
	if ((np = find_leaf(t, k)) == NULL)
		panic("find"); 

	nb = B(np);
	
	idx = find_rec(t, np, k);
	if (idx == -1){
		release_page(t, np);
		return E_NOT_FOUND;
	}
	memcpy(r, &nb->l_recs[idx], sizeof(record));
	release_page(t, np);
	return E_OK;
}

/* Prints the B+ tree in the command
 * line in level (rank) order, with the 
 * keys in each node and the '|' symbol
 * to separate nodes.
 */
void print_tree(table *t) {
	hpage *hp;
	npage *np = NULL;
	int i = 0;
	npage **queue;
	int *depth;
	int last_depth = 0;
	int head = 0, tail = 0;

	if ((np = get_root(t)) == NULL) {
		printf("Empty tree.\n");
		return;
	}
	hp = get_hpage(t);
	queue = (npage**) malloc(B(hp)->num_page * sizeof(npage*));
	depth = (int*) malloc(B(hp)->num_page * sizeof(int));
	release_page(t, hp);
	queue[tail] = np;
	depth[tail] = 0;
	tail++;
	while( head != tail){
		if (last_depth != depth[head]){
			last_depth = depth[head];
			printf("\n");
		}
		np = queue[head++];
		if (B(np)->is_leaf){
			for (i = 0; i < B(np)->num_keys; i++)
				printf("%ld ",B(np)->l_recs[i].k);
		}
		else{
			queue[tail] = get_child(t, np, 0);
			if (B(queue[tail])->parent != np->offset)
				panic("print_tree"); 

			depth[tail++] = last_depth + 1;
			for (i = 0; i < B(np)->num_keys; i++){
				printf("%ld ",B(np)->i_children[i].k);
#ifdef DEBUG_TREE
				fflush(stdout);
#endif
				queue[tail] = get_child(t, np, i+1);
				if (B(queue[tail])->parent != np->offset)
					panic("print_tree"); 

				depth[tail++] = last_depth + 1;
			}
		}
		printf("|  ");
		release_page(t, np);
	}
	printf("\n");
	free(queue);
	free(depth);
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

/* Get the root page of the table
 */
npage *get_root(table *t){
	hpage *hp = get_hpage(t);
	hblock *hb = B(hp);
	npage *np;

	if (hb->root == ADDR_NOT_EXIST){
		release_page(t, hp);
		return NULL;
	}

	np = get_npage(t, hb->root);
	release_page(t, hp);
	return np;
}

/* Get a child page from the parent page
 */
npage *get_child(table *t, npage *np, int idx){
	nblock *nb = B(np);
	if (idx == 0){
		if (nb->i_leftmost == ADDR_NOT_EXIST)
			return NULL;

		return get_npage(t, nb->i_leftmost);
	}
	else{
		idx--;
		if (nb->i_children[idx].v == ADDR_NOT_EXIST)
			return NULL;

		return get_npage(t, nb->i_children[idx].v);
	}
}

/* Get the parent page from the child page
 */
npage *get_parent(table *t, npage *np){
	nblock *nb = B(np);

	if (nb->parent == ADDR_NOT_EXIST)
		return NULL;

	return get_npage(t, nb->parent);
}

/* Get the internal / leaf page by address
 */
npage *get_npage(table *t, addr ad){
	npage *np = (npage *)get_page(t, ad);
	return np;
}

/* Get the header page
 */
hpage *get_hpage(table *t){
	hpage *hp = (hpage *)get_page(t, HPAGE_NUM);
	return hp;
}

/* Get the free page
 */
fpage *get_fpage(table *t, addr ad){
	fpage *fp = (fpage *)get_page(t, ad);
	return fp;
}

/* Allocate new header page when a table is created
 */
hpage *alloc_hpage(table *t){
	hpage *hp = (hpage *)alloc_page(t, HPAGE_NUM);
	return hp;
}

/* Print fatal error and terminate the program
 */
void panic(const char *str){
	fprintf(stderr, "%s panic!\n", str);
	exit(EXIT_FAILURE);
}
