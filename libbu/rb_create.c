/*			R B _ C R E A T E . C
 *
 *	Written by:	Paul Tanenbaum
 *
 */
#ifndef lint
static char RCSid[] = "@(#) $Header$";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "machine.h"
#include "vmath.h"
#include "raytrace.h"
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

    if (((tree = (rb_tree *) rt_malloc(sizeof(rb_tree), "red-black tree"))
	    == RB_TREE_NULL)						||
	((tree -> rbt_root = (struct rb_node **)
		    rt_malloc(nm_orders * sizeof(struct rb_node),
			"red-black roots")) == 0)			||
	((tree -> rbt_empty_node = (struct rb_node *)
		    rt_malloc(sizeof(struct rb_node),
			"red-black empty node")) == RB_NODE_NULL)	||
	((tree -> rbt_empty_node  -> rbn_parent = (struct rb_node **)
		rt_malloc(nm_orders * sizeof(struct rb_node *),
			    "red-black parents")) == 0))
    {
	fputs("rb_create(): Ran out of memory\n", stderr);
	return (RB_TREE_NULL);
    }
    tree -> rbt_magic = RB_TREE_MAGIC;
    tree -> rbt_description = description;
    tree -> rbt_nm_orders = nm_orders;
    tree -> rbt_order = order_funcs;
    tree -> rbt_print = 0;
    tree -> rbt_current = tree -> rbt_empty_node;

    /*
     *	Initialize the nil sentinel
     */
    tree -> rbt_empty_node -> rbn_magic = RB_NODE_MAGIC;
    tree -> rbt_empty_node -> rbn_tree = tree;
    for (order = 0; order < nm_orders; ++order)
	(tree -> rbt_empty_node -> rbn_parent)[order] = RB_NODE_NULL;
    tree -> rbt_empty_node -> rbn_left = 0;
    tree -> rbt_empty_node -> rbn_right = 0;

    for (order = 0; order < nm_orders; ++order)
	rb_root(tree, order) = rb_null(tree);
    return (tree);
}

/*			_ R B _ I N S E R T ( )
 *
 *	    Insert a node into one linear order of a red-black tree
 *
 *	This function has three parameters: the tree and linear order into
 *	which to insert the new node and the contents of the node.  If
 *	the new node was equal (modulo the linear order) to a node already
 *	in the tree, _rb_insert() returns 1.  Otherwise, it returns 0.
 */
static int _rb_insert (rb_tree *tree, int order, struct rb_node *new_node)
{
    struct rb_node	*node;
    struct rb_node	*parent;
    int			(*compare)();
    int			comparison;


    RB_CKMAG(tree, RB_TREE_MAGIC, "red-black tree");
    RB_CKORDER(tree, order);
    RB_CKMAG(new_node, RB_NODE_MAGIC, "red-black node");

    /*
     *	Initialize the new node
     */
    rb_parent(new_node, order) =
    rb_left_child(new_node, order) =
    rb_right_child(new_node, order) = rb_null(tree);

    parent = rb_null(tree);
    node = rb_root(tree, order);
    compare = rb_order_func(tree, order);
    while (node != rb_null(tree))
    {
	parent = node;
	comparison = (*compare)(rb_data(new_node, order), rb_data(node, order));
	if (comparison < 0)
	    node = rb_left_child(node, order);
	else
	    node = rb_right_child(node, order);
    }
    rb_parent(new_node, order) = parent;
    if (parent == rb_null(tree))
	rb_root(tree, order) = new_node;
    else if ((*compare)(rb_data(new_node, order), rb_data(parent, order)) < 0)
	rb_left_child(parent, order) = new_node;
    else
	rb_right_child(parent, order) = new_node;
    
    return (comparison == 0);
}

