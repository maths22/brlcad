/*
 *			R T L I S T . H
 *
 *  A set of general-purpose doubly-linked list macros.
 *  Used throughout LIBRT, but also valuable for general use.
 *
 *  Authors -
 *	Michael John Muuss
 *	Lee A. Butler
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimitied.
 *
 *  $Header$
 */

#ifndef SEEN_RTLIST_H
#define SEEN_RTLIST_H yes


/************************************************************************
 *									*
 *			Doubly-linked list support			*
 *									*
 *  These macros assume that all user-provided structures will have	*
 *  a "struct rt_list" as their first element (often named "l" [ell]).	*
 *  Thus, a pointer to the rt_list struct is a "pun" for the		*
 *  user-provided structure as well, and the pointers can be converted	*
 *  back and forth safely with type casts.				*
 *									*
 *  Furthermore, the head of the linked list is usually			*
 *  a full instance of the user-provided structure			*
 *  (although the storage-conscious programmer could make the head	*
 *  just an rt_list structure, with careful type casting).		*
 *  This makes results in a doubly-linked circular list, with the head	*
 *  having the same shape as all the list members.			*
 *  The application is free to make use of this symmetry and store	*
 *  data values in the head, or the extra storage in the head can	*
 *  be ignored.								*
 *									*
 *  Where a macro expects an argument "p", it should be a pointer to	*
 *  a user-provided structure.						*
 *									*
 *  Where a macro expects an argument "hp", it should be a pointer to	*
 *  a "struct rt_list" located in the list head, e.g., &(head.l).	*
 *									*
 *  Where a macro expects an argument "old", "new", or "cur", it should	*
 *  be a pointer to the "struct rt_list" located either			*
 *  in a user-provided structure, e.g. &((p)->l),			*
 *  or for the case of "old" it may also be in the list head, e.g.	*
 *	RT_LIST_INSERT( &(head.l), &((p)->l) );				*
 *									*
 *  Dequeueing the head of a list is a valid and			*
 *  well defined operation which should be performed with caution.	*
 *  Unless a pointer to some other element of the list is retained	*
 *  by the application, the rest of the linked list can no longer be	*
 *  referred to.							*
 *									*
 ************************************************************************/

struct rt_list  {
	long		magic;
	struct rt_list	*forw;		/* "forward", "next" */
	struct rt_list	*back;		/* "back", "last" */
};
#define RT_LIST_HEAD_MAGIC	0x01016580	/* Magic num for list head */
#define RT_LIST_NULL	((struct rt_list *)0)


/*
 *  Insert "new" item in front of "old" item.  Often, "old" is the head.
 *  To put the new item at the tail of the list, insert before the head, e.g.
 *	RT_LIST_INSERT( &(head.l), &((p)->l) );
 */
#define RT_LIST_INSERT(old,new)	{ \
	(new)->back = (old)->back; \
	(old)->back = (new); \
	(new)->forw = (old); \
	(new)->back->forw = (new);  }

/*
 *  Append "new" item after "old" item.  Often, "old" is the head.
 *  To put the new item at the head of the list, append after the head, e.g.
 *	RT_LIST_APPEND( &(head.l), &((p)->l) );
 */
#define RT_LIST_APPEND(old,new)	{ \
	(new)->forw = (old)->forw; \
	(new)->back = (old); \
	(old)->forw = (new); \
	(new)->forw->back = (new);  }

/* Dequeue "cur" item from anywhere in doubly-linked list */
#define RT_LIST_DEQUEUE(cur)	{ \
	(cur)->forw->back = (cur)->back; \
	(cur)->back->forw = (cur)->forw; \
	(cur)->forw = (cur)->back = RT_LIST_NULL;  /* sanity */ }

/*
 *  "Bulk transfer" all elements attached to list headed by src
 *  the list headed by dest, without examining every element in the list.
 *  RT_LIST_INSERT_LIST places src elements at head of dest list,
 *  RT_LIST_APPEND_LIST places src elements at end of dest list.
 */
#define RT_LIST_INSERT_LIST(dest,src) \
	if( RT_LIST_NON_EMPTY(src) )  { \
		register struct rt_list	*_first = (src)->forw; \
		register struct rt_list	*_last = (src)->back; \
		(dest)->forw->back = _last; \
		_last->forw = (dest)->forw; \
		(dest)->forw = _first; \
		_first->back = (dest); \
		(src)->forw = (src)->back = (src); \
	}

#define RT_LIST_APPEND_LIST(dest,src) \
	if( RT_LIST_NON_EMPTY(src) )  {\
		register struct rt_list	*_first = (src)->forw; \
		register struct rt_list	*_last = (src)->back; \
		_first->back = (dest)->back; \
		(dest)->back->forw = _first; \
		(dest)->back = _last; \
		_last->forw = (dest); \
		(src)->forw = (src)->back = (src); \
	}

