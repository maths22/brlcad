/*
   Jonathan Payne at Lincoln-Sudbury Regional High School 5-25-83

   jove_c.c

   contains commands for C mode, paren matching routines are in here. */

#include "jove.h"

BUFLOC *
m_paren(orig, stop)
char	orig;		/* One we are on */
LINE	*stop;
{
	char	*origs = "(){}",
		*matches = ")(}{",
		matcher;
	int	which,
		forward,
		count = 0;
	register LINE	*lp = curline;
	register char	*cp,
			c;
	int	c_char = curchar;
	static BUFLOC	ret;

	which = index(origs, orig) - origs;
	forward = (which % 2) == 0;
	matcher = matches[which];

	while (count >= 0) {
		if ((forward && lp == stop->l_next) ||
					(!forward && lp == stop->l_prev))
			return 0;
		if (forward) {
			cp = getright(lp, genbuf) + c_char;
			while (c = *++cp) {
				c_char++;
				if (c == matcher)
					count -= NotInQuotes(genbuf, c_char);
				else if (c == orig)
					count += NotInQuotes(genbuf, c_char);
				if (count < 0)
					goto done;
			}
			lp = lp->l_next;
			c_char = -1;
		} else {
			cp = getright(lp, genbuf);
			cp += (c_char >= 0 ? c_char : (c_char = strlen(genbuf)));
			while ((cp > genbuf) && (c = *--cp)) {
				--c_char;
				if (c == matcher)
					count -= NotInQuotes(genbuf, c_char);
				else if (c == orig)
					count += NotInQuotes(genbuf, c_char);
				if (count < 0)
					goto done;
			}
			c_char = -1;
			lp = lp->l_prev;
		}
	}
done:
	if (count >= 0)
		return 0;
	ret.p_line = lp;
	ret.p_char = forward ? (cp - genbuf) + 1 : (cp - genbuf);
	return &ret;
}

Fparen()
{
	FindMatch(1);
}

Bparen()
{
	FindMatch(-1);
}

/* Move to the matching brace or paren depending on the current position
 * in the buffer.
 */

FindMatch(dir)
{
	BUFLOC	*bp;
	char	c;

	bp = dosearch(dir > 0 ? "[{(]" : "[})]", dir, 1);
	if (bp == 0)
		complain((char *) 0);
	SetDot(bp);
	if (dir > 0)
		BackChar();
	c = linebuf[curchar];
	if (c == '\0' || index("){}(", c) == 0)
		complain((char *) 0);
	if (!NotInQuotes(linebuf, curchar))	/* If in quotes */
		complain("In quotes");
	else {
		bp = m_paren(c, dir > 0 ? curbuf->b_dol : curbuf->b_zero);
		if (bp)
			SetDot(bp);
		else
			complain("No match");
	}
}

/* Make sure character at c_char is not surrounded by double
 * or single quotes.
 */

char	quots[10];

NotInQuotes(buf, pos)
char	*buf;
{
	char	quotchar = 0,
		c;
	int	i;

	if (quots[0] == 0)
		return 1;	/* Not in quotes */
	for (i = 0; i < pos && buf[i]; i++) {
		if ((c = buf[i]) == '\\') {
			i++;
			continue;
		}
		if (!index(quots, c))
			continue;
		if (quotchar == 0)
			quotchar = c;
		else if (c == quotchar)
			quotchar = 0;	/* Terminated string */
	}
	return (quotchar == 0);
}
