#include "bptree.h"

/* Pop buffer page from lru list
 */
void pop_from_lru(table *t, page *p){
	bufmgr *bfm = t->c->bfm;
	if (p == bfm->lru_head || p == bfm->lru_tail){
		if (p == bfm->lru_head){
			bfm->lru_head = p->lru_next;
			if (p->lru_next)
				p->lru_next->lru_prev = NULL;
		}
		if (p == bfm->lru_tail){
			bfm->lru_tail = p->lru_prev;
			if (p->lru_prev)
				p->lru_prev->lru_next = NULL;
		}
	}
	else{
		p->lru_prev->lru_next = p->lru_next;
		p->lru_next->lru_prev = p->lru_prev;
	}

	p->lru_prev = p->lru_next = NULL;
}

/* Push buffer page to lru list
 */
void push_to_lru(table *t, page *p){
	bufmgr *bfm = t->c->bfm;
	p->lru_next = bfm->lru_head;
	if (p->lru_next)
		p->lru_next->lru_prev = p;
	p->lru_prev = NULL;
	bfm->lru_head = p;
	if (bfm->lru_tail == NULL){
		bfm->lru_tail = p;
	}
}

/* Evict a page from buffer pool and flush it if needed
 */
page *evict_page(table *t){
	conn *c = t->c;
	bufmgr *bfm = c->bfm;
	page *p = bfm->lru_tail;
	while(p != NULL){
		if (p->pincnt != 0){
			p = p->lru_prev;
			continue;
		}
		pop_from_lru(t, p);
		if (p->is_dirty)
			write_page(&c->tbls[p->table_id], p);
		p->lru_next = p->lru_prev = NULL;
		return p;
	}
	panic("evict_page"); 
	exit(1);
}

/* Allocate an empty page from buffer pool
 */
page *alloc_page(table *t, addr ad){
	bufmgr *bfm = t->c->bfm;
	page *freepage = NULL;
	int i;
	for (i = 0; i < bfm->num_buf; i++){
		if (!bfm->pages[i].is_used){
			freepage = &bfm->pages[i];
			break;
		}
	}
	if (freepage == NULL)
		freepage = evict_page(t);

	freepage->pincnt++;
	freepage->table_id = t->table_id;
	freepage->offset = ad;
	freepage->is_dirty = false;
	freepage->is_used = true;
	push_to_lru(t, freepage);
	bfm->tot_pincnt++;
	return freepage;
}

/* Get an address-specific page from buffer pool.
 * if it doesn't exist, Load it from disk to buffer. 
 */
page *get_page(table *t, addr ad){
	bufmgr *bfm = t->c->bfm;
	page *freepage = NULL;
	int i;
	for (i = 0; i < bfm->num_buf; i++){
		if (bfm->pages[i].table_id == t->table_id &&
				bfm->pages[i].offset == ad && bfm->pages[i].is_used){
			bfm->pages[i].pincnt++;
			bfm->tot_pincnt++;
			update_lru(t, &bfm->pages[i]);
			return &bfm->pages[i];
		}
	}

	freepage = alloc_page(t, ad);
	read_page(t, freepage);
	return freepage;
}

/* Flush all buffer pages which belong to the table.
 */
void flush_page(table *t){
	bufmgr *bfm = t->c->bfm;
	page *p = bfm->lru_head;
	page *next;
	while(p != NULL){
		next = p->lru_next;
		if (p->table_id == t->table_id){
			if (p->is_dirty)
				write_page(t, p);
			pop_from_lru(t, p);
		}
		p = next;
	}
}

/* Intialize buffer manager
 */
int init_bufmgr(conn *c, int buf_num){
	int64_t size = buf_num * (sizeof(page) + BLOCK_SIZE) + BLOCK_SIZE;
	int i;
	void *p;
	bufmgr *bfm = (bufmgr*)malloc(sizeof(bufmgr));
	p = malloc(size);
	memset(p, 0, size);
	bfm->pages = (page*)p;
	p += buf_num * sizeof(page);
	p = (void*)ALIGN_UP((uintptr_t)p, BLOCK_SIZE);
	for (i = 0; i < buf_num; i++, p += BLOCK_SIZE){
		bfm->pages[i].b = p;
	}
	bfm->num_buf = buf_num;
	bfm->lru_head = bfm->lru_tail = NULL;
	bfm->tot_pincnt = 0;
	c->bfm = bfm;
	return E_OK;
}

/* Free all resources which belong to the buffer manager
 */
void close_bufmgr(conn *c){
	bufmgr *bfm = c->bfm;
	page *p = bfm->lru_head;
	while(p != NULL){
		if (p->is_dirty)
			write_page(&c->tbls[p->table_id], p);
		p = p->lru_next;
	}
	free(bfm->pages);
	free(bfm);
	c->bfm = NULL;
}
