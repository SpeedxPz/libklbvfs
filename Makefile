CC = gcc
CFLAGS = -fPIC
DUMPMACHINE = $(shell $(CC) -dumpmachine -m$(ARCH))
ARCH = $(shell getconf LONG_BIT)
SRC_ROOT = src
TEST_ROOT = test
KLBVFS_SOURCE = $(SRC_ROOT)/klbvfs.c
LIB_PREFIX = lib
LIB_SUFFIX = .so

default: build

build: klbvfs

mkdir-lib:
	mkdir -p lib/$(DUMPMACHINE)

mkdir-bin:
	mkdir -p bin

klbvfs-arch64: mkdir-lib
	$(CC) $(CFLAGS) -shared -m64 -o lib/$(DUMPMACHINE)/$(LIB_PREFIX)klbvfs$(LIB_SUFFIX) $(KLBVFS_SOURCE)
klbvfs-arch32: mkdir-lib
	$(CC) $(CFLAGS) -shared -m32 -o lib/$(DUMPMACHINE)/$(LIB_PREFIX)klbvfs$(LIB_SUFFIX) $(KLBVFS_SOURCE)

klbvfs: klbvfs-arch$(ARCH)
	@echo "*** BUILD SUCCESSFUL ***"
