CC=gcc
CFLAGS=



mysh: shell.c interpreter.c shellmemory.c
	$(CC) $(CFLAGS) -D FRAME_STORE_SIZE=$(framesize) -D VAR_STORE_SIZE=$(varmemsize) -c shell.c interpreter.c shellmemory.c kernel.c pcb.c ready_queue.c paging.c -lpthread
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o kernel.o pcb.o ready_queue.o paging.o

clean: 
	rm mysh; rm *.o


