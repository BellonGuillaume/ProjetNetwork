all : sender receiver

sender: packet.o sender.o
	gcc -o sender packet.o  sender.o -lz

receiver: packet.o receiver.o
	gcc -o receiver packet.o receiver.o -lz

packet.o: src/packet.c src/packet_interface.h
	gcc -c src/packet.c -lz

receiver.o: src/receiver.c src/receiver.h src/socket_manipulation.c src/commonlib.c src/buffer_struct.c
	gcc -c src/receiver.c src/socket_manipulation.c src/commonlib.c src/buffer_struct.c

sender.o: src/sender.c src/sender.h src/socket_manipulation.c src/commonlib.c src/buffer_struct.c
	gcc -c src/sender.c src/socket_manipulation.c src/commonlib.c src/buffer_struct.c
clean :
	rm sender receiver packet.o receiver.o sender.o commonlib.o socket_manipulation.o buffer_struct.o
