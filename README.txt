README

Environment:
--------------------

Built on LCPU and AQUILA linux systems using GCC and the posix thread library

Building:
--------------------

- Run the makefile using 'make'

OR

- Sequential version: gcc -Wall relax_serial.c -o relax_serial
- Parallel   version: gcc -Wall -lpthread relax_parallel.c -o relax_parallel



Running:
--------------------

Arguments:
	
	- Array dimension: Integer
	- Thread count: Integer
	- Precision: Float
	- Flags:
		
		- '-t': Prints timing information
		- '-p': Prints array results

Serial: ./relax_serial [array dimension] [precision] [flags]

Parallel: ./relax_parallel [array dimension] [thread count] [precision] [flags]