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

#include <unistd.h>
#include <stdio.h>
#include <libgen.h> // basename()
#include <list>

static bool errorSeen;
static int lineNumber;
static bool trace;//=true;
static FILE *outfile;

#include "vlex.yy.c"

std::string tokenId, tokenString, tokenInt, tokenFloat, tokenExpr, idAny;

class Driver{
public:
   Driver() = default;

   virtual ~Driver() {
       delete(scanner);
       scanner = nullptr;
       delete(parser);
       parser = nullptr;
    }
   void parse( const char * const filename ) {
       assert( filename != nullptr );
       FILE *in_file;
       in_file = fopen(filename, "r");
       if( ! in_file) {
           exit( EXIT_FAILURE );
       }
       parse_helper( in_file );
    }
   int yylexMiddle( yy::Parser::semantic_type * const lval, yy::Parser::location_type *loc ) {
       static bool sawEnd = false;
       yy::Parser::symbol_type yyla;
       int token = scanner->yylex(&yyla.value, &yyla.location);
       const char *spell = yytext;
    switch (token) {
    case yy::Parser::token::yaID__LEX:
	token = yy::Parser::token::yaID__ETC;
        tokenId = yytext;
        tokenExpr = yytext;
        break;
    case yy::Parser::token::yaSTRING:
        tokenString = yytext;
        tokenExpr = yytext;
        break;
    case yy::Parser::token::yaINTNUM:
        tokenInt = yytext;
        tokenExpr = yytext;
        break;
    case yy::Parser::token::yaFLOATNUM:
        tokenFloat = yytext;
        tokenExpr = yytext;
        break;
    case yy::Parser::token::END:
        if (sawEnd)
            break;
        sawEnd = true;
        token = yy::Parser::token::ENDSOURCE;
        break;
    }
    if (trace || token == 0)
        printf("[%s:%d]                                                    token %d. = '%s'\n", __FUNCTION__, __LINE__, token, spell);
       if (errorSeen) {
printf("[%s:%d]EXIT: line number %d\n", __FUNCTION__, __LINE__, lineNumber);
           exit(-1);
       }
       return token;
   }
private:

   void parse_helper( FILE *in ) {
       delete(scanner);
       try {
          scanner = new Scanner( in );
       } catch( std::bad_alloc &ba ) {
          std::cerr << "Failed to allocate scanner: (" << ba.what() << "), exiting!!\n";
          exit( EXIT_FAILURE );
       }
   
       delete(parser); 
       try {
          parser = new yy::Parser( (*scanner) /* scanner */, (*this) /* driver */ );
          if (trace) {
              parser->set_debug_level(99);
              yy_flex_debug = 1;
          }
          else
              yy_flex_debug = 0;
       } catch( std::bad_alloc &ba ) {
          std::cerr << "Failed to allocate parser: (" << ba.what() << "), exiting!!\n";
          exit( EXIT_FAILURE );
       }
       if( parser->parse() != 0 ) {
          std::cerr << "Parse failed!!\n";
       }
    }
   yy::Parser *parser  = nullptr;
   Scanner *scanner = nullptr;
};

void yy::Parser::error( const location_type &l, const std::string &err_message )
{
    std::cerr << "Error: " << err_message << " at " << l << "\n";
    printf("[%s:%d]ERROR\n", __FUNCTION__, __LINE__);
    errorSeen = true;
}

void yyerrorf(const char* format, ...)
{
    printf("[%s:%d] ERROR: %s\n", __FUNCTION__, __LINE__, format);
    exit(-1);
}

typedef struct {
    std::string name;
    std::string type;
    std::string range;
} NetInfo;
NetInfo currentNet;

typedef struct {
    std::string name;
    std::string value;
} ParamInfo;
typedef struct {
    std::string object;
    std::string name;
    std::list<ParamInfo> pins;
    std::list<ParamInfo> params;
} InstanceInfo;
InstanceInfo currentInstance;

typedef struct {
    std::string lower;
    std::string upper;
} Range;
Range currentRange;
std::string idArrayed;

typedef struct {
    std::string direction;
    std::string type;
    std::string name;
    std::string range;
} PortInfo;
PortInfo currentPort;

typedef struct {
    std::string direction;
    std::string name;
    std::string range;
} VarDeclInfo;
VarDeclInfo currentVarDecl;

typedef struct {
    std::string             name;
    std::list<PortInfo>     ports;
    std::list<NetInfo>      nets;
    std::list<VarDeclInfo>  vars;
    std::list<InstanceInfo> instances;
} ModuleInfo;
ModuleInfo currentModule;

