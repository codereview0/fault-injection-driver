# Florentina Popovici
# 

INC = ../include

CC = gcc
# defines
DEF = #-D__KERNEL_VERSION
OPTS = -O3 -g -Wall $(DEF)
INCS = -I. -I$(INC)

SRCS = hashtable.c utils.c cache.c limitedSizeCache.c testHash.c 
#SRCS = hashtable.c utils.c cache.c limitedSizeCache.c testCache.c 
OBJS = ${addsuffix .o,${basename ${SRCS}}}
#TARGET = testCache
TARGET = testHash

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(OPTS) -o $(TARGET) $(OBJS)

$(OBJS): 
	$(CC) $(OPTS) $(INCS) -c ${addsuffix .c,${basename $@}} -o $@

clean:
	rm *.o *.c~ *.h~ core $(TARGET)




