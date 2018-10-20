test: test_prog.o
	gcc -o test test_prog.o -lcunit

test_prog.o: test_prog.c
	gcc -c test_prog.c -lcunit

clean: 
	rm test_prog.o test test.txt out.txt
