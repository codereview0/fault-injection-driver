SRCS = fslib.c posix_lib.c
OBJS = ${addsuffix .o,${basename ${SRCS}}}
COORD = coordinator
OPTS = -I/root/vijayan/sosp05/analysis/include -I/usr/src/linux-2.4/include/ -Wall -O6 -g
LIBS = -lpthread

all: $(COORD)

$(COORD): $(OBJS) coordinator.c
	$(CC) $(LIBS) $(OPTS) -o $@ $(OBJS) $@.c

$(OBJS): $(SRCS)
	$(CC) $(OPTS) -c ${addsuffix .c,${basename $@}} -o $@

clean:
	rm -f $(OBJS) core $(COORD)
