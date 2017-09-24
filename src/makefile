CC              ?= gcc
AR              ?= ar
CFLAGS			?= -g   -Wall -rdynamic
INCLUDE			=-I.

ECHO = @
DST = libwlog.a

SRC = $(wildcard *.c )
OBJ = $(patsubst %.c,%.o,$(SRC))

%.o:%.c
	$(ECHO)$(CC) -c $(MACRO) $(CFLAGS) $(INCLUDE)  $< -o $@

$(DST):$(OBJ) 
	$(ECHO)ar -r $(DST) $(OBJ)
	$(ECHO)rm -f *.o

.PHONY:clean 
clean:
	$(ECHO)rm -rf $(DST) $(OBJ)
