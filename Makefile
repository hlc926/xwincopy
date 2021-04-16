
cc = gcc
cflags = -g -O2 -Wall -Wextra

objs = xwincopy.o

target = xwincopy

.SUFFIXES: .c .o

%.o:%.c
	$(cc) $(cflags) -o $@ -c $<

all: $(target)

$(target): $(objs)
	$(cc) $(cflags) -o $@ $^ -lX11

clean:
	rm -f $(objs) $(target)

