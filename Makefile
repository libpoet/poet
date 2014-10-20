# C compiler
CC = /usr/bin/gcc

# C compiler flags for POET source files
CFLAGS = -fPIC -Wall -Iinc

# C compiler flags for test source files
TESTCFLAGS = -Wall -Iinc -g -O0

# linker flags for test binaries
TESTLFLAGS = -Llib -lpoet -lhb-acc-pow-shared -lrt -lpthread

# Directories
SRCDIR = src
BINDIR = bin
INCDIR = inc
TESTDIR = test
LIBDIR = lib
LOGDIR = log

# Flags

# DEBUG flag, for using gdb
ifeq ($(DEBUG), 1)
	CFLAGS += -g -O0 -DDEBUG
else
	CFLAGS += -DNDEBUG -O3
endif

# OVERFLOW flag, causing poet fixed point to check for overflows print error statements
ifdef OVERFLOW
	CFLAGS += -DPOET_MATH_OVERFLOW -DOVERFLOW_WARNING -DUNDERFLOW_WARNING
	TESTCFLAGS += -DPOET_MATH_OVERFLOW -DOVERFLOW_WARNING -DUNDERFLOW_WARNING
endif

# FIXED_POINT flag, for compiling the fixed point version of POET
ifdef FIXED_POINT
	CFLAGS += -DFIXED_POINT
	TESTCFLAGS += -DFIXED_POINT
endif

HEADERS = $(wildcard $(INCDIR)/*.h)
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BINDIR)/%.o,$(SOURCES))
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_OBJECTS = $(patsubst $(TESTDIR)/%.c,$(BINDIR)/%.o,$(TEST_SOURCES))
TESTS = $(patsubst $(TESTDIR)/%.c,$(BINDIR)/%,$(TEST_SOURCES))
LIBNAME = libpoet.so
LIBPOET = lib/$(LIBNAME)

# phony targets
.PHONY: all clean

all: $(LIBPOET) $(TESTS)

# Build source object files
$(BINDIR)/%.o : $(SRCDIR)/%.c $(HEADERS) | $(BINDIR)
	$(CC) -c $(CFLAGS) $< -o $@

# Build test object files
$(BINDIR)/%.o : $(TESTDIR)/%.c $(HEADERS) | $(BINDIR)
	$(CC) $(CFLAGS) -c $(TESTCFLAGS) $< -o $@

# Build POET Library
$(LIBPOET) : $(BINDIR)/poet.o $(BINDIR)/poet_config_linux.o | $(LIBDIR)
	$(CC) -shared -Wl,-soname,$(@F) -o $@ $^

# Build test binaries
$(BINDIR)/% : $(BINDIR)/%.o $(LIBPOET) | $(LOGDIR)
	$(CC) $< $(TESTLFLAGS) -o $@

$(LIBDIR) :
	mkdir -p $@

$(BINDIR) :
	mkdir -p $@

$(LOGDIR) :
	mkdir -p $@

# Installation
install: all
	install -m 0644 $(LIBPOET) /usr/local/lib/
	mkdir -p /usr/local/include/poet
	install -m 0644 inc/* /usr/local/include/poet/
	install -m 0644 src/*.h /usr/local/include/poet/

uninstall:
	rm -f /usr/local/lib/$(LIBNAME)
	rm -rf /usr/local/include/poet/

# Cleanup
clean :
	rm -rf $(BINDIR) $(LOGDIR) $(LIBDIR)
