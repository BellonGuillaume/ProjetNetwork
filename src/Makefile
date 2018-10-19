windowtest: packet.o buffer_struct.o
	gcc -o windowtest packet.o  buffer_struct.o -lz

packet.o: packet.c buffer_struct.c
	gcc -c packet.c buffer_struct.c -lz

clean :
	rm windowtest packet.o buffer_struct.o
