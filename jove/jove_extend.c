/*
   Jonathan Payne at Lincoln-Sudbury Regional High School 4-19-83
  
   jove_extend.c

   This contains the TENEX style command completion routine and
   various random routines.  This is not well organized...use the
   tags feature of jove to find your way around...  */

#include "jove.h"

int	InJoverc = 0;

extern int	getch(),
		getchar();

min(a, b)
{
	return a < b ? a : b;
}

BindAKey()
{
	BindSomething(functions);
}

BindMac()
{
	BindSomething(macros);
}

BindSomething(funcs)
struct function	*funcs;
{
	char	*prompt;
	int	c,
		command;

	prompt = FuncName();
	command = findcom(funcs, prompt);
	s_mess("%s%s ", prompt, funcs[command].f_name);
	Asking = strlen(mesgbuf);
	if ((c = (*Getchar)()) == EOF)
		return;
	s_mess("%s%s %c%s", prompt, funcs[command].f_name, c,
				(c == '\033' || c == CTL(X)) ? "-" : "");
	Asking = strlen(mesgbuf);
	if (c != '\033' && c != CTL(X))
		mainmap[c] = &funcs[command];
	else {
		int	next = (*Getchar)();

		if (next == EOF)
			return;
		ignore(sprintf(&mesgbuf[strlen(mesgbuf)], "%c", next));
		if (c == CTL(X))
			pref2map[next] = &funcs[command];
		else
			pref1map[next] = &funcs[command];
	}
	Asking = 0;
}

UnBound()
{
	Beep();
}

/* Describe key */

extern int	EscPrefix(),
		CtlxPrefix();

KeyDesc()
{
	int	key;

	s_mess(FuncName());
	UpdateMesg();
	key = getchar();
	if (mainmap[key]->f.Func == EscPrefix)
		DoKeyDesc(pref1map, key);
	else if (mainmap[key]->f.Func == CtlxPrefix)
		DoKeyDesc(pref2map, key);
	else if (mainmap[key]->f.Func)
		s_mess("%c is bound to %s", key, mainmap[key]->f_name);
	else
		s_mess("%c is unbound", key);
}

DoKeyDesc(map, key)
struct function	**map;
{
	int	c;

	s_mess("%s%c-", FuncName(), key);
	UpdateMesg();
	c = getchar();
	s_mess("%s%c-%c", FuncName(), key, c);
	if (map[c] == 0)
		s_mess("%c-%c is unbound", key, c);
	else
		s_mess("%c-%c is bound to %s", key, c, map[c]->f_name);
	return;
}
		
DescCom()
{
	int	com;
	struct function	*fp;
	WINDOW	*savewp = curwind;

	com = findcom(functions, FuncName());
	if (com < 0)
		complain("No such command");
	fp = &functions[com];
	ignore(UnixToBuf("Command Description", 0, exp_p == 0,
					FINDCOM,
					"describe",
					fp->f_name, 0));
	message("Done");
	SetWind(savewp);
}

Beep()
{
	extern int	errormsg;

	message("");
	rbell();		/* Ring the bell (or flash) */
	errormsg = 0;
}

Apropos()
{
	register struct function	*fp;
	char	*ans;

	ans = ask((char *) 0, "Apropos (keyword) ");
	if (UseBuffers) {
		TellWBuffers("Apropos", 0);
		curwind->w_bufp->b_type = SCRATCHBUF;
	} else
		TellWScreen(0);
	for (fp = functions; fp->f_name; fp++)
		if (sindex(ans, fp->f_name))
			ignore(DoTell(fp->f_name));

	for (fp = variables; fp->f_name; fp++)
		if (sindex(ans, fp->f_name))
			ignore(DoTell(fp->f_name));

	for (fp = macros; fp->f_name; fp++)
		if (sindex(ans, fp->f_name))
			ignore(DoTell(fp->f_name));

	if (UseBuffers) {
		Bof();		/* Go to the beginning of the file */
		NotModified();
	}
	StopTelling();
}

sindex(pattern, string)
register char	*pattern,
		*string;
{
	register int	len = strlen(pattern);

	while (*string) {
		if (*pattern == *string && strncmp(pattern, string, len) == 0)
			return 1;
		string++;
	}
	return 0;
}

Extend()
{
	int	command;

	command = findcom(functions, ": ");
	if (command >= 0)
		ExecFunc(&functions[command], !InJoverc);
}

AskInt(prompt)
char	*prompt;
{
	char	*val = ask((char *) 0, prompt);
	int	value;

	if (strcmp(val, "on") == 0)
		value = 1;
	else if (strcmp(val, "off") == 0)
		value = 0;
	else
		value = atoi(val);
	return value;
}	

PrVar()
{
	int	var;

	var = findcom(variables, FuncName());
	if (var < 0)
		return;
	s_mess("%s is set to %d", variables[var].f_name,
				  *(variables[var].f.Var));
}

