/* Generated by re2c */
#line 1 "bug1479044.re"
#define NULL ((char*) 0)
#define YYCTYPE char
#define YYCURSOR p
#define YYLIMIT p
#define YYMARKER q
#define YYFILL(n)

#include <stdio.h>

char *scan281(char *p)
{
	char *q;
start:

#line 18 "<stdout>"
{
	YYCTYPE yych;
	unsigned int yyaccept = 0;

	if ((YYLIMIT - YYCURSOR) < 11) YYFILL(11);
	yych = *YYCURSOR;
	switch (yych) {
	case 0x00:	goto yy16;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy14;
	case 'a':	goto yy2;
	case 'b':	goto yy4;
	case 'c':	goto yy5;
	case 'd':	goto yy6;
	case 'e':	goto yy7;
	case 'f':	goto yy8;
	case 'l':	goto yy9;
	case 'p':	goto yy10;
	case 'r':	goto yy13;
	case 'v':	goto yy11;
	case 'x':	goto yy12;
	default:	goto yy15;
	}
yy2:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy172;
	default:	goto yy3;
	}
yy3:
#line 32 "bug1479044.re"
	{
		goto start;
	}
#line 62 "<stdout>"
yy4:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy170;
	default:	goto yy3;
	}
yy5:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy168;
	default:	goto yy3;
	}
yy6:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'h':	goto yy154;
	case 'o':	goto yy153;
	case 's':	goto yy155;
	default:	goto yy3;
	}
yy7:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy151;
	default:	goto yy3;
	}
yy8:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy149;
	default:	goto yy3;
	}
yy9:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy147;
	default:	goto yy3;
	}
yy10:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'o':	goto yy137;
	case 'p':	goto yy136;
	case 'r':	goto yy135;
	default:	goto yy3;
	}
yy11:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'd':	goto yy133;
	default:	goto yy3;
	}
yy12:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 's':	goto yy108;
	default:	goto yy3;
	}
yy13:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case 'h':	goto yy71;
	default:	goto yy3;
	}
yy14:
	yyaccept = 0;
	yych = *(YYMARKER = ++YYCURSOR);
	switch (yych) {
	case '-':	goto yy18;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy20;
	default:	goto yy3;
	}
yy15:
	yych = *++YYCURSOR;
	goto yy3;
yy16:
	++YYCURSOR;
#line 37 "bug1479044.re"
	{
		return NULL;
	}
#line 163 "<stdout>"
yy18:
	yych = *++YYCURSOR;
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy22;
	default:	goto yy19;
	}
yy19:
	YYCURSOR = YYMARKER;
	switch (yyaccept) {
	case 0: 	goto yy3;
	case 1: 	goto yy63;
	case 2: 	goto yy107;
	case 3: 	goto yy132;
	}
yy20:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	switch (yych) {
	case '-':	goto yy18;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy20;
	default:	goto yy19;
	}
yy22:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	switch (yych) {
	case '-':	goto yy24;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy22;
	default:	goto yy19;
	}
yy24:
	yych = *++YYCURSOR;
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy25;
	default:	goto yy19;
	}
yy25:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	switch (yych) {
	case '-':	goto yy27;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy25;
	default:	goto yy19;
	}
yy27:
	yych = *++YYCURSOR;
	switch (yych) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy28;
	default:	goto yy19;
	}
yy28:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 8) YYFILL(8);
	yych = *YYCURSOR;
	switch (yych) {
	case '.':	goto yy30;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy28;
	default:	goto yy19;
	}
yy30:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'b':	goto yy34;
	case 'd':	goto yy31;
	default:	goto yy33;
	}
yy31:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'h':	goto yy69;
	default:	goto yy33;
	}
yy32:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy33:
	switch (yych) {
	case '\n':	goto yy19;
	case 'm':	goto yy35;
	default:	goto yy32;
	}
yy34:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'o':	goto yy64;
	default:	goto yy33;
	}
yy35:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case 'm':	goto yy35;
	case 'o':	goto yy37;
	default:	goto yy32;
	}
yy37:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case 'd':	goto yy38;
	case 'm':	goto yy35;
	default:	goto yy32;
	}
yy38:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case 'e':	goto yy39;
	case 'm':	goto yy35;
	default:	goto yy32;
	}
yy39:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case 'm':	goto yy40;
	default:	goto yy32;
	}
yy40:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 'o':	goto yy46;
	default:	goto yy42;
	}
yy42:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy44:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 'w':	goto yy49;
	default:	goto yy42;
	}
yy46:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'd':	goto yy47;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy47:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'e':	goto yy48;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy48:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy49:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'a':	goto yy50;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy50:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 's':	goto yy51;
	default:	goto yy42;
	}
yy51:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'h':	goto yy52;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy52:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'i':	goto yy53;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy53:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 'n':	goto yy54;
	default:	goto yy42;
	}
