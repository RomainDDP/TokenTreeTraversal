CC=gcc
SRCs=function.c
OBJs=$(SRCs:.c=.o)

CFLAGS=-W -Wall -ansi -pedantic --std=c99

EXE=Projet

$(EXE) : $(OBJs) main.o
	$(CC)  $^ -o $@ $(CFLAGS)

clean : 
	rm -rf *.o

mrproper : clean
	rm -rf $(EXE) *,*/
