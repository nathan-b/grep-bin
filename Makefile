DBFLAGS=-g -O0 -DDEBUG
NDBFLAGS=-O2
CPPFLAGS=-Wall -Werror --std=c++20
OUTDIR=build
OUTPUT=$(OUTDIR)/gb

GTEST=$(OUTDIR)/gtest
GTBUILD=$(OUTDIR)/gtbuild

GBENCH=$(OUTDIR)/gbench
GBBUILD=$(OUTDIR)/gbbuild

INCLUDES+=-I $(GTEST)/googletest/include -I $(GBENCH)/include
CPPFILES=main.cpp
TESTFILES=buftest.cpp
BENCHFILES=bufbench.cpp
LIBS=
TESTLIBS=$(GTBUILD)/lib/libgtest.a
BENCHLIBS=$(GBBUILD)/src/libbenchmark.a

all: debug test

$(OUTDIR):
	mkdir -p $(OUTDIR)

debug: $(CPPFILES) $(OUTDIR)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

release: $(CPPFILES) $(OUTDIR)
	g++ $(CPPFLAGS) $(NDBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

test: $(CPPFILES) $(TESTFILES) $(OUTDIR) $(TESTLIBS)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTDIR)/tests $(TESTFILES) $(LIBS) $(TESTLIBS)

bench: $(CPPFILES) $(BENCHFILES) $(OUTDIR) $(BENCHLIBS)
	g++ $(CPPFLAGS) $(NDBFLAGS) $(INCLUDES) -o $(OUTDIR)/benchmarks $(BENCHFILES) $(LIBS) $(BENCHLIBS)
 

$(TESTLIBS): $(GTEST)
	mkdir -p $(GTBUILD)
	cmake -B $(GTBUILD) $(GTEST)
	make -C $(GTBUILD)

$(BENCHLIBS): $(GBENCH)
	mkdir -p $(GBBUILD)
	cmake -B $(GBBUILD) -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DCMAKE_BUILD_TYPE=Release $(GBENCH)
	make -C $(GBBUILD)

$(GTEST):
	git clone --depth=1 -b main https://github.com/google/googletest.git $(GTEST)

$(GBENCH):
	git clone https://github.com/google/benchmark.git $(GBENCH)

clean:
	-rm -rf $(OUTPUT) $(OUTDIR)
