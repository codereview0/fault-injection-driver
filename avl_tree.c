/*
 *	This code is derived from the avl functions in mmap.c
 */

#include "avl_tree.h"


int graid_avl_init(struct tree **head)
{
	if (!head) {
		printk("\ngraid_avl_init: head NULL!");
		return -1;
	}

	*head = kmalloc(sizeof(struct tree), GFP_ATOMIC);
	(*head)->tree_avl_height = 0;
	(*head)->tree_avl_left = (struct tree *)0;
	(*head)->tree_avl_right = (struct tree *)0;
	(*head)->key = -1;
	return(0);
}

struct tree *graid_avl_find_next(struct tree *head, int key)
{
	struct tree * tmp, *tmp1;
	struct tree * known_highest = NULL;

	//vp - FIXME
	if (!head) {
		return known_highest;
	}	
	
	for (tmp = head ; ; ) {
		tmp1 = tmp;

		if (tmp -> key > key) {
			known_highest = tmp;

			if  (tmp->tree_avl_left) {
				tmp = tmp->tree_avl_left;
				continue;
			}
			else
				return known_highest;
		}

		if (tmp-> key <= key ) {
			if (tmp->tree_avl_right) {
				tmp = tmp->tree_avl_right;
				continue;
			}
			else
				return known_highest;
		}
	}
}


struct tree *graid_avl_find_previous(struct tree *head, int key)
{
	struct tree * tmp, *tmp1;
	struct tree * known_lowest = NULL;

	for (tmp = head ; ; ) {
		tmp1 = tmp;

		if (tmp -> key >= key) {
			if  (tmp->tree_avl_left) {
				tmp = tmp->tree_avl_left;
				continue;
			}
			else
				return known_lowest;
		}

		if (tmp-> key < key ) {
			known_lowest = tmp;
			if (tmp->tree_avl_right) {
				tmp = tmp->tree_avl_right;
				continue;
			}
			else
				return known_lowest;
		}
	}
}


struct tree *graid_avl_find(struct tree *head, int key)
{
	struct tree * result = NULL;
	struct tree * tree;

	for (tree = head ; ; ) {
		if (tree == avl_br_empty) {
			return result;
		}

		if (tree->key == key) {
			return tree;
		}
		if (tree->key > key) {
			tree = tree->tree_avl_left;
		} else {
			tree = tree->tree_avl_right;
		}
	}
}


/*
 * Rebalance a tree.
 * After inserting or deleting a node of a tree we have a sequence of subtrees
 * nodes[0]..nodes[k-1] such that
 * nodes[0] is the root and nodes[i+1] = nodes[i]->{tree_avl_left|tree_avl_right}.
 */
