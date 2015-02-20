all: serial parallel

serial:
	gcc -Wall relax_serial.c -o relax_serial

parallel:
	gcc -Wall -lpthread relax_parallel.c -o relax_parallel

clean:
	rm -f relax_serial
	rm -f relax_parallel
