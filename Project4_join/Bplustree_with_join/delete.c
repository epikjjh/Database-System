#include "bptree.h"

int remove_entry_from_node(table *t, npage *np, int64_t k, int idx){
	nblock *nb = B(np);
	int i;

	// Remove the key and shift other keys accordingly.
	i = 0;
	while (k != (nb->is_leaf ? nb->l_recs[i].k : nb->i_children[i].k))
		i++;

	if (!nb->is_leaf && idx == 0){
		nb->i_leftmost = nb->i_children[0].v;
	}
	for (++i; i < nb->num_keys; i++){
		if (nb->is_leaf)
			nb->l_recs[i-1] = nb->l_recs[i];
		else{
			nb->i_children[i-1] = nb->i_children[i];
		}
	}
	if (nb->is_leaf)
		memset(&nb->l_recs[i-1], 0, sizeof(record));
	else
		memset(&nb->i_children[i-1], 0, sizeof(child));

	// One key fewer.
	nb->num_keys--;

	return E_OK;
}

int adjust_root(table *t, npage *root) {
	npage *new_root;

	/* Case: nonempty root.
	 * Key and pointer have already been deleted,
	 * so nothing to be done.
	 */
	if (root == NULL){
		panic("adjust_root panic\n"); 
		exit(1);
	}

	if (B(root)->num_keys > 0)
		return E_OK;

	/* Case: empty root. 
	 */

	// If it has a child, promote 
	// the first (only) child
	// as the new root.

	if (!B(root)->is_leaf) {
		new_root = get_child(t, root, 0);
		set_dirty(new_root);
		B(new_root)->parent = ADDR_NOT_EXIST;
		set_root(t, new_root);
		release_page(t, new_root);
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else{
		set_root(t, NULL);
	}

	free_block(t, root);

	return E_OK;
}

/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
int coalesce_nodes(table *t, npage *np, npage *neighbor, int neighbor_index, int k_prime) {
	int i, j, neighbor_insertion_index, n_end;
	nblock *nb;
	npage *tmp, *parent;

	/* Swap neighbor with node if node is on the
	 * extreme left and neighbor is to its right.
	 */

	if (neighbor_index == -1) {
		tmp = np;
		np = neighbor;
		neighbor = tmp;
	}
	nb = B(np);

	/* Starting point in the neighbor for copying
	 * keys and pointers from n.
	 * Recall that n and neighbor have swapped places
	 * in the special case of n being a leftmost child.
	 */

	neighbor_insertion_index = B(neighbor)->num_keys;

	/* Case:  nonleaf node.
	 * Append k_prime and the following pointer.
	 * Append all pointers and keys from the neighbor.
	 */

	if (!nb->is_leaf) {

		/* Append k_prime.
		 */

		B(neighbor)->i_children[neighbor_insertion_index].k = k_prime;
		B(neighbor)->i_children[neighbor_insertion_index].v = nb->i_leftmost;
		B(neighbor)->num_keys++;

		n_end = nb->num_keys;

		for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
			B(neighbor)->i_children[i] = nb->i_children[j];
			B(neighbor)->num_keys++;
			nb->num_keys--;
		}

		/* All children must now point up to the same parent.
		 */

		for (i = 0; i < B(neighbor)->num_keys + 1; i++) {
			tmp = get_child(t, neighbor, i);
			set_dirty(tmp);
			B(tmp)->parent = neighbor->offset;
			release_page(t, tmp);
		}
	}

	/* In a leaf, append the keys and pointers of
	 * n to the neighbor.
	 * Set the neighbor's last pointer to point to
	 * what had been n's right neighbor.
	 */

	else {
		for (i = neighbor_insertion_index, j = 0; j < nb->num_keys; i++, j++) {
			B(neighbor)->l_recs[i] = nb->l_recs[j];
			B(neighbor)->num_keys++;
		}
		B(neighbor)->l_sib = nb->l_sib;
	}

	parent = get_parent(t, np);
	set_dirty(parent);
	free_block(t, np);
	delete_entry(t, parent, k_prime, neighbor_index == -1 ? 1 : neighbor_index+1);
	release_page(t, parent);
	return E_OK;
}

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index(table *t, npage *np, npage *parent) {
	int i;

	/* Return the index of the key to the left
	 * of the pointer in the parent pointing
	 * to n.  
	 * If n is the leftmost child, this means
	 * return -1.
	 */
	if (B(parent)->i_leftmost == np->offset)
		return -1;
	for (i = 0; i < B(parent)->num_keys; i++)
		if (B(parent)->i_children[i].v == np->offset)
			return i;

	// Error state.
	panic("get_neighbor_index");
}

