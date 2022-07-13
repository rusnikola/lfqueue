TESTS = wfqueue wfqueue0 lcrq ccqueue msqueue faa delay cas scq scq2 scqd ncq wcq

# if using clang, please also specify -mcx16 for x86-64
CC = gcc
CFLAGS = -g -Wall -O3 -pthread -D_GNU_SOURCE
LDLIBS = -ljemalloc -lpthread -lm

ifeq (${VERIFY}, 1)
	CFLAGS += -DVERIFY
endif

ifeq (${SANITIZE}, 1)
	CFLAGS += -fsanitize=address -fno-omit-frame-pointer
	LDLIBS += -lasan
	LDFLAGS = -fsanitize=address
endif

ifdef JEMALLOC_PATH
	LDFLAGS += -L${JEMALLOC_PATH}/lib -Wl,-rpath,${JEMALLOC_PATH}/lib
	LDLIBS += -ljemalloc
endif

all: $(TESTS)

wfqueue0: CFLAGS += -DMAX_PATIENCE=0
wfqueue0.o: wfqueue.c
	$(CC) $(CFLAGS) -c -o $@ $^

haswell: CFLAGS += -DGUADALUPE_COMPACT
haswell: all

mic: CC = /usr/linux-k1om-4.7/bin/x86_64-k1om-linux-gcc
mic: CFLAGS += -DGUADALUPE_MIC_COMPACT -DLOGN_OPS=6
mic biou: $(filter-out lcrq,$(TESTS))

biou: CFLAGS += -DBIOU_COMPACT

wfqueue wfqueue0: CFLAGS += -DWFQUEUE
lcrq: CFLAGS += -DLCRQ
ccqueue: CFLAGS += -DCCQUEUE
msqueue: CFLAGS += -DMSQUEUE
faa: CFLAGS += -DFAAQ
cas: CFLAGS += -DFAAQ
delay: CFLAGS += -DDELAY
scq: CFLAGS += -DSCQ
scqd: CFLAGS += -DSCQD
scq2: CFLAGS += -DSCQ2
ncq: CFLAGS += -DNCQ
wcq: CFLAGS += -DWCQ

$(TESTS): harness.o
ifeq (${HALFHALF}, 1)
$(TESTS): halfhalf.o
else
$(TESTS): pairwise.o
endif

msqueue lcrq: hzdptr.o xxhash.o

clean:
	rm -f $(TESTS) *.o
