/*			R B _ C R E A T E . C
 *
 *	Written by:	Paul Tanenbaum
 *
 *  $Header$
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "redblack.h"
#define		RB_CREATE	1
#include "rb_internals.h"

/*		    R B _ C R E A T E ( )
 *
 *		    Create a red-black tree
 *
 *	This function has three parameters: a comment describing the
 *	tree to create, the number of linear orders to maintain
 *	simultaneously, the comparison functions (one per order).  On
 *	success, rb_create() returns a pointer to the red-black tree
 *	header record created.  Otherwise, it returns RB_TREE_NULL.
 */
rb_tree *rb_create (char *description, int nm_orders, int (**order_funcs)())
{
    int		order;
    rb_tree	*tree;

    if (((tree = (rb_tree *) malloc(sizeof(rb_tree))) == RB_TREE_NULL)	||
	((tree -> rbt_root = (struct rb_node **)
		    malloc(nm_orders * sizeof(struct rb_node))) == 0))
    {
	fputs("rb_create(): Ran out of memory\n", stderr);
	return (RB_TREE_NULL);
    }
    tree -> rbt_magic = RB_TREE_MAGIC;
    tree -> rbt_description = description;
    tree -> rbt_nm_orders = nm_orders;
    tree -> rbt_order = order_funcs;
    for (order = 0; order < nm_orders; ++order)
	rb_root(tree, order) = RB_NODE_NULL;
    return (tree);
}

/*			_ R B _ I N S E R T ( )
 *
 *	    Insert a node into one linear order of a red-black tree
 *
 *	This function has three parameters: the tree and linear order into
 *	which to insert the new node and the contents of the node. On success,
 *	_rb_insert() returns the value 1.  Otherwise, it returns 0.
 */
static int _rb_insert (rb_tree *tree, int order, struct rb_node *new_node)
{
    struct rb_node	*node;
    struct rb_node	*parent;
    int			(*compare)();


    RB_CKMAG(tree, RB_TREE_MAGIC, "red-black tree");
    RB_CKMAG(new_node, RB_NODE_MAGIC, "red-black node");

    /*
     *	Initialize the new node
     */
    rb_parent(new_node, order) =
    rb_left_child(new_node, order) =
    rb_right_child(new_node, order) = RB_NODE_NULL;

    parent = RB_NODE_NULL;
    node = rb_root(tree, order);
    compare = rb_order_func(tree, order);
    while (node != RB_NODE_NULL)
    {
	parent = node;
	if ((*compare)(new_node -> rbn_data, node -> rbn_data) < 0)
	    node = rb_left_child(node, order);
	else
	    node = rb_right_child(node, order);
    }
    rb_parent(new_node, order) = parent;
    if (parent == RB_NODE_NULL)
	rb_root(tree, order) = new_node;
    else if ((*compare)(new_node -> rbn_data, parent -> rbn_data) < 0)
	rb_left_child(parent, order) = new_node;
    else
	rb_right_child(parent, order) = new_node;

    return (1);
}

/*		    R B _ I N S E R T ( )
 *
 *		Insert a node into a red-black tree
 *
 *	This function has two parameters: the tree into which to insert
 *	the new node and the contents of the node.  The bulk of this code
 *	is from T. H. Cormen, C. E. Leiserson, and R. L. Rivest.  _Intro-
 *	duction to Algorithms_.  Cambridge, MA: MIT Press, 1990. p. 268.
 *	On success, rb_insert() returns the value 1.  Otherwise, it returns 0.
 */
int rb_insert (rb_tree *tree, void *data)
{
    int			nm_orders;
    int			order;
    struct rb_node	*node;

    /* Check data type of the parameter "tree" */
    RB_CKMAG(tree, RB_TREE_MAGIC, "red-black tree");
    nm_orders = tree -> rbt_nm_orders;

    /* Create a new node */
    if (((node = (struct rb_node *) malloc(sizeof(struct rb_node)))
	== RB_NODE_NULL)						||
	((node -> rbn_parent = (struct rb_node **)
		    malloc(nm_orders * sizeof(struct rb_node))) == 0)	||
	((node -> rbn_left = (struct rb_node **)
		    malloc(nm_orders * sizeof(struct rb_node))) == 0)	||
	((node -> rbn_right = (struct rb_node **)
		    malloc(nm_orders * sizeof(struct rb_node))) == 0)	||
	((node -> rbn_color =
	    malloc((size_t) ceil((double) (nm_orders / 8.0)))) == 0))
    {
	fputs("rb_insert(): Ran out of memory\n", stderr);
	return (0);
    }

    /*
     *	Fill the node and insert it into the tree
     */
    node -> rbn_magic = RB_NODE_MAGIC;
    node -> rbn_tree = tree;
    node -> rbn_data = data;
    /*
     *	If the tree was empty, install this node as the root
     *	and give it a null parent and null children
     */
    if (rb_root(tree, 0) == RB_NODE_NULL)
	for (order = 0; order < nm_orders; ++order)
	{
	    rb_root(tree, order) = node;
	    rb_parent(node, order) =
	    rb_left_child(node, order) =
	    rb_right_child(node, order) = RB_NODE_NULL;
	}
    /*	Otherwise, insert the node into the tree */
    else
	for (order = 0; order < nm_orders; ++order)
	{
	    _rb_insert(tree, order, node);
	    rb_set_color(node, order, RB_RED);
	}

    /* Record the node with which we've been working */
    current_node = node;
    return (1);
}

/*		    R B _ C R E A T E 1 ( )
 *
 *		Create a single red-black tree
 *
 *	This function has two parameters: a comment describing the
 *	tree to create and a comparison function.  Rb_create1() builds
 *	an array of one function pointer and passes it to rb_create().
 *	Onsuccess, rb_create1() returns a pointer to the red-black tree
 *	header record created.  Otherwise, it returns RB_TREE_NULL.
 */
rb_tree *rb_create1 (char *description, int (*order_func)())
{
    int		(**ofp)();

    if ((ofp = (int (**)()) malloc(sizeof(int (*)()))) == NULL)
    {
	fputs("rb_create1(): Ran out of memory\n", stderr);
	return (RB_TREE_NULL);
    }
    *ofp = order_func;
    return (rb_create(description, 1, ofp));
}
