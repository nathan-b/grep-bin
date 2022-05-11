DBFLAGS=-g -O0 -DDEBUG
NDBFLAGS=-O2
CPPFLAGS=-Wall -Werror
OUTPUT=gb

CPPFILES=main.cpp
TESTFILES=buftest.cpp
LIBS=

debug: $(CPPFILES)
	g++ $(CPPFLAGS) $(DBFLAGS) $(INCLUDES) -o $(OUTPUT) $(CPPFILES) $(LIBS)

release: $(CPPFILES)
	g++ $(CPPFLAGS) $(NDBFLAGS) -o $(OUTPUT) $(CPPFILES) $(LIBS)

test: $(TESTFILES)
	g++ $(CPPFLAGS) $(DBFLAGS) -o tests $(TESTFILES) $(LIBS)
clean:
	-rm $(OUTPUT)
