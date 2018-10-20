windowtest: packet.o window.o
	gcc -o windowtest packet.o  window.o -lz

packet.o: packet.c window.c
	gcc -c packet.c window.c -lz

clean :
	rm windowtest packet.o window.o
