# Makefile
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS += -g -Wall -pedantic
CC = gcc

all: heatSim

heatSim: main.o matrix2d.o leQueue.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

main.o: main.c matrix2d.h mplib3.h
	$(CC) -o $@ -c $< $(CFLAGS)

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) -o $@ -c $< $(CFLAGS)

mplib3.o: mplib3.c mplib3.h
	$(CC) -o $@ -c $< $(CFLAGS)
	
mplib4.o: mplib4.c mplib4.h
	$(CC) -o $@ -c $< $(CFLAGS)

leQueue.o: leQueue.c leQueue.h
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -f *.o *.orig heatSim

zip:
	zip heatSim.zip main.c matrix2d.c matrix2d.h mplib3.c mplib3.h mp4lib.c mp4lib.h leQueue.c leQueue.h Makefile

run:
	./heatSim 10 2.0 3.0 20.0 1.0 10 5 0
