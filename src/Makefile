streamfs : streamfs.o log.o sfslib.o
	gcc -g -lcurl `pkg-config fuse --libs` -D_FILE_OFFSET_BITS=64 -o streamfs streamfs.o log.o sfslib/sfslib.o

sfslib.o : log.o
	gcc -lcurl -c sfslib/sfslib.c -o sfslib/sfslib.o

streamfs.o : streamfs.c log.h params.h
	gcc -g -Wall `pkg-config fuse --cflags` -c streamfs.c

log.o : log.c log.h params.h
	gcc -g -Wall `pkg-config fuse --cflags` -c log.c

clean:
	rm -f streamfs *.o sfslib/*.o
