all: pack pack_see

pack: package.c
	gcc package.c -g -o package -lpcap
pack_see: package_see.c
	gcc package_see.c -g -o see -lpcap
