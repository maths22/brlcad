/*			R E D B L A C K . H
 *
 *	Written by:	Paul Tanenbaum
 *
 *  $Header$
 */

#ifndef REDBLACK_H
#define REDBLACK_H seen

/*
 *	Data structures
 */
typedef struct
{
    long	 	rbt_magic;	  /* Magic no. for integrity check */
    char		*rbt_description; /* Comment for diagnostics */
    int		 	rbt_nm_orders;	  /* Number of simultaneous orders */
    int			(**rbt_order)();  /* Comparison functions */
    void		(*rbt_print)();	  /* Data pretty-print function */
    struct rb_node	**rbt_root;	  /* The actual trees */
    struct rb_node	*rbt_current;	  /* Current node */
    struct rb_node	*rbt_empty_node;  /* Sentinel representing nil */
}	rb_tree;
#define	RB_TREE_NULL	((rb_tree *) 0)

struct rb_package
{
    long		rbp_magic;	/* Magic no. for integrity check */
    struct rb_node	**rbp_node;	/* Containing nodes */
    void		*rbp_data;	/* Application data */
};
#define	RB_PKG_NULL	((struct rb_package *) 0)

struct rb_node
{
    long		rbn_magic;	/* Magic no. for integrity check */
    rb_tree		*rbn_tree;	/* Tree containing this node */
    struct rb_node	**rbn_parent;	/* Parents */
    struct rb_node	**rbn_left;	/* Left subtrees */
    struct rb_node	**rbn_right;	/* Right subtrees */
    char		*rbn_color;	/* Colors of this node */
    struct rb_package	**rbn_package;	/* Contents of this node */
    int			rbn_pkg_refs;	/* How many orders are being used? */
};
#define	RB_NODE_NULL	((struct rb_node *) 0)

/*
 *	Applications interface to rb_extreme()
 */
#define	SENSE_MIN	0
#define	rb_min(t,o)	rb_extreme((t), (o), SENSE_MIN)
#define	SENSE_MAX	1
#define	rb_max(t,o)	rb_extreme((t), (o), SENSE_MAX)
#define rb_pred(t,o)	rb_neighbor((t), (o), SENSE_MIN)
#define rb_succ(t,o)	rb_neighbor((t), (o), SENSE_MAX)
#define rb_search1(t,n)	rb_search((t), 0, (n))

/*
 *	Applications interface to LIBREDBLACK
 */
rb_tree *rb_create	(
			    char	*description,
			    int 	nm_orders,
			    int		(**order_funcs)()
			);
rb_tree *rb_create1	(
			    char	*description,
			    int		(*order_func)()
			);
void rb_delete		(
			    rb_tree	*tree,
			    int		order
			);
void rb_diagnose_tree	(
			    rb_tree	*tree,
			    int		order
			);
void *rb_extreme	(
			    rb_tree	*tree,
			    int		order,
			    int		sense
			);
int rb_insert		(
			    rb_tree	*tree,
			    void	*data
			);
void rb_install_print	(
			    rb_tree	*tree,
			    void	(*print_func)()
			);
void *rb_neighbor	(
			    rb_tree	*tree,
			    int		order,
			    int		sense
			);
void *rb_search		(
			    rb_tree	*tree,
			    int		order,
			    void	*data
			);
void rb_summarize_tree	(   rb_tree	*tree	);
void rb_walk		(
			    rb_tree	*tree,
			    int		order,
			    void	(*visit)()
			);

#endif /* REDBLACK_H */
