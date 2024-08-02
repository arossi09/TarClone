CC = gcc
CFLAGS = -Wall -g
LD = gcc
LDFLAGS =


all: mytar.o archiveC.o archiveT.o archiveX.o ustar.o util.o
	$(LD) $(LDFLAGS) -o mytar mytar.o archiveC.o archiveT.o archiveX.o ustar.o util.o



mytar: mytar.o archiveC.o archiveT.o archiveX.o ustar.o util.o
	$(LD) $(LDFLAGS) -o mytar mytar.o archiveC.o archiveT.o archiveX.o ustar.o util.o



mytar.o: mytar.c
	$(CC) $(CFLAGS) -c -o mytar.o mytar.c


archiveX.o: archiveX.c
	$(CC) $(CFLAGS) -c -o archiveX.o archiveX.c
archiveC.o: archiveC.c
	$(CC) $(CFLAGS) -c -o archiveC.o archiveC.c
archiveT.o: archiveT.c
	$(CC) $(CFLAGS) -c -o archiveT.o archiveT.c

util.o: util.c
	$(CC) $(CFLAGS) -c -o util.o util.c
ustar.o: ustar.c
	$(CC) $(CFLAGS) -c -o ustar.o ustar.c


clean:
	@echo Cleaning things...
	@rm *.o 