/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
int redistribute_nodes(table *t, npage *np, npage *neighbor, int neighbor_index, 
		int k_prime_index, int64_t k_prime) {  
	int i;
	nblock *nb = B(np);
	npage *parent;
	npage *tmp;

	/* Case: n has a neighbor to the left. 
	 * Pull the neighbor's last key-pointer pair over
	 * from the neighbor's right end to n's left end.
	 */
	parent = get_parent(t, np);
	set_dirty(parent);

	if (neighbor_index != -1) {
		for (i = nb->num_keys; i > 0; i--) {
			if (nb->is_leaf)
				nb->l_recs[i] = nb->l_recs[i - 1];
			else
				nb->i_children[i] = nb->i_children[i - 1];
		}
		if (!nb->is_leaf) {
			nb->i_children[0].v = nb->i_leftmost;
			nb->i_leftmost = B(neighbor)->i_children[B(neighbor)->num_keys-1].v;
			tmp = get_child(t, np, 0);
			set_dirty(tmp);
			B(tmp)->parent = np->offset;
			release_page(t, tmp);
			nb->i_children[0].k = k_prime;
			B(parent)->i_children[k_prime_index].k = 
				B(neighbor)->i_children[B(neighbor)->num_keys - 1].k;

			memset(&B(neighbor)->i_children[B(neighbor)->num_keys], 0, sizeof(child));
		}
		else {
			nb->l_recs[0] = B(neighbor)->l_recs[B(neighbor)->num_keys - 1];
			memset(&B(neighbor)->l_recs[B(neighbor)->num_keys - 1], 0, sizeof(record));
			B(parent)->i_children[k_prime_index].k = nb->l_recs[0].k;
		}
	}

	/* Case: n is the leftmost child.
	 * Take a key-pointer pair from the neighbor to the right.
	 * Move the neighbor's leftmost key-pointer pair
	 * to n's rightmost position.
	 */

	else {  
		if (nb->is_leaf) {
			nb->l_recs[nb->num_keys] = B(neighbor)->l_recs[0];
			B(parent)->i_children[k_prime_index].k = B(neighbor)->l_recs[1].k;
		}
		else {
			nb->i_children[nb->num_keys].k = k_prime;
			nb->i_children[nb->num_keys].v = B(neighbor)->i_leftmost;
			tmp = get_child(t, np, nb->num_keys + 1);
			set_dirty(tmp);
			B(tmp)->parent = np->offset;
			release_page(t, tmp);
			B(parent)->i_children[k_prime_index].k = B(neighbor)->i_children[0].k;

			B(neighbor)->i_leftmost = B(neighbor)->i_children[0].v;
		}
		for (i = 0; i < B(neighbor)->num_keys - 1; i++) {
			if (B(neighbor)->is_leaf)
				B(neighbor)->l_recs[i] = B(neighbor)->l_recs[i + 1];
			else
				B(neighbor)->i_children[i] = B(neighbor)->i_children[i + 1];
		}
	}

	/* n now has one more key and one more pointer;
	 * the neighbor has one fewer of each.
	 */

	nb->num_keys++;
	B(neighbor)->num_keys--;

	release_page(t, parent);

	return E_OK;
}

/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
int delete_entry(table *t, npage *np, int64_t k, int idx){
	DEC_RET;
	npage *parent;
	nblock *nb = B(np);
	int min_keys;
	npage *neighbor;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;

	// Remove key and pointer from node.

	RET(remove_entry_from_node(t, np, k, idx));

	/* Case:  deletion from the root. 
	 */

	if (nb->parent == ADDR_NOT_EXIST) 
		return adjust_root(t, np);


	/* Case:  deletion from a node below the root.
	 * (Rest of function body.)
	 */

	/* Determine minimum allowable size of node,
	 * to be preserved after deletion.
	 */

	min_keys = nb->is_leaf ? cut(LEAF_ORDER - 1) : cut(INT_ORDER) - 1;

	/* Case:  node stays at or above minimum.
	 * (The simple case.)
	 */

	if (nb->num_keys >= min_keys)
		return E_OK;

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

	parent = get_parent(t, np);
	neighbor_index = get_neighbor_index(t, np, parent);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	k_prime = B(parent)->i_children[k_prime_index].k;
	neighbor = (neighbor_index == -1) ? get_child(t, parent, 1) : 
		get_child(t, parent, neighbor_index);
	set_dirty(neighbor);

	release_page(t, parent);
	capacity = nb->is_leaf ? LEAF_ORDER : INT_ORDER - 1;

	/* Coalescence. */

	if (B(neighbor)->num_keys + nb->num_keys < capacity){
		ret = coalesce_nodes(t, np, neighbor, neighbor_index, k_prime);
	}

	/* Redistribution. */

	else
		ret = redistribute_nodes(t, np, neighbor, neighbor_index, k_prime_index, k_prime);

	release_page(t, neighbor);
	return ret;
}

/* Master internal deletion function.
 */
int delete_low(table *t, int64_t k) {
	DEC_RET;
	npage *key_leaf;
	record r;
	int idx;

	RET(find_low(t, k, &r));

	key_leaf = find_leaf(t, k);
	if (key_leaf != NULL) {
		set_dirty(key_leaf);
		idx = find_rec(t, key_leaf, k);
		delete_entry(t, key_leaf, k, idx);
		release_page(t, key_leaf);
		return E_OK;
	}
	return E_OK;
}
