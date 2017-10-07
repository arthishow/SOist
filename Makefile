# Makefile
# Sistemas Operativos, DEI/IST/ULisboa 2017-18

CFLAGS += -g -Wall -pedantic
CC = gcc

heatSim: main.o matrix2d.o
	$(CC) -o $@ $^ $(CFLAGS) 

main.o: main.c matrix2d.h
	$(CC) $@ -c $< $(CFLAGS)

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $@ -c $< $(CFLAGS)

clean:
	rm -f *.o heatSim

zip:
	zip heatSim_entraga1.zip main.c matrix2d.c matrix2d.h mplib3.c mplib3.h leQueue.c leQueue.h Makefile

run:
	./heatSim 10 10.0 10.0 0.0 0.0 10 2 1
