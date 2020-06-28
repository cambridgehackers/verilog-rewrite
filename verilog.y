// Copyright (c) 2020 The Connectal Project
// Original author: John Ankcorn

// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

%skeleton "lalr1.cc"
%require  "3.0"
%debug 
%defines 
%define parser_class_name {Parser}

%code requires{
      class Driver;
      class Scanner;

# ifndef YY_NULLPTR
#  if defined __cplusplus && 201103L <= __cplusplus
#   define YY_NULLPTR nullptr
#  else
#   define YY_NULLPTR 0
#  endif
# endif

}

%parse-param { Scanner  &scanner  }
%parse-param { Driver  &driver  }

%define api.value.type variant
%define parse.assert

%token          END    0    "end of file"
%token          ENDSOURCE   "end parse marker"
%token          yaFLOATNUM  "FLOATING-POINT NUMBER"
%token          yaID__LEX   "IDENTIFIER-in-lex"
%token          yaINTNUM    "INTEGER NUMBER"
%token          yaSTRING    "STRING"
%token          yaID__ETC   "IDENTIFIER"
%token          yENDMODULE  "endmodule"
%token          yINOUT      "inout"
%token          yINPUT      "input"
%token          yMODULE     "module"
%token          yOUTPUT     "output"
%token          yWIRE       "wire"
%token		yASSIGN	    "assign"

%locations

%%

source_text:              // ==IEEE: source_text
                ENDSOURCE /* empty */
        |        descriptionList ENDSOURCE
        ;
descriptionList:          // IEEE: part of source_text
                description
        |        descriptionList description
        ;
description:              // ==IEEE: description
                module_declaration
        |        error
        ;
package_or_generate_item_declaration: // ==IEEE: package_or_generate_item_declaration
                net_declaration
        |       ';'
        ;
module_declaration:       // ==IEEE: module_declaration
                modFront importsAndParametersE portsStarE ';'
                        module_itemListE yENDMODULE endLabelE { createModule(); }
        ;
modFront:
                yMODULE lifetimeE idAny { startModule(); }
        ;
importsAndParametersE:    // IEEE: common part of module_declaration, interface_declaration, program_declaration
                parameter_port_listE
        ;
parameter_value_assignmentE: // IEEE: [ parameter_value_assignment ]
                /* empty */
        |        '#' '(' cellparamList ')'
        ;
parameter_port_listE:    // IEEE: parameter_port_list + empty == parameter_value_assignment
                /* empty */
        |        '#' '(' ')'
        ;
portsStarE:              // IEEE: .* + list_of_ports + list_of_port_declarations + empty
                /* empty */
        |        '(' ')'
        |        '(' list_of_ports ')'
        ;
list_of_ports:           // IEEE: list_of_ports + list_of_port_declarations
                port
        |        list_of_ports ',' port
        ;
port:                    // ==IEEE: port
                 portDirNetE signingE rangeList  portSig variable_dimensionListE sigAttrListE { createPort(); }
        |        portDirNetE /*implicit*/        portSig variable_dimensionListE sigAttrListE { createPort(); }
         ;
portDirNetE:             // IEEE: part of port, optional net type and/or direction
                /* empty */              { clearRange(); }
        |        port_direction          { clearRange(); }
        |        port_direction net_type { clearRange(); }
        |        net_type                { clearRange(); } // net_type calls VARNET
         ;
port_declNetE:           // IEEE: part of port_declaration, optional net type
                /* empty */
        |        net_type   // net_type calls VARNET
        ;
portSig:
                id/*port*/ { setPortName(); }
         ;
net_declaration:         // IEEE: net_declaration - excluding implict
                net_declarationFront netSigList ';'
        ;
net_declarationFront:    // IEEE: beginning of net_declaration
                net_declRESET net_type   strengthSpecE net_scalaredE net_dataType
        ;
net_declRESET: /* empty */ ;
net_scalaredE: /* empty */ ;
net_dataType:
                 signingE rangeList delayE
        |        /*implicit*/ delayE
        ;
net_type:                // ==IEEE: net_type
                 yWIRE  { setNetType(); }
        ;
port_direction:          // ==IEEE: port_direction + tf_port_direction
                yINPUT   { setModulePortDirection(); }
        |        yOUTPUT   { setModulePortDirection(); }
        |        yINOUT   { setModulePortDirection(); }
        ;
port_directionReset:     // IEEE: port_direction that starts a port_declaraiton
                yINPUT   { setPortDirection(); }
        |        yOUTPUT   { setPortDirection(); }
        |        yINOUT   { setPortDirection(); }
        ;
port_declaration:        // ==IEEE: port_declaration
        port_directionReset port_declNetE signingE rangeList
                        list_of_variable_decl_assignments
        |           port_directionReset port_declNetE /*implicit*/
                        list_of_variable_decl_assignments
        ;
signingE:      /*empty*/ ;          // IEEE: signing - plus empty
list_of_variable_decl_assignments:        // ==IEEE: list_of_variable_decl_assignments
                variable_decl_assignment
        |        list_of_variable_decl_assignments ',' variable_decl_assignment
        ;
variable_decl_assignment:        // ==IEEE: variable_decl_assignment
                variableDecl variable_dimensionListE sigAttrListE { createVarDecl(); }
        ;
variableDecl:
        id       { setVarDecl(); }
        ;
