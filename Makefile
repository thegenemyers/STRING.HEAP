DEST_DIR = ~/bin

CFLAGS = -O3 -Wall -Wextra -Wno-unused-result -fno-strict-aliasing

CC = gcc

ALL = Sheap Stats Rtats Cheap SCheap Heap Trie Rran Cran

all: $(ALL)

Sheap: string_heap.c
	$(CC) $(CFLAGS) -USTATS -o Sheap string_heap.c -lm

Stats: string_heap.c
	$(CC) $(CFLAGS) -DSTATS -o Stats string_heap.c -lm

Rtats: string_heap.c
	$(CC) $(CFLAGS) -DSTATS -DRATS -o Rtats string_heap.c -lm

Cheap: collision_heap.c
	$(CC) $(CFLAGS) -o Cheap collision_heap.c -lm

SCheap: combo_heap.c
	$(CC) $(CFLAGS) -o SCheap combo_heap.c -lm

Heap: regular_heap.c
	$(CC) $(CFLAGS) -o Heap regular_heap.c -lm

Trie: trie.c
	$(CC) $(CFLAGS) -o Trie trie.c -lm

Rran: gen_random.c
	$(CC) $(CFLAGS) -o Rran gen_random.c -lm

Cran: gen_repeat.c
	$(CC) $(CFLAGS) -o Cran gen_repeat.c -lm

trials:
	csh trial1 >table1
	csh trial2 >table2

clean:
	rm -f $(ALL)
	rm -fr *.dSYM
	rm -f heaps.tar.gz

install:
	cp $(ALL) $(DEST_DIR)

package:
	make clean
	tar -zcf heaps.gz Makefile *.h *.c