void dumpModule(ModuleInfo *module)
{
    fprintf(outfile, "module %s\n    (", module->name.c_str());
    std::string sep;
    for (auto item: module->ports) {
        fprintf(outfile, "%s%s %s%s", sep.c_str(), item.direction.c_str(), item.range.c_str(), item.name.c_str());
//, item.type.c_str());
        sep = ",\n    ";
    }
    fprintf(outfile, ");\n");
    for (auto item: module->vars) {
        fprintf(outfile, "    %s %s%s;\n", item.direction.c_str(), item.range.c_str(), item.name.c_str());
    }
    fprintf(outfile, "\n");
    for (auto item: module->nets) {
        fprintf(outfile, "    %s %s%s;\n", item.type.c_str(), item.range.c_str(), item.name.c_str());
    }
    fprintf(outfile, "\n");
    for (auto item: module->instances) {
        fprintf(outfile, "    %s", item.object.c_str());
        if (item.params.size()) {
            fprintf(outfile, "#(\n");
            std::string sep;
            for (auto pitem: item.params) {
                fprintf(outfile, "%s      .%s(%s)", sep.c_str(), pitem.name.c_str(), pitem.value.c_str());
                sep = ",\n";
            }
            fprintf(outfile, ")\n");
        }
        fprintf(outfile, "      %s\n", item.name.c_str());
        std::string sep = "        (";
        for (auto pitem: item.pins) {
            fprintf(outfile, "%s.%s(%s)", sep.c_str(), pitem.name.c_str(), pitem.value.c_str());
            sep = ",\n         ";
        }
        fprintf(outfile, ");\n");
    }
    fprintf(outfile, "endmodule\n");
}

void clearRange()
{
    currentRange.lower = "";
    currentRange.upper = "";
}

std::string getRange()
{
    if (currentRange.lower != "") {
        if (currentRange.upper == "")
            return "[" + currentRange.lower + "]";
        else
            return "[" + currentRange.lower + ":" + currentRange.upper + "]";
    }
    return "";
}

void startModule()
{
    currentModule.name = tokenId;
}

void createModule()
{
    dumpModule(&currentModule);
    currentModule.name = "";
    currentModule.ports.clear();
    currentModule.nets.clear();
    currentModule.vars.clear();
    currentModule.instances.clear();
}

///////////////////////////////////////
void setPortName()
{
    currentPort.name = tokenId;
}
void setNetType()
{
    clearRange();
    currentNet.type = yytext;
    currentPort.type = yytext;
}

void setPortDirection()
{
    clearRange();
    currentVarDecl.direction = yytext;
}
void setModulePortDirection()
{
    clearRange();
    currentPort.direction = yytext;
    currentPort.type = "";
}
void createPort()
{
    currentPort.range = getRange();
    currentModule.ports.push_back(currentPort);
}

void setVarDecl()
{
    currentVarDecl.name = tokenId;
}
void createVarDecl()
{
    currentVarDecl.range = getRange();
    currentModule.vars.push_back(currentVarDecl);
}

void setNetId()
{
    currentNet.name = tokenId;
}

void createNet()
{
    currentNet.range = getRange();
    currentModule.nets.push_back(currentNet);
}

///////////////////////////////////////
void setObjectId()
{
    clearRange();
    currentInstance.object = tokenId;
    currentInstance.params.clear();
    currentInstance.pins.clear();
}

void setInstance()
{
    currentInstance.name = tokenId;
}
void setParam()
{
    currentInstance.params.push_back(ParamInfo{idAny, tokenExpr});
}
void setPin()
{
    currentInstance.pins.push_back(ParamInfo{idAny, tokenExpr});
}

void createInstance()
{
    currentModule.instances.push_back(currentInstance);
}

///////////////////////////////////////

void setIdAny()
{
    idAny = tokenId;
    tokenExpr = "";
//printf("[%s:%d] idAny %s\n", __FUNCTION__, __LINE__, idAny.c_str());
}

void setRange1()
{
    currentRange.lower = tokenExpr;
    currentRange.upper = "";
}

void setRange()
{
    currentRange.upper = tokenExpr;
//printf("[%s:%d]range %s:%s\n", __FUNCTION__, __LINE__, currentRange.lower.c_str(), currentRange.upper.c_str());
}

void setIdArrayed()
{
    idArrayed = tokenId;
    currentRange.lower = "";
    currentRange.upper = "";
}
void finishArrayed(int arg)
{
    if (arg == 1)
        setRange1();
    if (arg == 2)
        setRange();
    if (arg != 0)
        idArrayed += getRange();
    tokenExpr = idArrayed;
}

std::string cateList;
void startCateList()
{
    cateList = yytext;
}

void addCateList()
{
    if (cateList != "{")
        cateList += ",";
    cateList += tokenExpr;
}

void finishCateList()
{
    tokenExpr = cateList + yytext;
}

void setVariableDimension()
{
printf("[%s:%d]lasttoken %s\n", __FUNCTION__, __LINE__, tokenExpr.c_str());
}

#undef yylex
#define yylex driver.yylexMiddle
#include "verilog.tab.c"

int main( const int argc, const char **argv )
{
   if( argc != 2 )
      return -1;

   std::string outFilename = basename((char *)argv[1]);
   outFilename += ".out";
   outfile = fopen(outFilename.c_str(), "w");
   if (!outfile)
       return -1;
   Driver driver;
   driver.parse( argv[1] );
   return 0;
}
