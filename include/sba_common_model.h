#ifndef __INCLUDE_SBA_COMMON_MODEL_H__
#define __INCLUDE_SBA_COMMON_MODEL_H__

#include "ht_at_wrappers.h"

/* these are the inputs that move the system from one state to another */
typedef struct _sba_state_input {
	int block_type;		//what type of block is this;
	int response;		//is this request a failure or success;
} sba_state_input;


/* this is the representation of state in model */
typedef struct _sba_state {
	char name[16];				//name of the state
	int outdegree;	 			//how many edges go out from this state
	hash_table *h_out_edges;	//list of edges that go out
} sba_state;


/* edges are represented as follows */
typedef struct _sba_edge {
	sba_state_input input;		//what is the input on this edge ?
	sba_state *dest;			//where is the destination ?
} sba_edge;


typedef struct _journaling_model {
	int mode;
	int total_states;
	sba_state **states;
	sba_state *current_state;
} journaling_model;

#endif