SetVar()
{
	int	command,
		value;
	static char	tmp[70];

	strcpy(tmp, ": set ");
	command = findcom(variables, tmp);
	if (command < 0)
		return;
	ignore(sprintf(&tmp[strlen(tmp)], "%s ", variables[command].f_name));
	value = AskInt(tmp);
	*(variables[command].f.Var) = value;
	setfuncs(globflags);
}
			
findcom(possible, prompt)
struct function possible[];
char	*prompt;
{
	char	response[132],
		*cp,
		*begin,
		c;
	int	minmatch,
		command,
		i,
		lastmatch,
		numfound,
		length,
		exactmatch;

	strcpy(response, prompt);
	message(response);
	Asking = strlen(prompt);
	begin = response + Asking;
	cp = begin;
	*cp = 0;

	while (c = (*Getchar)()) {
		switch (c) {
		case EOF:
			return EOF;

		case CTL(U):
			cp = begin;
			*cp = 0;
			break;

		case '\r':
		case '\n':
			command = match(possible, begin);
			if (command >= 0) {
				Asking = 0;
				return command;
			}
			if (cp == begin) {
				Asking = 0;
				return EOF;
			}
			rbell();
			if (InJoverc)
				return EOF;
			break;

		case '\033':		/* Try to complete command */
		case ' ':
			minmatch = 1000;
			numfound = 0;
			exactmatch = lastmatch = -1;
			length = strlen(begin);
			for (i = 0; possible[i].f_name; i++)
				if (numcomp(possible[i].f_name, begin) >=
							length) {
					if (numfound)
						minmatch = min(minmatch, numcomp(possible[lastmatch].f_name, possible[i].f_name));
					else
						minmatch = strlen(possible[i].f_name);
					lastmatch = i;
					if (!strcmp(begin, possible[i].f_name))
						exactmatch = i;
					numfound++;
				}
			if (lastmatch >= 0) {
				strncpy(begin, possible[lastmatch].f_name, minmatch);
				begin[minmatch] = '\0';
				cp = &begin[minmatch];
				if (numfound == 1 || exactmatch != -1) {
					if (c == '\033') {
						message(response);
						UpdateMesg();
					}
					Asking = 0;
					return (numfound == 1) ? lastmatch :
							exactmatch;
				} else if (c == '\033')
					rbell();
			} else {
				rbell();
				if (InJoverc)
					return EOF;
			}
			break;

		case '?':
			if (InJoverc)
				return EOF;
			if (UseBuffers) {
				TellWBuffers("Help", 1);
				curwind->w_bufp->b_type = SCRATCHBUF;
			} else
				TellWScreen(0);
			length = strlen(begin);
			for (i = 0; possible[i].f_name; i++)
				if (numcomp(possible[i].f_name, begin) >= length) {
					int	what = DoTell(possible[i].f_name);

					if (what == ABORT)
						goto abort;
					if (what == STOP)
						break;
				}

			if (UseBuffers)
				Bof();
			StopTelling();
			break;

		default:
			if ((int)(cp = RunEdit(c, begin, cp, (char *) 0, Getchar)) == -1)
abort:
				if (InJoverc)
					return EOF;
				else
					complain("Aborted");
		}
		Asking = cp - response;		/* Wonderful! */
		message(response);
	}
	/* NOTREACHED */
}

numcomp(s1, s2)
register char	*s1, *s2;
{
	register int	i;

	for (i = 0; s1[i] || s2[i]; i++)
		if (s1[i] != s2[i])
			break;
	return i;
}

match(choices, what)
struct function	choices[];
char	*what;
{
	int	len,
		i,
		found = 0,
		save,
		exactmatch = -1;

	len = strlen(what);
	for (i = 0; choices[i].f_name; i++)
		if (strncmp(what, choices[i].f_name, len) == 0) {
			if (strcmp(what, choices[i].f_name) == 0)
				exactmatch = i;
			save = i;
			found++;	/* Found one. */
		}

	if (found == 0)
		save = -1;
	else if (found > 1) {
		if (exactmatch != -1)
			save = exactmatch;
		else
			save = -1;
	}
			
	return save;
}

Source()
{
	char	*com = ask((char *) 0, FuncName());

	if (joverc(com) == -1)
		complain(IOerr("read", com));
}

joverc(filename)
char	*filename;
{
	if ((Input = open(filename, 0)) == -1) {
		Input = 0;
		return -1;
	}

	InJoverc = 1;
	Getchar = getchar;
	while (Ungetc(getchar()) != EOF)
		Extend();
	InJoverc = 0;
	Getchar = getch;
	ignore(close(Input));
	Input = 0;
	Asking = 0;
	message("");
	return 1;
}

BufPos()
{
	register LINE	*lp = curbuf->b_zero;
	register int	i = 1;

	for (i = 1; lp != 0 && lp != curline; i++, lp = lp->l_next)
		;
	s_mess("line %d, column %d", i, curchar);
}

extern char	quots[];

SetQchars()
{
	char	*chars = ask((char *) 0, FuncName());

	strncpy(quots, chars, 10);	/* Hope that they were reasonable characters */
}
