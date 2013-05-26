SRCS = fslib.c
OBJS = ${addsuffix .o,${basename ${SRCS}}}
COLONY = colony
IF = inject_fault
COORD = coordinator
OPTS = -I/root/vijayan/sosp05/analysis/include -I/usr/src/linux-2.4/include/ -Wall -O6 -g
LIBS = -lpthread

all: $(COLONY)
#all: $(COORD)

$(COORD): $(OBJS) coordinator.c
	$(CC) $(LIBS) $(OPTS) -o $@ $(OBJS) $@.c

$(COLONY): $(OBJS) fslib.c colony.c
	$(CC) $(LIBS) $(OPTS) -o $@ fslib.o $@.c

$(IF): $(OBJS) inject_fault.c
	$(CC) $(LIBS) $(OPTS) -o $@ $(OBJS) $@.c

$(OBJS): $(SRCS)
	$(CC) $(OPTS) -c ${addsuffix .c,${basename $@}} -o $@

clean:
	rm -f $(TARG) $(OBJS) core $(IF) $(COLONY) $(COORD)
