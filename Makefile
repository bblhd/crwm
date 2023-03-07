dest:=/usr/local/bin

compile: 
	gcc -Wall -Wextra -Werror -lxcb -lxcb-randr -lm -o crwm crwm.c ctheme.c layout.c
	gcc -Wall -Wextra -Werror -lxcb -lxcb-keysyms -lm -o crwmkeys crwmkeys.c
	gcc -Wall -Wextra -Werror -o crwmctl crwmctl.c

install:
	mkdir -p $(dest)
	cp crwm $(dest)
	cp crwmkeys $(dest)
	cp crwmctl $(dest)
	sudo chmod 755 $(dest)/crwm
	sudo chmod 755 $(dest)/crwmkeys
	sudo chmod 755 $(dest)/crwmctl