static void graid_avl_rebalance (struct tree *** nodeplaces_ptr, int count)
{
	for ( ; count > 0 ; count--) {
		struct tree ** nodeplace = *--nodeplaces_ptr;
		struct tree * node = *nodeplace;
		struct tree * nodeleft = node->tree_avl_left;
		struct tree * noderight = node->tree_avl_right;
		int heightleft = heightof(nodeleft);
		int heightright = heightof(noderight);
		if (heightright + 1 < heightleft) {
			/*                                                      */
			/*                            *                         */
			/*                          /   \                       */
			/*                       n+2      n                     */
			/*                                                      */
			struct tree * nodeleftleft = nodeleft->tree_avl_left;
			struct tree * nodeleftright = nodeleft->tree_avl_right;
			int heightleftright = heightof(nodeleftright);
			if (heightof(nodeleftleft) >= heightleftright) {
				/*                                                        */
				/*                *                    n+2|n+3            */
				/*              /   \                  /    \             */
				/*           n+2      n      -->      /   n+1|n+2         */
				/*           / \                      |    /    \         */
				/*         n+1 n|n+1                 n+1  n|n+1  n        */
				/*                                                        */
				node->tree_avl_left = nodeleftright;
				nodeleft->tree_avl_right = node;
				nodeleft->tree_avl_height = 1 + (node->tree_avl_height = 1 + heightleftright);
				*nodeplace = nodeleft;
			} else {
				/*                                                        */
				/*                *                     n+2               */
				/*              /   \                 /     \             */
				/*           n+2      n      -->    n+1     n+1           */
				/*           / \                    / \     / \           */
				/*          n  n+1                 n   L   R   n          */
				/*             / \                                        */
				/*            L   R                                       */
				/*                                                        */
				nodeleft->tree_avl_right = nodeleftright->tree_avl_left;
				node->tree_avl_left = nodeleftright->tree_avl_right;
				nodeleftright->tree_avl_left = nodeleft;
				nodeleftright->tree_avl_right = node;
				nodeleft->tree_avl_height = node->tree_avl_height = heightleftright;
				nodeleftright->tree_avl_height = heightleft;
				*nodeplace = nodeleftright;
			}
		} else if (heightleft + 1 < heightright) {
			/* similar to the above, just interchange 'left' <--> 'right' */
			struct tree * noderightright = noderight->tree_avl_right;
			struct tree * noderightleft = noderight->tree_avl_left;
			int heightrightleft = heightof(noderightleft);
			if (heightof(noderightright) >= heightrightleft) {
				node->tree_avl_right = noderightleft;
				noderight->tree_avl_left = node;
				noderight->tree_avl_height = 1 + (node->tree_avl_height = 1 + heightrightleft);
				*nodeplace = noderight;
			} else {
				noderight->tree_avl_left = noderightleft->tree_avl_right;
				node->tree_avl_right = noderightleft->tree_avl_left;
				noderightleft->tree_avl_right = noderight;
				noderightleft->tree_avl_left = node;
				noderight->tree_avl_height = node->tree_avl_height = heightrightleft;
				noderightleft->tree_avl_height = heightright;
				*nodeplace = noderightleft;
			}
		} else {
			int height = (heightleft<heightright ? heightright : heightleft) + 1;
			if (height == node->tree_avl_height)
				break;
			node->tree_avl_height = height;
		}
	}
}

/* Insert a node into a tree.
 * Performance improvement:
 *	 call addr_cmp() only once per node and use result in a switch.
 * Return old node address if we knew that MAC address already
 * Return NULL if we insert the new node
 */
int graid_avl_insert (struct tree **head, struct tree * new_node)
{
	struct tree ** nodeplace = head;
	struct tree ** stack[avl_maxheight];
	int stack_count = 0;
	struct tree *** stack_ptr = &stack[0]; /* = &stack[stackcount] */

	for (;;) {
		struct tree *node;

		node = *nodeplace;
		if (node == avl_br_empty)
			break;
		*stack_ptr++ = nodeplace; stack_count++;
		if (new_node->key == node->key) {
		   printk("\nDuplicate key insertion! NOT adding key %d", node->key);
		   return -1;
		}

		if (new_node -> key > node->key)
		   nodeplace = &node->tree_avl_right;
		else
		   nodeplace = &node->tree_avl_left;
	}

	new_node->tree_avl_left = avl_br_empty;
	new_node->tree_avl_right = avl_br_empty;
	new_node->tree_avl_height = 1;
	*nodeplace = new_node;
	graid_avl_rebalance(stack_ptr,stack_count);

#ifdef DEBUG_AVL
	printk_avl(*head);
#endif /* DEBUG_AVL */
	
	return 1;
}


