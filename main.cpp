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
#include <map>
#include "common.h"

static bool errorSeen;
static int lineNumber;
static bool trace;//=true;
static FILE *outfile, *newfile;

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

std::map<std::string, int> objectCount;
typedef std::map<std::string, std::string> MapStr;

static void groupAssign(const char *prefix, MapStr &map, const char *op)
{
    MapStr out;

    std::map<std::string, std::map<std::string, std::map<int, int>>> common;
    for (auto pitem: map) {
        if (pitem.first[pitem.first.length()-1] == ']') {
            int ind = pitem.first.rfind("[");
            if (ind > 0) {
                std::string tsub = pitem.first.substr(ind);
                std::string target = pitem.first.substr(0, ind);
                int ind = pitem.second.rfind("[");
                if (ind > 0) {
                    std::string ssub = pitem.second.substr(ind);
                    std::string source = pitem.second.substr(0, ind);
                    tsub = tsub.substr(1, tsub.length() - 2);
                    ssub = ssub.substr(1, ssub.length() - 2);
                    if(tsub.find_first_not_of("0123456789") == std::string::npos
                     && ssub.find_first_not_of("0123456789") == std::string::npos) {
                        common[target][source][atoi(tsub.c_str())] = atoi(ssub.c_str());
                        continue;
                    }
                }
            }
        }
        out[pitem.first] = pitem.second;
    }
    for (auto titem: common) {
        for (auto sitem: titem.second) {
             bool init = false;
             int lower, upper, diff;
             for (auto sub: sitem.second) {
                 if (!init) {
                     init = true;
                     lower = sub.first;
                     diff = sub.second - sub.first;
                 }
                 else {
                     if (sub.first != upper + 1 || sub.second != sub.first + diff) {
                         if (lower == upper)
                             fprintf(newfile, "    %s %s [%d] %s %s [%d] ;\n", prefix, titem.first.c_str(), lower, op, sitem.first.c_str(), lower + diff);
                         else
                             fprintf(newfile, "    %s %s [%d:%d] %s %s [%d:%d] ;\n", prefix, titem.first.c_str(), upper, lower, op, sitem.first.c_str(), upper + diff, lower + diff);
                         lower = sub.first;
                         diff = sub.second - sub.first;
                     }
                 }
                 upper = sub.first;
             }
             if (lower == upper)
                 fprintf(newfile, "    %s %s [%d] %s %s [%d] ;\n", prefix, titem.first.c_str(), lower, op, sitem.first.c_str(), lower + diff);
             else
                 fprintf(newfile, "    %s %s [%d:%d] %s %s [%d:%d] ;\n", prefix, titem.first.c_str(), upper, lower, op, sitem.first.c_str(), upper + diff, lower + diff);
        }
    }
    for (auto pitem: out)
        fprintf(newfile, "    %s %s %s %s ;\n", prefix, pitem.first.c_str(), op, pitem.second.c_str());
}

static std::string lutEquation(int width, unsigned long init)
{
    std::string ret;
    int limit = 1 << width;
    for (int i = 0; i < limit; i++) {
        std::string phrase;
        if (init & 1) {
            for (int j = width - 1; j >= 0; j--) {
                std::string current = " I" + autostr(j);
                if (phrase != "")
                    phrase += " & ";
                if (i & (1 << j)) 
                    phrase += current;
                else
                    phrase += " ( ! " + current + " )";
            }
            if (ret != "")
               ret += " | ";
            ret += " ( " + phrase + " ) ";
        }
        init = init >> 1;
    }
    return tree2str(cleanupBool(str2tree(ret)));
}

static std::string replaceEquation(MapStr &pinValue, int width, unsigned long init)
{
    std::string lut = lutEquation(width, init);
    std::string ret;
    int ind;
    while ((ind = lut.find("I")) != -1) {
        ret += lut.substr(0, ind);
        ret += pinValue[lut.substr(ind, 2)];
        lut = lut.substr(ind+2);
    }
    return ret + lut;
}

void outputInstance(FILE *file, InstanceInfo &item)
{
    fprintf(file, "    %s", item.object.c_str());
    if (item.params.size()) {
        fprintf(file, "#(\n");
        std::string sep;
        for (auto pitem: item.params) {
            fprintf(file, "%s      .%s(%s )", sep.c_str(), pitem.name.c_str(), pitem.value.c_str());
            sep = " ,\n";
        }
        fprintf(file, ")\n");
    }
    fprintf(file, "      %s\n", item.name.c_str());
    std::string sep = "        (";
    for (auto pitem: item.pins) {
        fprintf(file, "%s.%s(%s )", sep.c_str(), pitem.name.c_str(), pitem.value.c_str());
        sep = ",\n         ";
    }
    fprintf(file, ");\n");
}

