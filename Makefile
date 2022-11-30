name:=crwm
dest:=/usr/local/bin

compile: 
	gcc -Wall -Wextra -Werror -lxcb -lxcb-keysyms -lm -o $(name) main.c pages.c controls.c

install:
	mkdir -p $(dest)
	cp $(name) $(dest)
	sudo chmod 755 $(dest)/$(name)
