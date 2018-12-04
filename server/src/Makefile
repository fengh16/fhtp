.SUFFIXES:.c .o
 
CC=gcc
 
SRCS=server.c listfile_helper.c basefunc.c
OBJS=$(SRCS:.c=.o)
EXEC=server
 
start: $(OBJS)
	$(CC) -Wall -o $(EXEC) $(OBJS)
 
.c.o:
	$(CC) -Wall -o $@ -c $<
 
clean:
	rm -rf $(EXEC) $(OBJS)