variable_dimensionListE:        // IEEE: variable_dimension + empty
                /*empty*/
        |        variable_dimensionList
        ;
variable_dimensionList:        // IEEE: variable_dimension + empty
                variable_dimension
        |        variable_dimensionList variable_dimension
        ;
variable_dimension:        // ==IEEE: variable_dimension
                anyrange      { setVariableDimension(); }
        |        '[' constExpr      { setVariableDimension(); } ']'
        ;
module_itemListE:        // IEEE: Part of module_declaration
                /* empty */
        |        module_itemList
        ;
module_itemList:                // IEEE: Part of module_declaration
                module_item
        |        module_itemList module_item
        ;
module_item:                // ==IEEE: module_item
                port_declaration ';'
        |       non_port_module_item
        ;
non_port_module_item:        // ==IEEE: non_port_module_item
        module_or_generate_item
        ;
module_or_generate_item:        // ==IEEE: module_or_generate_item
        module_common_item
        ;
module_common_item:        // ==IEEE: module_common_item
                module_or_generate_item_declaration
        |        etcInst
        ;
module_or_generate_item_declaration:        // ==IEEE: module_or_generate_item_declaration
                package_or_generate_item_declaration
        ;
delayE: /* empty */ ;
netSigList:                // IEEE: list_of_port_identifiers
                netSig
        |        netSigList ',' netSig
        ;
netSig:                        // IEEE: net_decl_assignment -  one element from list_of_port_identifiers
                netId sigAttrListE                          { createNet(); }
        |        netId variable_dimensionList sigAttrListE  { createNet(); }
        ;
netId:
                id/*new-net*/ { setNetId(); }
        ;
sigAttrListE: /* empty */ ;
rangeList:                // IEEE: {packed_dimension}
                anyrange
        |        rangeList anyrange
        ;
anyrange:
                '[' constExpr { setRange1(); } ':' constExpr ']' { setRange(); }
        ;
etcInst:                        // IEEE: module_instantiation + gate_instantiation + udp_instantiation
                instDecl
        ;
instDecl:
                objectId parameter_value_assignmentE instnameList ';'
        ;
objectId:
                id { setObjectId(); }
        ;
instnameList:
                instnameParen
        |        instnameList ',' instnameParen
        ;
instnameParen:
                instanceId instRangeE '(' cellpinList ')' { createInstance(); }
        |        instanceId instRangeE                    { createInstance(); }
        ;
instanceId:
                id { setInstance(); }
        ;
instRangeE:
                /* empty */
        |        '[' constExpr ']' { setRange(); }
        |        '[' constExpr { setRange1(); } ':' constExpr ']' { setRange(); } 
        ;
cellparamList:
                cellparamItList
        ;
cellpinList:
                cellpinItList
        ;
cellparamItList:                // IEEE: list_of_parameter_assignmente
                cellparamItemE
        |        cellparamItList ',' cellparamItemE
        ;
cellpinItList:                // IEEE: list_of_port_connections
                cellpinItemE
        |        cellpinItList ',' cellpinItemE
        ;
cellparamItemE:                // IEEE: named_parameter_assignment + empty
                /* empty: ',,' is legal */
        |        '.' idAny             { setParam(); }
        |        '.' idAny '(' ')'     { setParam(); }
        |        '.' idAny '(' expr ')'{ setParam(); }
        |        expr                  { setParam(); }
        ;
cellpinItemE:                // IEEE: named_port_connection + empty
                /* empty: ',,' is legal */
        |        '.' idAny              { setPin(); }
        |        '.' idAny '(' ')'      { setPin(); }
        |        '.' idAny '(' expr ')' { setPin(); }
        |        expr                   { setPin(); }
        ;
lifetimeE:       /* empty */ ;                  // IEEE: [lifetime]
constExpr:
                expr
        ;
expr:                        // IEEE: part of expression/constant_expression/primary
                 yaINTNUM
        |        yaFLOATNUM
        |        strAsInt
        |        '(' expr ')'
        |        '_' '(' expr ')'
        |        exprOkLvalue
        ;
exprOkLvalue:             // expression that's also OK to use as a variable_lvalue
        exprScope
        |       startCate cateList '}' { finishCateList(); }
        ;
startCate:
                '{' { startCateList(); }
        ;
exprScope:                // scope and variable for use to inside an expression
                idArrayed
        |        //~l~
                 expr '.' idArrayed
        ;
cateList:
                stream_expression
        |        cateList ',' stream_expression
        ;
stream_expression:        // ==IEEE: stream_expression
                expr  { addCateList(); }
        ;
strengthSpecE:  /* empty */ ;  // IEEE: drive_strength + pullup_strength + pulldown_strength + charge_strength - plus empty
id:
                yaID__ETC
        ;
idAny:                    // Any kind of identifier
        	yaID__ETC { setIdAny(); }
        ;
idArrayed:                // IEEE: id + select
                idArrayItem { finishArrayed(0); }
        |        idArrayed '[' expr ']' { finishArrayed(1); }
        |        idArrayed '[' constExpr { setRange1(); } ':' constExpr ']' { finishArrayed(2); } 
        ;
idArrayItem:
	id { setIdArrayed(); }
        ;
strAsInt:
                yaSTRING
        ;
endLabelE:
                /* empty */
        |        ':' idAny
        ;
%%
