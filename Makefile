AUTHOR = "\x06\xfe\x1b\xe2"
NAME = pwn

CC = gcc

_CFLAGS := $(CFLAGS) -Os -DGNU_SOURCE -masm=intel
LDFLAGS := -static
LIBS := -pthread

all: out out/$(NAME)

strip: _CFLAGS += -s
strip: all

rootfs: all
	cp out/$(NAME) rootfs/$(NAME)

out/$(NAME): out/$(NAME).o out/util.o out/io_uring.o out/keap.o
	$(CC) $(LDFLAGS) $^ $(LIBS) -o $@

out/$(NAME).o: $(NAME).c
	$(CC) $(_CFLAGS) -c $< -o $@

out/keap.o: ./libs/keap.c
	$(CC) $(_CFLAGS) -c $< -o $@

out/util.o: ./libs/util.c
	$(CC) $(_CFLAGS) -c $< -o $@

out/io_uring.o: ./libs/io_uring.c
	$(CC) $(_CFLAGS) -c $< -o $@

out:
	mkdir -p out

clean:
	rm out/*.o out/$(NAME)
	rmdir out