yy54:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'g':	goto yy55;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy55:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 't':	goto yy56;
	default:	goto yy42;
	}
yy56:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 'o':	goto yy57;
	default:	goto yy42;
	}
yy57:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 'n':	goto yy58;
	default:	goto yy42;
	}
yy58:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy59;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy59:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'e':	goto yy60;
	case 'm':	goto yy40;
	case 'w':	goto yy49;
	default:	goto yy42;
	}
yy60:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'd':	goto yy61;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy61:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	case 'u':	goto yy62;
	default:	goto yy42;
	}
yy62:
	yyaccept = 1;
	YYMARKER = ++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy63;
	case '.':	goto yy44;
	case 'm':	goto yy40;
	default:	goto yy42;
	}
yy63:
#line 27 "bug1479044.re"
	{
		return "edu";
	}
#line 580 "<stdout>"
yy64:
	yych = *++YYCURSOR;
	switch (yych) {
	case 't':	goto yy65;
	default:	goto yy33;
	}
yy65:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'h':	goto yy66;
	default:	goto yy33;
	}
yy66:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'e':	goto yy67;
	default:	goto yy33;
	}
yy67:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy68;
	default:	goto yy33;
	}
yy68:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy42;
	default:	goto yy33;
	}
yy69:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'c':	goto yy70;
	default:	goto yy33;
	}
yy70:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy42;
	default:	goto yy33;
	}
yy71:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy73;
	default:	goto yy71;
	}
yy73:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy73;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy75;
	default:	goto yy71;
	}
yy75:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy77;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy75;
	default:	goto yy71;
	}
yy77:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy73;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy78;
	default:	goto yy71;
	}
yy78:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy77;
	case '.':	goto yy80;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy78;
	default:	goto yy71;
	}
yy80:
	yych = *++YYCURSOR;
	switch (yych) {
	case '.':	goto yy81;
	default:	goto yy82;
	}
yy81:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
yy82:
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	default:	goto yy81;
	}
yy83:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy87;
	default:	goto yy81;
	}
yy85:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'r':	goto yy92;
	default:	goto yy81;
	}
yy87:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy89;
	case '.':	goto yy85;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy87;
	default:	goto yy81;
	}
yy89:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy90;
	default:	goto yy81;
	}
yy90:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy89;
	case '.':	goto yy85;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':	goto yy90;
	default:	goto yy81;
	}
yy92:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'e':	goto yy93;
	default:	goto yy81;
	}
yy93:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 's':	goto yy94;
	default:	goto yy81;
	}
yy94:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'n':	goto yy95;
	default:	goto yy81;
	}
yy95:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'e':	goto yy96;
	default:	goto yy81;
	}
yy96:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 't':	goto yy97;
	default:	goto yy81;
	}
yy97:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy98;
	default:	goto yy81;
	}
yy98:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'p':	goto yy99;
	case 'r':	goto yy92;
	default:	goto yy81;
	}
yy99:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'i':	goto yy100;
	default:	goto yy81;
	}
yy100:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 't':	goto yy101;
	default:	goto yy81;
	}
yy101:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 't':	goto yy102;
	default:	goto yy81;
	}
yy102:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy103;
	default:	goto yy81;
	}
yy103:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'e':	goto yy104;
	case 'r':	goto yy92;
	default:	goto yy81;
	}
yy104:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'd':	goto yy105;
	default:	goto yy81;
	}
yy105:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '-':	goto yy83;
	case '.':	goto yy85;
	case 'u':	goto yy106;
	default:	goto yy81;
	}
yy106:
	yyaccept = 2;
	YYMARKER = ++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy107;
	case '-':	goto yy83;
	case '.':	goto yy85;
	default:	goto yy81;
	}
yy107:
#line 22 "bug1479044.re"
	{
		return "resnet";
	}
#line 987 "<stdout>"
yy108:
	yych = *++YYCURSOR;
	switch (yych) {
	case 't':	goto yy109;
	default:	goto yy19;
	}
yy109:
	yych = *++YYCURSOR;
	switch (yych) {
	case 't':	goto yy110;
	default:	goto yy19;
	}
yy110:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy111;
	default:	goto yy19;
	}
yy111:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'd':	goto yy112;
	default:	goto yy19;
	}
yy112:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy113;
	default:	goto yy19;
	}
yy113:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy114:
	yych = *++YYCURSOR;
	switch (yych) {
	case '.':	goto yy19;
	default:	goto yy116;
	}
yy115:
	++YYCURSOR;
	if ((YYLIMIT - YYCURSOR) < 2) YYFILL(2);
	yych = *YYCURSOR;
yy116:
	switch (yych) {
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'h':
	case 'i':
	case 'j':
	case 'k':	goto yy115;
	case '.':	goto yy117;
	default:	goto yy19;
	}
