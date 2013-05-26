TARG = sba
OPTS = -I./include -I../include -I../test_suits/ -Wall -O6 -g
LIBS = -lpthread

all: $(TARG)

$(TARG): sba.c
	$(CC) $(LIBS) $(OPTS) -o $@ $@.c

%.o: %.c
	$(CC) $(OPTS) -c ${addsuffix .c,${basename $@}} -o $@

clean:
	rm -f $(TARG) $(OBJS) core