/* Test if a doubly linked list is empty, given head pointer */
#define RT_LIST_IS_EMPTY(hp)	((hp)->forw == (hp))
#define RT_LIST_NON_EMPTY(hp)	((hp)->forw != (hp))

/* Handle list initialization */
#define	RT_LIST_UNINITIALIZED(hp)	((hp)->forw == RT_LIST_NULL)
#define RT_LIST_INIT(hp)	{ \
	(hp)->forw = (hp)->back = (hp); \
	(hp)->magic = RT_LIST_HEAD_MAGIC;	/* used by circ. macros */ }
#define RT_LIST_MAGIC_SET(hp,val)	{(hp)->magic = (val);}
#define RT_LIST_MAGIC_WRONG(hp,val)	((hp)->magic != (val))

/* Return re-cast pointer to first element on list.
 * No checking is performed to see if list is empty.
 */
#define RT_LIST_LAST(structure,hp)	\
	((struct structure *)((hp)->back))
#define RT_LIST_PREV(structure,hp)	\
	((struct structure *)((hp)->back))
#define RT_LIST_FIRST(structure,hp)	\
	((struct structure *)((hp)->forw))
#define RT_LIST_NEXT(structure,hp)	\
	((struct structure *)((hp)->forw))

/* Boolean test to see if current list element is the head */
#define RT_LIST_IS_HEAD(p,hp)	\
	(((struct rt_list *)(p)) == (hp))
#define RT_LIST_NOT_HEAD(p,hp)	\
	(((struct rt_list *)(p)) != (hp))

/* Boolean test to see if the next list element is the head */
#define RT_LIST_NEXT_IS_HEAD(p,hp)	\
	(((struct rt_list *)(p))->forw == (hp))
#define RT_LIST_NEXT_NOT_HEAD(p,hp)	\
	(((struct rt_list *)(p))->forw != (hp))

/*
 *  Intended as innards for a for() loop to visit all nodes on list, e.g.:
 *	for( RT_LIST_FOR( p, structure, hp ) )  {
 *		work_on( p );
 *	}
 */
#define RT_LIST_FOR(p,structure,hp)	\
	(p)=RT_LIST_FIRST(structure,hp); \
	RT_LIST_NOT_HEAD(p,hp); \
	(p)=RT_LIST_PNEXT(structure,p)

/*
 *  Innards for a while() loop that constantly picks off the first element.
 *  Useful mostly for a loop that will dequeue every list element, e.g.:
 *	while( RT_LIST_WHILE(p, structure, hp) )  {
 *		RT_LIST_DEQUEUE( &(p->l) );
 *		free( (char *)p );
 *	}
 */
#define RT_LIST_WHILE(p,structure,hp)	\
	(((p)=(struct structure *)((hp)->forw)) != (struct structure *)(hp))

/* compat */
#define RT_LIST(p,structure,hp)		RT_LIST_FOR(p,structure,hp)
#define RT_LIST_LOOP(p,structure,hp)	RT_LIST_WHILE(p,structure,hp)

/* Return the magic number of the first (or last) item on a list */
#define RT_LIST_FIRST_MAGIC(hp)		((hp)->forw->magic)
#define RT_LIST_LAST_MAGIC(hp)		((hp)->back->magic)

/* Return pointer to next (or previous) element, which may be the head */
#define RT_LIST_PNEXT(structure,p)	\
	((struct structure *)(((struct rt_list *)(p))->forw))
#define RT_LIST_PLAST(structure,p)	\
	((struct structure *)(((struct rt_list *)(p))->back))

/* Return pointer two links away, which may include the head */
#define RT_LIST_PNEXT_PNEXT(structure,p)	\
	((struct structure *)(((struct rt_list *)(p))->forw->forw))
#define RT_LIST_PNEXT_PLAST(structure,p)	\
	((struct structure *)(((struct rt_list *)(p))->forw->back))
#define RT_LIST_PLAST_PNEXT(structure,p)	\
	((struct structure *)(((struct rt_list *)(p))->back->forw))
#define RT_LIST_PLAST_PLAST(structure,p)	\
	((struct structure *)(((struct rt_list *)(p))->back->back))

/* Return pointer to circular next element; ie, ignoring the list head */
#define RT_LIST_PNEXT_CIRC(structure,p)	\
	((RT_LIST_FIRST_MAGIC((struct rt_list *)(p)) == RT_LIST_HEAD_MAGIC) ? \
		RT_LIST_PNEXT_PNEXT(structure,(struct rt_list *)(p)) : \
		RT_LIST_PNEXT(structure,p) )

/* Return pointer to circular last element; ie, ignoring the list head */
#define RT_LIST_PLAST_CIRC(structure,p)	\
	((RT_LIST_LAST_MAGIC((struct rt_list *)(p)) == RT_LIST_HEAD_MAGIC) ? \
		RT_LIST_PLAST_PLAST(structure,(struct rt_list *)(p)) : \
		RT_LIST_PLAST(structure,p) )


#endif /* SEEN_RTLIST_H */
