# Copyright (c) 2020 The Connectal Project
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

ATOMICCDIR = ../atomicc
CUDDINC = -I../cudd/cudd
CUDDLIB = ../cudd/cudd/.libs/libcudd.a
CXX = clang++
CXXFLAGS = -O0 -g -Wall -std=c++11 -fblocks -fno-diagnostics-color -DYYDEBUG
CXXFLAGS += -I$(ATOMICCDIR) $(CUDDINC)
OBJS  = main.o expr.o

all: vlex.yy.c verilog.tab.c main.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o verilogRewrite $(OBJS) -lBlocksRuntime $(CUDDLIB)

verilog.tab.c: verilog.y
	bison -d -v verilog.y

vlex.yy.c: vlex.l
	flex --outfile=vlex.yy.c  $<

expr.o: $(ATOMICCDIR)/expr.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean:
	rm -rf $(OBJS) verilog.tab.c verilog.tab.h location.hh position.hh \
		stack.hh verilog.output vlex.yy.c verilogRewrite *.v.out

