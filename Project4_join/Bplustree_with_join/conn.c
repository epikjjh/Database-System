#include "bptree.h"

/* Open new connection
 */
int open_conn(conn *c, int buf_num){
	DEC_RET;
	RET(init_bufmgr(c, buf_num));
	return E_OK;
}

/* Close the connection
 */
int close_conn(conn *c){
	close_bufmgr(c);
	return E_OK;
}
