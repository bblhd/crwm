CC := cc -std=c99 -Werror -Wall -Wextra -D_POSIX_C_SOURCE=199309L 
INSTALL := /usr/local/bin

all: crwm crwmctl crwmkeys

crwm: crwm.c
	${CC} -std=c99 $^ -o $@ -lxcb -lxcb-randr

crwmctl: crwmctl.c
	${CC} -std=c99 $^ -o $@

crwmkeys: crwmkeys.c
	${CC} $^ -o $@ -lxcb -lxcb-keysyms

clean:
	rm -f crwm crwmctl crwmkeys

install: all
	mkdir -p $(INSTALL)
	cp -f crwm crwmctl crwmkeys $(INSTALL)
	chmod 755 $(INSTALL)/crwm
	chmod 755 $(INSTALL)/crwmctl
	chmod 755 $(INSTALL)/crwmkeys

uninstall:
	rm -f $(INSTALL)/crwm
	rm -f $(INSTALL)/crwmctl
	rm -f $(INSTALL)/crwmkeys
