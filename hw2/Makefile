INC=/lib/modules/$(shell uname -r)/build/arch/x86/include

all: trctl treplay

trctl:trctl.c
	gcc -I$(INC)/generated/uapi -I$(INC)/uapi trctl.c -o trctl
treplay:treplay.c
	gcc -I$(INC)/generated/uapi -I$(INC)/uapi treplay.c -o treplay

clean:
	rm -f trctl
	rm -f treplay
