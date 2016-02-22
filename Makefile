#
#
#	Author: Koneru, Kaushik 
#
#	Email:konerukaushik@gmail.com
#

CFLAGS=-lpthread

all: clean hosta hostb router

hosta:
	gcc hosta.c -o hosta

hostb:
	gcc hostb.c -o hostb $(CFLAGS)

router:
	gcc router.c -o router

clean:
	rm -rf hosta hostb router
