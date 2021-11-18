all: dif

dif:
	xxd Sample-png-image-30mb.png > a.hex
	xxd file/Sample-png-image-30mb.png > b.hex
	diff a.hex b.hex
