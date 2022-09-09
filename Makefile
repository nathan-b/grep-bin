DBFLAGS=-g -O0 -DDEBUG
NDBFLAGS=-O2
CPPFLAGS=-Wall -Werror
OUTPUT=gb

CPPFILES=main.cpp
TESTFILES=buftest.cpp
LIBS=
TESTLIBS=$(OUTDIR)/lib/libgtest.a

debug: $(CPPFILES)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

release: $(CPPFILES) $(OUTDIR)
	g++ $(CPPFLAGS) $(NDBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

test: $(TESTFILES) $(OUTDIR) gtest_git
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTDIR)/tests $(TESTFILES) $(LIBS) $(TESTLIBS)

gtest_git:
	git clone --depth=1 -b main https://github.com/google/googletest.git $(GTEST)
	cmake -B $(OUTDIR) $(GTEST)
	make -C $(OUTDIR)

clean:
	-rm $(OUTPUT)
