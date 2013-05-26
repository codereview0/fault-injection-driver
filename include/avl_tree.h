#ifndef __AVLTREE_H__
#define __AVLTREE_H__

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/slab.h>

#define _DEBUG_AVL

struct tree {
	int key;
	void *val;
	struct tree *tree_avl_left;
	struct tree *tree_avl_right;
	int tree_avl_height;
	struct tree *tree_next;
};

#ifdef DEBUG_AVL
static void printk_avl (struct tree * tree);
#endif


#ifndef avl_br_empty
#define avl_br_empty	(struct tree *) NULL
#endif

/* Since the trees are balanced, their height will never be large. */
#define avl_maxheight	127
#define heightof(tree)	((tree) == avl_br_empty ? 0 : (tree)->tree_avl_height)

int graid_avl_init(struct tree **head);
struct tree *graid_avl_find_next(struct tree *head, int key);
struct tree *graid_avl_find_previous(struct tree *head, int key);
struct tree *graid_avl_find(struct tree *head, int key);
int graid_avl_insert (struct tree **head, struct tree * new_node);
int graid_avl_remove (struct tree ** head, struct tree * node_to_delete);
void graid_printk_avl (struct tree * tree);

#endif

