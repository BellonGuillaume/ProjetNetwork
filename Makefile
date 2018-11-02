all : sender receiver test

sender: packet.o sender.o
	gcc -o sender packet.o  sender.o -lz

receiver: packet.o receiver.o
	gcc -o receiver packet.o receiver.o -lz

test: test_prog.o packet.o
	gcc -o test test_prog.o packet.o -lz -lcunit

packet.o: src/packet.c src/packet_interface.h
	gcc -c src/packet.c -lz

receiver.o: src/receiver.c src/receiver.h src/socket_manipulation.c src/commonlib.c src/window.c
	gcc -c src/receiver.c src/socket_manipulation.c src/commonlib.c src/window.c

sender.o: src/sender.c src/sender.h src/socket_manipulation.c src/commonlib.c src/window.c
	gcc -c src/sender.c src/socket_manipulation.c src/commonlib.c src/window.c

test_prog.o: tests/test_prog.o src/window.c src/commonlib.c
	gcc -c tests/test_prog.c src/window.c src/commonlib.c -lcunit

clean :
	rm sender receiver packet.o receiver.o sender.o commonlib.o socket_manipulation.o tests/test_prog.o window.o test test_prog.o
