CC= gcc
INC = -I ../src
LIB = -L ../src -lwlog -lpthread
CFLAGS= -Wall -g


OBJS=main.o

.c.o:
	$(CC) -c $< $(CFLAGS) $(INC)

all: $(OBJS)
	$(CC) -o demo $(OBJS) $(LIB)  $(LFLAGS)
	rm -f *.o

clean:
	rm -f *.o *~ *.log  
	rm -f demo