void analyzeModule(ModuleInfo *module)
{
    fprintf(newfile, "module %s\n    (", module->name.c_str());
    std::string sep;
    std::map<std::string, VarDeclInfo>  varMap;
    for (auto item: module->vars)
        varMap[item.name] = VarDeclInfo{item.direction, item.name, item.range};
    for (auto item: module->ports) {
        std::string direction = item.direction;
        std::string range = item.range;
        std::string name = item.name;
        auto vitem = varMap.find(name);
        if (vitem != varMap.end()) {
            direction = vitem->second.direction;
            range = vitem->second.range;
            vitem->second.name = "";
        }
        fprintf(newfile, "%s%s %s %s", sep.c_str(), direction.c_str(), range.c_str(), name.c_str());
        sep = " ,\n    ";
    }
    fprintf(newfile, ");\n");
    for (auto item: module->vars) {
        if (varMap[item.name].name != "")
            fprintf(stdout, "ZZZZZ    %s %s'%s';\n", item.direction.c_str(), item.range.c_str(), item.name.c_str());
    }
    for (auto item: module->nets) {
        fprintf(newfile, "    %s %s%s;\n", item.type.c_str(), item.range.c_str(), item.name.c_str());
    }
    std::map<std::string, MapStr> mapStorage;
    MapStr mapAssign;
    for (auto item: module->instances) {
        std::string type = item.object;
        std::string initValue;
        int size = item.params.size();
        if (item.params.size()) {
            for (auto pitem: item.params) {
                if (pitem.name == "INIT")
                    initValue = pitem.value;
            }
        }
        MapStr pinValue;
        for (auto pitem: item.pins)
            pinValue[pitem.name] = pitem.value;
        objectCount[item.object]++;
        if (type == "FDRE") {
            if (pinValue.size() != 5 || pinValue["C"] != "CLK" || (pinValue["R"] != "SR" && pinValue["R"] != "\\<const0>")) {
                fprintf(newfile, "FDRE ");
                for (auto pitem: item.pins) {
                    fprintf(newfile, "%s(%s), ", pitem.name.c_str(), pitem.value.c_str());
                }
                fprintf(newfile, "\n");
            }
            mapStorage[pinValue["CE"]][item.name] = pinValue["D"];
            mapAssign[pinValue["Q"]] = item.name;
            if (size == 1)
                continue;
        }
        else if (type.substr(0, 3) == "LUT") {
            if (size == 1) {
                unsigned long init = 0;
                initValue = initValue.substr(initValue.find('h')+1);
                while (initValue != "") {
                    char ch = initValue[0];
                    if (ch >= '0' && ch <= '9')
                        init = (init << 4) + (ch - '0');
                    else if (ch >= 'A' && ch <= 'F')
                        init = (init << 4) + (ch - 'A' + 10);
                    else if (ch >= 'a' && ch <= 'f')
                        init = (init << 4) + (ch - 'a' + 10);
                    initValue = initValue.substr(1);
                }
                mapAssign[pinValue["O"]] = replaceEquation(pinValue, type[3] - '0', init);
                continue;
            }
        }
        else {
        //if (type == "BIBUF")
        //else if (type == "CARRY4")
            outputInstance(newfile, item);
            continue;
        }
printf("[%s:%d] type %s size %d\n", __FUNCTION__, __LINE__, type.c_str(), size);
    }
    groupAssign("assign", mapAssign, "=");
    for (auto item: mapStorage) {
        std::string cond = item.first;
        if (cond != "\\<const1>") {
        fprintf(newfile, "    if (%s )", cond.c_str());
        assert(item.second.size());
        if (item.second.size() > 1)
            fprintf(newfile, " begin");
        fprintf(newfile, "\n");
        }
        groupAssign("   ", item.second, "<=");
        if (cond != "\\<const1>" && item.second.size() > 1)
            fprintf(newfile, "    end\n");
    }
    fprintf(newfile, "endmodule\n");
}
void dumpAnalysis()
{
    return;
printf("[%s:%d]\n", __FUNCTION__, __LINE__);
    for (auto item: objectCount)
        printf(" %10s %4d\n", item.first.c_str(), item.second);
}
void dumpModule(ModuleInfo *module)
{
    fprintf(outfile, "module %s\n    (", module->name.c_str());
    std::string sep;
    for (auto item: module->ports) {
        fprintf(outfile, "%s%s %s%s", sep.c_str(), item.direction.c_str(), item.range.c_str(), item.name.c_str());
//, item.type.c_str());
        sep = " ,\n    ";
    }
    fprintf(outfile, ");\n");
    for (auto item: module->vars) {
        fprintf(outfile, "    %s %s%s ;\n", item.direction.c_str(), item.range.c_str(), item.name.c_str());
    }
    fprintf(outfile, "\n");
    for (auto item: module->nets) {
        fprintf(outfile, "    %s %s%s ;\n", item.type.c_str(), item.range.c_str(), item.name.c_str());
    }
    fprintf(outfile, "\n");
    for (auto item: module->instances) {
        outputInstance(outfile, item);
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
    analyzeModule(&currentModule);
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
// Atomicc stubs needed by expr
void walkReplaceBuiltin(ACCExpr *expr, std::string phiDefault)
{
}
std::string exprWidth(ACCExpr *expr, bool forceNumeric)
{
    return "1";
}

#undef yylex
#define yylex driver.yylexMiddle
#include "verilog.tab.c"

int main( const int argc, const char **argv )
{
    if( argc != 2 )
        return -1;

    std::string outFilename = basename((char *)argv[1]);
    outfile = fopen((outFilename + ".out").c_str(), "w");
    newfile = fopen((outFilename + ".new").c_str(), "w");
    if (!outfile)
        return -1;
    Driver driver;
    driver.parse( argv[1] );
    dumpAnalysis();
    return 0;
}
