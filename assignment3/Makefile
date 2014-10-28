CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra

INPUTFILESIZEMEGABYTES = 1

KILO = 1024
MEGA = $(shell echo $(KILO)\*$(KILO) | bc)
INPUTFILESIZEBYTES = $(shell echo $(MEGA)\*$(INPUTFILESIZEMEGABYTES) | bc)
INPUTBLOCKSIZEBYTES = $(KILO)
INPUTBLOCKS = $(shell echo $(INPUTFILESIZEBYTES)\/$(INPUTBLOCKSIZEBYTES) | bc)

.PHONY: all clean

all: pi pi-sched rw  mix_fork rw_fork pi_fork 
#rr_quantum

pi.o: pi.c
	$(CC) $(CFLAGS) $<

mix_fork.o: mix_fork.c
	$(CC) $(CFLAGS) $<

pi_fork.o: pi_fork.c
	$(CC) $(CFLAGS) $<

rw_fork.o: rw_fork.c
	$(CC) $(CFLAGS) $<

pi-sched.o: pi-sched.c
	$(CC) $(CFLAGS) $<

rw.o: rw.c
	$(CC) $(CFLAGS) $<

mix_fork: mix_fork.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

pi_fork: pi_fork.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

rw_fork: rw_fork.o rwinput
	$(CC) $(LFLAGS) rw.o -o $@ -lm

pi: pi.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

pi-sched: pi-sched.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

rw: rw.o rwinput
	$(CC) $(LFLAGS) rw.o -o $@ -lm


#This is broken for now. 
rr_quantum: rr_quantum.o
	$(CC) $(LFLAGS) $^ -o $@ -lm

#This is necessary for the rw test, we don't need to change it at all. 
rwinput: Makefile
	dd if=/dev/urandom of=./rwinput bs=$(INPUTBLOCKSIZEBYTES) count=$(INPUTBLOCKS)

rr_quantum.o: rr_quantum.c
	$(CC) $(CFLAGS) $<

clean: testclean
	rm -f pi pi-sched rw rr_quantum pi_fork mix_fork 
	rm -f rwinput
	rm -f *.o
	rm -f *~
	rm -f handout/*~
	rm -f handout/*.log
	rm -f handout/*.aux

testclean:
	rm -f rwoutput*