yy117:
	yych = *++YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	default:	goto yy118;
	}
yy118:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	default:	goto yy118;
	}
yy120:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 'u':	goto yy122;
	default:	goto yy118;
	}
yy122:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 's':	goto yy123;
	default:	goto yy118;
	}
yy123:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 'w':	goto yy124;
	default:	goto yy118;
	}
yy124:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 'e':	goto yy125;
	default:	goto yy118;
	}
yy125:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 's':	goto yy126;
	default:	goto yy118;
	}
yy126:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 't':	goto yy127;
	default:	goto yy118;
	}
yy127:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy128;
	default:	goto yy118;
	}
yy128:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 'n':	goto yy129;
	case 'u':	goto yy122;
	default:	goto yy118;
	}
yy129:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 'e':	goto yy130;
	default:	goto yy118;
	}
yy130:
	++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy19;
	case '.':	goto yy120;
	case 't':	goto yy131;
	default:	goto yy118;
	}
yy131:
	yyaccept = 3;
	YYMARKER = ++YYCURSOR;
	if (YYLIMIT <= YYCURSOR) YYFILL(1);
	yych = *YYCURSOR;
	switch (yych) {
	case '\n':	goto yy132;
	case '.':	goto yy120;
	default:	goto yy118;
	}
yy132:
#line 17 "bug1479044.re"
	{
		return "dsl";
	}
#line 1191 "<stdout>"
yy133:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy134;
	default:	goto yy19;
	}
yy134:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy135:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'e':	goto yy142;
	default:	goto yy19;
	}
yy136:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy139;
	default:	goto yy19;
	}
yy137:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'o':	goto yy138;
	default:	goto yy19;
	}
yy138:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy139:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'd':	goto yy140;
	default:	goto yy19;
	}
yy140:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy141;
	default:	goto yy19;
	}
yy141:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy142:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'm':	goto yy143;
	default:	goto yy19;
	}
yy143:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'i':	goto yy144;
	default:	goto yy19;
	}
yy144:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'u':	goto yy145;
	default:	goto yy19;
	}
yy145:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'm':	goto yy146;
	default:	goto yy19;
	}
yy146:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'C':	goto yy114;
	default:	goto yy19;
	}
yy147:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy148;
	default:	goto yy19;
	}
yy148:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy149:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy150;
	default:	goto yy19;
	}
yy150:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy151:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy152;
	default:	goto yy19;
	}
yy152:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy153:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'r':	goto yy166;
	default:	goto yy19;
	}
yy154:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'c':	goto yy165;
	default:	goto yy19;
	}
yy155:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy156;
	default:	goto yy19;
	}
yy156:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'g':	goto yy158;
	case 'p':	goto yy157;
	default:	goto yy19;
	}
yy157:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy164;
	default:	goto yy19;
	}
yy158:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'w':	goto yy159;
	default:	goto yy19;
	}
yy159:
	yych = *++YYCURSOR;
	switch (yych) {
	case '4':	goto yy160;
	default:	goto yy19;
	}
yy160:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy161;
	default:	goto yy19;
	}
yy161:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'o':	goto yy162;
	default:	goto yy19;
	}
yy162:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'o':	goto yy163;
	default:	goto yy19;
	}
yy163:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy164:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy114;
	default:	goto yy19;
	}
yy165:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy71;
	default:	goto yy19;
	}
yy166:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'm':	goto yy167;
	default:	goto yy19;
	}
yy167:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy71;
	default:	goto yy19;
	}
yy168:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy169;
	default:	goto yy19;
	}
yy169:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy170:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy171;
	default:	goto yy19;
	}
yy171:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy114;
	default:	goto yy19;
	}
yy172:
	yych = *++YYCURSOR;
	switch (yych) {
	case 's':	goto yy173;
	default:	goto yy19;
	}
yy173:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'l':	goto yy174;
	default:	goto yy19;
	}
yy174:
	yych = *++YYCURSOR;
	switch (yych) {
	case '.':	goto yy19;
	case 'p':	goto yy175;
	default:	goto yy116;
	}
yy175:
	yych = *++YYCURSOR;
	switch (yych) {
	case 'p':	goto yy176;
	default:	goto yy19;
	}
yy176:
	++YYCURSOR;
	switch ((yych = *YYCURSOR)) {
	case 'p':	goto yy114;
	default:	goto yy19;
	}
}
#line 40 "bug1479044.re"

}

int main(int argc, char **argv)
{
	int n = 0;
	char *largv[2];

	if (argc < 2)
	{
		argc = 2;
		argv = largv;
		argv[1] = "D-128-208-46-51.dhcp4.washington.edu";
	}
	while(++n < argc)
	{
		char *res = scan281(argv[n]);
		printf("%s\n", res ? res : "<NULL>");
	}
	return 0;
}
