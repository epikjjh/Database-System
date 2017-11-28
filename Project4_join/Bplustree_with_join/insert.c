#include "bptree.h"

/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
npage *make_node(table *t){
	npage *np;
	nblock *nb;

	np = get_npage(t, alloc_block(t));
	nb = B(np);

	nb->is_leaf = false;
	nb->num_keys = 0;
	nb->parent = ADDR_NOT_EXIST;
	nb->i_leftmost = ADDR_NOT_EXIST;
	return np;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
npage *make_leaf(table *t){
	npage *np;
	nblock *nb;

	np = make_node(t);
	nb = B(np);

	nb->is_leaf = true;
	nb->l_sib = ADDR_NOT_EXIST;

	return np;
}

/* Set the page as root page
 */
void set_root(table *t, npage *np){
	hpage *hp = get_hpage(t);
	hblock *hb = B(hp);

	set_dirty(hp);
	if (np == NULL){
		hb->root = ADDR_NOT_EXIST;
	}
	else{
		hb->root = np->offset;
	}
	release_page(t, hp);
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 */
int insert_into_leaf(table *t, npage *leaf, const record *r){
	nblock *nb = B(leaf);
	int i, j;

	for (i = 0; i < nb->num_keys; i++){
		if (r->k < nb->l_recs[i].k) break;
	}
	nb->num_keys++;
	for (j = nb->num_keys-1; j > i; j--){
		nb->l_recs[j] = nb->l_recs[j-1];
	}
	nb->l_recs[i] = *r;
	return E_OK;
}

/* First insertion:
 * start a new tree.
 */
int start_new_tree(table *t, record *r) {
	hpage *hp = get_hpage(t);
	hblock *hb = B(hp);
	npage *root;
	set_dirty(hp);
	root = make_leaf(t);
	set_dirty(root);
	insert_into_leaf(t, root, r);
	B(root)->parent = ADDR_NOT_EXIST;
	hb->root = root->offset;

	release_page(t, hp);
	release_page(t, root);
	return E_OK;
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
int insert_into_new_root(table *t, npage *left, int64_t k, npage *right) {
	npage *root = make_node(t);
	nblock *nb = B(root);
	set_dirty(root);
	nb->i_children[0].k = k;
	nb->i_leftmost = left->offset;
	nb->i_children[0].v = right->offset;
	nb->num_keys++;
	nb->parent = ADDR_NOT_EXIST;
	B(left)->parent = root->offset;
	B(right)->parent = root->offset;

	set_root(t, root);

	release_page(t, root);
	return E_OK;
}

/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(npage *parent, npage *left) {
	int left_index = 0;
	if (B(parent)->i_leftmost == left->offset)
		return 0;

	while (left_index < B(parent)->num_keys && 
			B(parent)->i_children[left_index].v != left->offset)
		left_index++;

	if (left_index >= B(parent)->num_keys)
		panic("get_left_index"); 

	return left_index+1;
}

/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
int insert_into_node(table *t, npage *np, 
		int left_index, int64_t k, npage *right) {
	nblock *nb = B(np);
	int i;

	for (i = nb->num_keys; i > left_index; i--) {
		nb->i_children[i] = nb->i_children[i-1];
	}
	nb->i_children[left_index].k = k;
	nb->i_children[left_index].v = right->offset;
	nb->num_keys++;
	B(right)->parent = np->offset;

	return E_OK;
}

/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
int insert_into_node_after_splitting(table *t, npage *np, int left_index, 
		int64_t k, npage *right) {
	DEC_RET;
	nblock *nb = B(np);
	npage *new_np;
	nblock *new_nb;
	child temp_children[NUM_INT_KEY+1];
	npage *temp_np[NUM_INT_KEY+1];
	int insertion_index, split, i;
	int64_t new_key;

	new_np = make_node(t);
	new_nb = B(new_np);
	set_dirty(new_np);

	insertion_index = 0;
	split = cut(INT_ORDER);

	//Put all key-link pairs in temporary array including new key-link pair
	while (insertion_index < nb->num_keys && nb->i_children[insertion_index].k < k){
		temp_children[insertion_index] = nb->i_children[insertion_index];
		temp_np[insertion_index] = get_child(t, np, insertion_index+1);
		set_dirty(temp_np[insertion_index]);
		insertion_index++;
	}
	temp_children[insertion_index].k = k;
	temp_children[insertion_index].v = right->offset;
	temp_np[insertion_index] = right;

	for (i = insertion_index+1; i < INT_ORDER; i++){
		temp_children[i] = nb->i_children[i-1];
		temp_np[i] = get_child(t, np, i);
		set_dirty(temp_np[i]);
	}

	new_key = temp_children[split-1].k;
	new_nb->i_leftmost = temp_children[split-1].v;

	B(temp_np[split-1])->parent = new_np->offset;
	if (temp_np[split-1] != right){
		release_page(t, temp_np[split-1]);
	}

	for (i = split; i <= nb->num_keys; i++){
		new_nb->i_children[i - split] = temp_children[i];
		B(temp_np[i])->parent = new_np->offset;
		if (temp_np[i] != right){
			release_page(t, temp_np[i]);
		}
		memset(&nb->i_children[i-1], 0, sizeof(child));
	}
	new_nb->num_keys = i - split;

	for (i = 0; i < split-1; i++){
		nb->i_children[i] = temp_children[i];
		B(temp_np[i])->parent = np->offset;
		if (temp_np[i] != right){
			release_page(t, temp_np[i]);
		}
	}
	nb->num_keys = i;
	new_nb->parent = nb->parent;

	ret = insert_into_parent(t, np, new_key, new_np);
	release_page(t, new_np);
	return ret;
}

/* Inserts a new node (leaf or internal node) into the B+ tree.
 */
int insert_into_parent(table *t, npage *left, int64_t k, npage *right) {
	DEC_RET;
	int left_index;
	npage *parent;

	parent = get_parent(t, left);

	/* Case: new root. */

	if (parent == NULL)
		return insert_into_new_root(t, left, k, right);
	set_dirty(parent);

	/* Case: leaf or node. (Remainder of
	 * function body.)  
	 */

	/* Find the parent's pointer to the left 
	 * node.
	 */

	left_index = get_left_index(parent, left);

	/* Simple case: the new key fits into the node. 
	 */

	if (B(parent)->num_keys < INT_ORDER- 1){
		ret = insert_into_node(t, parent, left_index, k, right);
		release_page(t, parent);
		return ret;
	}

	/* Harder case:  split a node in order 
	 * to preserve the B+ tree properties.
	 */

	ret = insert_into_node_after_splitting(t, parent, left_index, k, right);
	release_page(t, parent);
	return ret;
}

/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
int insert_into_leaf_after_splitting(table *t, npage *np, record *r) {
	DEC_RET;
	nblock *nb = B(np);
	npage *new_np;
	nblock *new_nb;
	record *temp_recs[NUM_LEAF_REC+1];
	int insertion_index, split, i;
	int64_t new_key;

	new_np = make_leaf(t);
	new_nb = B(new_np);
	set_dirty(new_np);

	insertion_index = 0;
	split = cut(LEAF_ORDER-1);

	while (insertion_index < nb->num_keys && nb->l_recs[insertion_index].k < r->k){
		temp_recs[insertion_index] = &nb->l_recs[insertion_index];
		insertion_index++;
	}
	temp_recs[insertion_index] = r;

	for (i = insertion_index+1; i < LEAF_ORDER; i++){
		temp_recs[i] = &nb->l_recs[i-1];
	}

	new_key = temp_recs[split]->k;
	new_nb->l_sib = nb->l_sib;
	nb->l_sib = new_np->offset;

	for (i = split; i <= nb->num_keys; i++){
		new_nb->l_recs[i - split] = *temp_recs[i];
	}

	new_nb->num_keys = i - split;

	for (i = split-1; i >= 0; i--){
		if (temp_recs[i] != &nb->l_recs[i])
			nb->l_recs[i] = *temp_recs[i];
	}

	for (i = split; i < nb->num_keys; i++)
		memset(&nb->l_recs[i], 0, sizeof(child));

	nb->num_keys = split;
	new_nb->parent = nb->parent;

	ret = insert_into_parent(t, np, new_key, new_np);
	release_page(t, new_np);
	return ret;
}

/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int insert_low( table *t, record *r) {
	DEC_RET;
	hpage *hp;
	record dup;
	npage *leaf;

	/* The current implementation ignores
	 * duplicates.
	 */

	if (find_low(t, r->k, &dup) == E_OK)
		return E_DUP;

	/* Case: the tree does not exist yet.
	 * Start a new tree.
	 */

	hp = get_hpage(t);
	if (B(hp)->root == ADDR_NOT_EXIST){
		release_page(t, hp);
		return start_new_tree(t, r);
	}
	release_page(t, hp);


	/* Case: the tree already exists.
	 * (Rest of function body.)
	 */

	if ((leaf = find_leaf(t, r->k)) == NULL)
		panic("insert"); 

	set_dirty(leaf);

	/* Case: leaf has room for key and pointer.
	 */

	if (B(leaf)->num_keys < LEAF_ORDER - 1) {
		insert_into_leaf(t, leaf, r);
		release_page(t, leaf);
		return E_OK;
	}

	/* Case:  leaf must be split.
	 */

	ret = insert_into_leaf_after_splitting(t, leaf, r);
	release_page(t, leaf);

	return ret;
}
