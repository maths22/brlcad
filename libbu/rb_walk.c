/*			R B _ W A L K . C
 *
 *	Written by:	Paul Tanenbaum
 *
 *  $Header$
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "redblack.h"
#include "rb_internals.h"

/*		        _ R B _ W A L K ( )
 *
 *	    Perform an inorder tree walk on a red-black tree
 *
 *	This function has three parameters: the root of the tree
 *	to traverse, the order on which to do the walking, and the
 *	function to apply to each node.
 */
void _rb_walk (struct rb_node *root, int order, void (*visit)())
{

    /* Check data type of the parameter "root" */
    if (root != 0)
    {
	RB_CKMAG(root, RB_NODE_MAGIC, "red-black node");
	_rb_walk (rb_left_child(root, order), order, visit);
	visit(root -> rbn_data);
	_rb_walk (rb_right_child(root, order), order, visit);
    }
}

/*		        R B _ W A L K ( )
 *
 *	        Applications interface to _rb_walk()
 *
 *	This function has three parameters: the tree to traverse,
 *	the order on which to do the walking, and the function to
 *	apply to each node.
 */
void rb_walk (rb_tree *tree, int order, void (*visit)())
{

    RB_CKMAG(tree, RB_TREE_MAGIC, "red-black tree");
    _rb_walk(rb_root(tree, order), order, visit);
}
