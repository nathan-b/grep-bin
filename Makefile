DBFLAGS=-g -O0 -DDEBUG
NDBFLAGS=-O2
CPPFLAGS=-Wall -Werror --std=c++20
OUTDIR=build
OUTPUT=$(OUTDIR)/gb


GTEST=$(OUTDIR)/gtest

INCLUDES+=-I $(GTEST)/googletest/include
CPPFILES=main.cpp
TESTFILES=buftest.cpp
LIBS=
TESTLIBS=$(OUTDIR)/lib/libgtest.a

all: debug test

$(OUTDIR):
	mkdir -p $(OUTDIR)

debug: $(CPPFILES) $(OUTDIR)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

release: $(CPPFILES) $(OUTDIR)
	g++ $(CPPFLAGS) $(NDBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

test: $(CPPFILES) $(TESTFILES) $(OUTDIR) $(GTEST)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTDIR)/tests $(TESTFILES) $(LIBS) $(TESTLIBS)

$(GTEST):
	git clone --depth=1 -b main https://github.com/google/googletest.git $(GTEST)
	cmake -B $(OUTDIR) $(GTEST)
	make -C $(OUTDIR)

clean:
	-rm -rf $(OUTPUT) $(OUTDIR)