/* Removes a node out of a tree. */
int graid_avl_remove (struct tree ** head, struct tree * node_to_delete)
{

	struct tree ** nodeplace = head;
	struct tree ** stack[avl_maxheight];
	int stack_count = 0;
	struct tree *** stack_ptr = &stack[0]; /* = &stack[stackcount] */
	struct tree ** nodeplace_to_delete;

	for (;;) {
		struct tree * node = *nodeplace;
		if (node == avl_br_empty) {
			/* what? node_to_delete not found in tree? */
			printk(KERN_ERR "br: avl_remove: node to delete not found in tree\n");
			return -1;
		}
		*stack_ptr++ = nodeplace; stack_count++;
		if (node_to_delete->key == node->key)
				break;
		if (node_to_delete->key <  node->key)
			nodeplace = &node->tree_avl_left;
		else
			nodeplace = &node->tree_avl_right;
	}
	nodeplace_to_delete = nodeplace;
	/* Have to remove node_to_delete = *nodeplace_to_delete. */
	if (node_to_delete->tree_avl_left == avl_br_empty) {
		*nodeplace_to_delete = node_to_delete->tree_avl_right;
		stack_ptr--; stack_count--;
	} else {
		struct tree *** stack_ptr_to_delete = stack_ptr;
		struct tree ** nodeplace = &node_to_delete->tree_avl_left;
		struct tree * node;
		for (;;) {
			node = *nodeplace;
			if (node->tree_avl_right == avl_br_empty)
				break;
			*stack_ptr++ = nodeplace; stack_count++;
			nodeplace = &node->tree_avl_right;
		}
		*nodeplace = node->tree_avl_left;
		/* node replaces node_to_delete */
		node->tree_avl_left = node_to_delete->tree_avl_left;
		node->tree_avl_right = node_to_delete->tree_avl_right;
		node->tree_avl_height = node_to_delete->tree_avl_height;
		*nodeplace_to_delete = node; /* replace node_to_delete */
		*stack_ptr_to_delete = &node->tree_avl_left; /* replace &node_to_delete->tree_avl_left */
	}
	graid_avl_rebalance(stack_ptr,stack_count);
	return 1;
}

#ifdef DEBUG_AVL

/* print a tree */
void graid_printk_avl (struct tree * tree)
{
	if (tree != avl_br_empty) {
		printk("(");
		printk("%d", tree->key);
		if (tree->tree_avl_left != avl_br_empty) {
			printk_avl(tree->tree_avl_left);
			printk("<");
		}
		if (tree->tree_avl_right != avl_br_empty) {
			printk(">");
			printk_avl(tree->tree_avl_right);
		}
		printk(")\n");
	}
}

static char *avl_check_point = "somewhere";

/* check a tree's consistency and balancing */
static void avl_checkheights (struct tree * tree)
{
	int h, hl, hr;

	if (tree == avl_br_empty)
		return;
	avl_checkheights(tree->tree_avl_left);
	avl_checkheights(tree->tree_avl_right);
	h = tree->tree_avl_height;
	hl = heightof(tree->tree_avl_left);
	hr = heightof(tree->tree_avl_right);
	if ((h == hl+1) && (hr <= hl) && (hl <= hr+1))
		return;
	if ((h == hr+1) && (hl <= hr) && (hr <= hl+1))
		return;
	printk("%s: avl_checkheights: heights inconsistent\n",avl_check_point);
}

/* check that all values stored in a tree are < key */
static void avl_checkleft (struct tree * tree, tree_avl_key_t key)
{
	if (tree == avl_br_empty)
		return;
	avl_checkleft(tree->tree_avl_left,key);
	avl_checkleft(tree->tree_avl_right,key);
	if (tree->tree_avl_key < key)
		return;
	printk("%s: avl_checkleft: left key %lu >= top key %lu\n",avl_check_point,tree->tree_avl_key,key);
}

/* check that all values stored in a tree are > key */
static void avl_checkright (struct tree * tree, tree_avl_key_t key)
{
	if (tree == avl_br_empty)
		return;
	avl_checkright(tree->tree_avl_left,key);
	avl_checkright(tree->tree_avl_right,key);
	if (tree->tree_avl_key > key)
		return;
	printk("%s: avl_checkright: right key %lu <= top key %lu\n",avl_check_point,tree->tree_avl_key,key);
}

/* check that all values are properly increasing */
static void avl_checkorder (struct tree * tree)
{
	if (tree == avl_br_empty)
		return;
	avl_checkorder(tree->tree_avl_left);
	avl_checkorder(tree->tree_avl_right);
	avl_checkleft(tree->tree_avl_left,tree->tree_avl_key);
	avl_checkright(tree->tree_avl_right,tree->tree_avl_key);
}

#endif /* DEBUG_AVL */


