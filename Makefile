CC		=  gcc
AR      =  ar
CFLAGS	+= -std=c99 -Wall -g
INCLUDEPATH =  ./header
LIBS     = -lpthread
objects  = supermercato.o queue.o cliente.o cassiere.o direttore.o

.PHONY: test2 clean

exe : $(objects)
	$(CC) $(objects) $(CFLAGS) -o supermercato 

queue.o: queue.c header/queue.h
	$(CC) $(CFLAGS) $(LIBS) -c queue.c 

cliente.o: cliente.c header/supermercato.h header/direttore.h header/queue.h header/cliente.h
	$(CC) $(CFLAGS) $(LIBS) -c cliente.c 

cassiere.o: cassiere.c header/supermercato.h header/direttore.h header/queue.h header/cliente.h
	$(CC) $(CFLAGS) $(LIBS) -c cassiere.c 

direttore.o: direttore.c header/supermercato.h header/direttore.h header/queue.h header/cliente.h header/cassiere.h
	$(CC) $(CFLAGS) $(LIBS) -c direttore.c 

supermercato.o: supermercato.c header/queue.h header/supermercato.h header/direttore.h header/cliente.h header/cassiere.h
	$(CC) $(CFLAGS) $(LIBS) -c supermercato.c

test2: 
	(./supermercato ./test/config.txt & echo $$! > pidfile.PID) &
	sleep 25s; \
	kill -1 $$(cat pidfile.PID); \
	chmod +x analisi.sh; \
	wait $$(cat pidfile.PID); \
	./analisi.sh \

clean: 
	rm -f ./supermercato ./*.o ./*.txt ./pidfile.PID ./*.log
