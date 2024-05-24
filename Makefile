AUTHOR = "\x06\xfe\x1b\xe2"
NAME = pwn

CC = cc
CFLAGS = -static -Os -DGNU_SOURCE
CLIBS = -pthread
CFLAGS += $(CLIBS)
debug: CFLAGS += -DDEBUG

all: $(NAME)

rootfs: all 
	mv $(NAME) ./rootfs/$(NAME)

$(NAME): $(NAME).o util.o keap.o 
	$(CC) $(CFLAGS) $(NAME).o util.o keap.o -o ./$(NAME)

$(NAME).o: $(NAME).c
	$(CC) $(CFLAGS) -c $(NAME).c

util.o: ./libs/util.c
	$(CC) $(CFLAGS) -c libs/util.c
	
keap.o: ./libs/keap.c
	$(CC) $(CFLAGS) -c libs/keap.c
	
clean:
	rm -f *.o $(NAME) 
