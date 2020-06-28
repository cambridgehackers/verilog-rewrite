/* Copyright (c) 2020 The Connectal Project
 * Original author: John Ankcorn

 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

%option interactive stack noyywrap
%{
#include "verilog.tab.h"

class Scanner {
public:
   Scanner(FILE *in) {
      yyin = in;
   };
   virtual ~Scanner() { }; 
   virtual int yylex( yy::Parser::semantic_type * const lval, yy::Parser::location_type *location );
private:
   /* yyval ptr */
   yy::Parser::semantic_type *yylval = nullptr;
};

extern void yyerrorf(const char* format, ...);
#undef  YY_DECL
#define YY_DECL int Scanner::yylex( yy::Parser::semantic_type * const lval, yy::Parser::location_type *loc )

#define yyterminate() return( yy::Parser::token::END )
#define YY_USER_ACTION loc->step(); loc->columns(yyleng);

#define NEXTLINE() { lineNumber++; }
#define LINECHECKS(textp,len)  { const char* cp=textp; for (int n=len; n; --n) if (cp[n]=='\n') NEXTLINE(); }
#define LINECHECK()  LINECHECKS(yytext,yyleng)
#define FL { }
%}
%option debug
%option nodefault
%option noyywrap

%e 2000
%p 5000
%n 2500
%k 1000
%a 15000
%o 25000

%s STRING ATTRMODE TABLE

ws	[ \t\f\r]+
wsnr	[ \t\f]+
crnl	[\r]*[\n]

	/* identifier */
id	[a-zA-Z_][a-zA-Z0-9_$]*
	/* escaped identifier */
escid	\\[^ \t\f\r\n]+
	/* verilog numbers, constructed to not match the ' that begins a '(  or '{ */
word	[a-zA-Z0-9_]+

vnum1	[0-9]*?['']s?[bcodhBCODH][ \t\n]*[A-Fa-f0-9xXzZ_?]*
vnum2	[0-9]*?['']s?[01xXzZ]
vnum3	[0-9][_0-9]*[ \t\n]*['']s?[bcodhBCODH]?[ \t]*[A-Fa-f0-9xXzZ_?]+
vnum4	[0-9][_0-9]*[ \t\n]*['']s?[bcodhBCODH]
vnum5	[0-9][_0-9]*[ \t\n]*['']s
vnum	{vnum1}|{vnum2}|{vnum3}|{vnum4}|{vnum5}

%%
%{          /** Code executed at the beginning of yylex **/
            yylval = lval;
%}

{ws}		{ }
{crnl}		{ NEXTLINE(); }
"`timescale"{ws}+[^\n\r]*	{ }
"assign"	{ FL; return yy::Parser::token::yASSIGN; }
"endmodule"	{ FL; return yy::Parser::token::yENDMODULE; }
"inout"		{ FL; return yy::Parser::token::yINOUT; }
"input"		{ FL; return yy::Parser::token::yINPUT; }
"module"	{ FL; return yy::Parser::token::yMODULE; }
"output"	{ FL; return yy::Parser::token::yOUTPUT; }
"wire"		{ FL; return yy::Parser::token::yWIRE; }

"{"		{ FL; return yytext[0]; }
"}"		{ FL; return yytext[0]; }
"!"		{ FL; return yytext[0]; }
"#"		{ FL; return yytext[0]; }
"$"		{ FL; return yytext[0]; }
"%"		{ FL; return yytext[0]; }
"&"		{ FL; return yytext[0]; }
"("		{ FL; return yytext[0]; }
")"		{ FL; return yytext[0]; }
"*"		{ FL; return yytext[0]; }
"+"		{ FL; return yytext[0]; }
","		{ FL; return yytext[0]; }
"-"		{ FL; return yytext[0]; }
"."		{ FL; return yytext[0]; }
"/"		{ FL; return yytext[0]; }
":"		{ FL; return yytext[0]; }
";"		{ FL; return yytext[0]; }
"<"		{ FL; return yytext[0]; }
"="		{ FL; return yytext[0]; }
">"		{ FL; return yytext[0]; }
"?"		{ FL; return yytext[0]; }
"@"		{ FL; return yytext[0]; }
"["		{ FL; return yytext[0]; }
"]"		{ FL; return yytext[0]; }
"^"		{ FL; return yytext[0]; }
"|"		{ FL; return yytext[0]; }
"~"		{ FL; return yytext[0]; }

{escid}		{ FL; return yy::Parser::token::yaID__LEX; }
{id}		{ FL; return yy::Parser::token::yaID__LEX; }
\"[^\"\\]*\"	{ FL; return yy::Parser::token::yaSTRING; }
   /* \" 	{ yy_push_state(STRING); yymore(); } */

{vnum}		{ FL; return yy::Parser::token::yaINTNUM; }
[0-9][_0-9]*	{ FL; return yy::Parser::token::yaINTNUM; }
[0-9][_0-9]*(\.[_0-9]+)([eE][-+]?[_0-9]+)? { FL; return yy::Parser::token::yaFLOATNUM; }
[0-9][_0-9]*(\.[_0-9]+)?([eE][-+]?[_0-9]+) { FL; return yy::Parser::token::yaFLOATNUM; }
<STRING><<EOF>>	{ yyerrorf("EOF in unterminated string"); yyleng = 0; yy_pop_state(); }
<STRING>{crnl}	{ yyerrorf("Unterminated string"); NEXTLINE(); }
<STRING>\\{crnl}	{ yymore(); NEXTLINE(); }
<STRING>\\. 	{ yymore(); }
<STRING>\" 	{ yy_pop_state(); FL; return yy::Parser::token::yaSTRING; }
<STRING>{word}	{ yymore(); }
<STRING>.	{ yymore(); }

<ATTRMODE>{crnl}	{ yymore(); NEXTLINE(); }
<ATTRMODE>"*)"		{ yy_pop_state(); }
<ATTRMODE>{word}	{ yymore(); }
<ATTRMODE>. 		{ yymore(); }
<ATTRMODE><<EOF>>	{ yyerrorf("EOF in (*"); yyleng = 0; yy_pop_state(); }
"(*"({ws}|{crnl})*({id}|{escid}).*"*)"	{ yymore(); /*yy_push_state(ATTRMODE);*/ }

"//"[^\n]*	{ }
.		{ FL; return yytext[0]; }	/* return single char ops. */

<*>.|\n   { printf("Missing verilog.l rule: Default rule invoked in state %d: %s", YY_START, yytext); exit(-1); }
%%

