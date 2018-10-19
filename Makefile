all : sender receiver

sender: packet.o sender.o
	gcc -o sender packet.o  sender.o -lz

receiver: packet.o receiver.o
	gcc -o receiver packet.o receiver.o -lz

packet.o: src/packet.c src/packet_interface.h
	gcc -c src/packet.c -lz

receiver.o: src/receiver.c src/receiver.h
	gcc -c src/receiver.c

sender.o: src/sender.c src/sender.h
	gcc -c src/sender.c
clean :
	rm sender receiver packet.o receiver.o sender.o
