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

CXX   ?= clang++
CXXFLAGS = -O0 -g -Wall -std=c++11 -fno-diagnostics-color -DYYDEBUG
CPPOBJ = main
OBJS  = $(addsuffix .o, $(CPPOBJ))

CLEANLIST =  $(addsuffix .o, $(OBJ)) $(OBJS) \
     verilog.tab.c verilog.tab.h location.hh position.hh \
     stack.hh verilog.output vlex.yy.c verilogRewrite

all: vlex.yy.c verilog.tab.c main.cpp main.o
	$(CXX) $(CXXFLAGS) -o verilogRewrite main.o $(LIBS)

verilog.tab.c: verilog.y
	bison -d -v verilog.y

vlex.yy.c: vlex.l
	flex --outfile=vlex.yy.c  $<

clean:
	rm -rf $(CLEANLIST)

