CC=gcc
AR=ar
CFLAGS= -g --std=c99 -O0 -Wall
LDFLAGS= 
OBJS=bpt_ext.o buf.o conn.o delete.o disk.o helper.o insert.o table.o join.o

NAME=bpt
OUT_TEST=test
OUT_LIB=lib$(NAME).a

all:	$(OUT_TEST)

lib:	$(OUT_LIB)

$(OUT_TEST):	$(OUT_TEST).o $(OUT_LIB)
	$(CC) $< -o $@ -L./ -l$(NAME)

$(OUT_LIB):	$(OBJS)
	$(AR) rc $(OUT_LIB) $^ 

%.o:	%.c
	$(CC) $(CFLAGS) $^ -c -o $@

clean:
	rm -f *.o $(OUT_APP) $(OUT_TEST) $(OUT_LIB)
