CC=gcc
CFLAGS=-I/usr/include/libgtop-2.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/GL
LDFLAGS=-lgtop-2.0 -lglib-2.0 -lGL -lglut -lm

cpuheatmap: cpuheatmap.o
	$(CC)  cpuheatmap.o -o cpuheatmap $(LDFLAGS)

cpuheatmap.o: cpuheatmap.c
	$(CC) -c cpuheatmap.c $ $(CFLAGS)
clean:
	rm cpuheatmap
	rm *.o

all: cpuheatmap
    