/*		    R B _ I N S E R T ( )
 *
 *		Insert a node into a red-black tree
 *
 *	This function has two parameters: the tree into which to insert
 *	the new node and the contents of the node.  The bulk of this code
 *	is from T. H. Cormen, C. E. Leiserson, and R. L. Rivest.  _Intro-
 *	duction to Algorithms_.  Cambridge, MA: MIT Press, 1990. p. 268.
 *	rb_insert() returns the number of orders for which the new node
 *	was equal to a node already in the tree.
 */
int rb_insert (rb_tree *tree, void *data)
{
    int			nm_orders;
    int			order;
    int			result = 0;
    struct rb_node	*node;
    struct rb_package	*package;

    RB_CKMAG(tree, RB_TREE_MAGIC, "red-black tree");

    nm_orders = tree -> rbt_nm_orders;

    /*
     *	Create a new package
     */
    package = (struct rb_package *)
		rt_malloc(sizeof(struct rb_package), "red-black package");
    package -> rbp_node = (struct rb_node **)
		rt_malloc(nm_orders * sizeof(struct rb_node *),
			    "red-black package nodes");

    /*
     *	Create a new node
     */
    node = (struct rb_node *)
		rt_malloc(sizeof(struct rb_node), "red-black node");
    node -> rbn_parent = (struct rb_node **)
		rt_malloc(nm_orders * sizeof(struct rb_node *),
			    "red-black parents");
    node -> rbn_left = (struct rb_node **)
		rt_malloc(nm_orders * sizeof(struct rb_node *),
			    "red-black left children");
    node -> rbn_right = (struct rb_node **)
		rt_malloc(nm_orders * sizeof(struct rb_node *),
			    "red-black right children");
    node -> rbn_color = (char *)
		rt_malloc((size_t) ceil((double) (nm_orders / 8.0)),
			    "red-black colors");
    node -> rbn_package = (struct rb_package **)
		rt_malloc(nm_orders * sizeof(struct rb_package *),
			    "red-black packages");
    /*
     *	Fill in the package
     */
    package -> rbp_magic = RB_PKG_MAGIC;
    package -> rbp_data = data;
    for (order = 0; order < nm_orders; ++order)
	(package -> rbp_node)[order] = node;

    /*
     *	Fill in the node
     */
    node -> rbn_magic = RB_NODE_MAGIC;
    node -> rbn_tree = tree;
    for (order = 0; order < nm_orders; ++order)
    {
	rb_set_color(node, order, RB_RED);
	(node -> rbn_package)[order] = package;
    }
    node -> rbn_pkg_refs = nm_orders;

    /*
     *	If the tree was empty, install this node as the root
     *	and give it a null parent and null children
     */
    if (rb_root(tree, 0) == rb_null(tree))
	for (order = 0; order < nm_orders; ++order)
	{
	    rb_root(tree, order) = node;
	    rb_parent(node, order) =
	    rb_left_child(node, order) =
	    rb_right_child(node, order) = rb_null(tree);
	}
    /*	Otherwise, insert the node into the tree */
    else
	for (order = 0; order < nm_orders; ++order)
	    result += _rb_insert(tree, order, node);

    /* Record the node with which we've been working */
    rb_current(tree) = node;
    return (result);
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

    if ((ofp = (int (**)())
		rt_malloc(sizeof(int (*)()),
		    "red-black function table")) == NULL)
    {
	fputs("rb_create1(): Ran out of memory\n", stderr);
	return (RB_TREE_NULL);
    }
    *ofp = order_func;
    return (rb_create(description, 1, ofp));
}

/*		    R B _ I N S T A L L _ P R I N T ( )
 *
 *	    Install a pretty-print function in a red-black tree
 *
 *	This function has two parameters: a tree and a pretty-print
 *	function, which LIBREDBLACK uses for diagnostic purposes.
 */
void rb_install_print (rb_tree *tree, void (*print_func)())
{
    RB_CKMAG(tree, RB_TREE_MAGIC, "red-black tree");

    tree -> rbt_print = print_func;
}
