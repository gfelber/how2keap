AUTHOR = "\x06\xfe\x1b\xe2"
NAME = pwn

CC = cc
_CFLAGS := $(CFLAGS) -static -Os -DGNU_SOURCE -masm=intel
CLIBS = -pthread
_CFLAGS += $(CLIBS)

all: out out/$(NAME)

strip: _CFLAGS += -s
strip: all

debug: _CFLAGS += -DDEBUG -g
debug: all

rootfs: all
	cp out/$(NAME) rootfs/$(NAME)

out/$(NAME): out/$(NAME).o out/util.o out/keap.o ./out/io_uring.o
	$(CC) $(_CFLAGS) out/$(NAME).o out/util.o out/keap.o out/io_uring.o -o out/$(NAME)

out/$(NAME).o: $(NAME).c
	$(CC) $(_CFLAGS) -c $(NAME).c -o out/$(NAME).o

out/util.o: ./libs/util.c
	$(CC) $(_CFLAGS) -c libs/util.c -o out/util.o

out/io_uring.o: ./libs/io_uring.c
	$(CC) $(_CFLAGS) -c libs/io_uring.c -o out/io_uring.o

out/keap.o: ./libs/keap.c
	$(CC) $(_CFLAGS) -c libs/keap.c -o out/keap.o

out:
	mkdir -p out

clean:
	rm out/*.o out/$(NAME)
	rmdir out
