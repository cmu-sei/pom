.PHONY: c
MINISAT_LIB = -lminisat -L./minisat/core
#MINISAT_OBJS = ./minisat/objs/*.o 
#PROFILE = -pg
PROFILE = -std=gnu++98 -fno-strict-aliasing
STATIC = -static

CXX := $(shell which g++ 2>/dev/null || which clang++ 2>/dev/null)

all: fmla ghostq aig-to-fmla
#pexp
# minisat-lib-r 

minisat-lib-d:
	export MROOT=`pwd`/minisat ; cd ./minisat/core; make -B LIB=minisat libd
minisat-lib-r:
	export MROOT=`pwd`/minisat ; cd ./minisat/core; make -B LIB=minisat libr
minisat-exe:
	export MROOT=`pwd`/minisat ; cd ./minisat/core; make -B r

fmla: fmla.cpp fmla.hh makefile
	perl ./cpp-proto-gen.pl fmla.cpp > fmla.proto
	$(CXX) -O1 $(PROFILE) -DFMLA_MAIN -ggdb3 -Wall -Wshadow -fpermissive -Wno-deprecated fmla.cpp  -o fmla -Wno-unused-result $(STATIC)

pexp: fmla.o fmla.cpp fmla.hh pexp.cpp makefile
	perl ./cpp-proto-gen.pl pexp.cpp > pexp.proto
	$(CXX) -O1 $(PROFILE) -DFMLA_MAIN -ggdb3 -Wall -Wshadow -fpermissive -Wno-deprecated pexp.cpp conio.c fmla.o -o pexp 


sort.o: sort.c sort.h
	$(CXX) -O3 -finline-functions -ggdb3 -Wall -Wno-sign-compare -c sort.c

fmla.o: fmla.cpp fmla.hh makefile
	perl ./cpp-proto-gen.pl fmla.cpp > fmla.proto
	$(CXX) -O1 -$(PROFILE) -finline-functions -ggdb3 -Wall -Wshadow -fpermissive -Wno-deprecated -c fmla.cpp -Wno-unused-result

ghostq: ghostq.cpp quickselect.cpp Heap.h sort.o makefile ghostq.proto ghostq-cleansed.cpp parser.o fmla.cpp fmla.hh fmla.o
	$(CXX) -O1 $(PROFILE) -finline-functions -ggdb3 -Wall -Wshadow -fpermissive -Wno-deprecated $(STATIC) ghostq-cleansed.cpp parser.o sort.o fmla.o -o ghostq

ghostq-cleansed.cpp: ghostq.cpp
	perl ./remove-arg-defaults.pl ghostq.cpp > ghostq-cleansed.cpp

ghostq.proto: ghostq.cpp
	perl ./cpp-proto-gen.pl ghostq.cpp > ghostq.proto

tags: ghostq.cpp
	ctags --language-force=c++ ghostq.cpp


parser.proto: parser.cpp parser.hh
	perl ./cpp-proto-gen.pl parser.cpp > parser.proto

parser.o: parser.cpp parser.proto
	$(CXX) -ggdb3 -O1 -c parser.cpp -o parser.o

aig-to-fmla: aiger.c aig-to-fmla.c makefile
	gcc aiger.c -c -o aiger.o
	gcc aig-to-fmla.c -c -o aig-to-fmla.o
	$(CXX) aiger.o aig-to-fmla.o -o aig-to-fmla


clean:
	rm -fv ghostq ghostq.proto ghostq-cleansed.cpp
