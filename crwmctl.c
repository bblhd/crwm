#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <stdio.h>

#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

void die(char *errstr);

bool setupFIFO();
size_t interpret(char **args);
void cleanupFIFO();

char *fifopath;
bool createdFifo = false;

int main(int argc, char *argv[]) {
	(void) argc;
	if (setupFIFO()) {
		interpret(argv+1);
		cleanupFIFO();
	}
	return 0;
}

bool setupFIFO() {
	if (!getenv("DISPLAY")) return false;
	size_t pathlen = strlen("/tmp/crwm.d/")+strlen(getenv("DISPLAY"))+1;
	fifopath = malloc(pathlen);
	if (!fifopath) return false;
	snprintf(fifopath, pathlen, "/tmp/crwm.d/%s", getenv("DISPLAY"));
	
	if (mkdir("/tmp/crwm.d", S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
		if (mkfifo(fifopath, S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) return true;
		rmdir("/tmp/crwm.d");
	}
	return false;
}

void cleanupFIFO() {
	free(fifopath);
	rmdir("/tmp/crwm.d");
}

enum Commandcodes {
	COMMAND_NULL,
	COMMAND_EXIT,
	COMMAND_RELOAD,
	COMMAND_CLOSE,
	COMMAND_MOVE,
	COMMAND_LOOK,
	COMMAND_SEND,
	COMMAND_SWITCH,
	COMMAND_GROW_VERTICAL,
	COMMAND_GROW_HORIZONTAL,
	COMMAND_BORDER_THICKNESS,
	COMMAND_PADDING_ALL,
	COMMAND_MARGIN_ALL,
	COMMAND_PADDING_HORIZONTAL,
	COMMAND_PADDING_VERTICAL,
	COMMAND_MARGIN_TOP,
	COMMAND_MARGIN_BOTTOM,
	COMMAND_MARGIN_LEFT,
	COMMAND_MARGIN_RIGHT,
};

size_t sendBuffer(size_t n, char *buffer) {
	int fd = open(fifopath, O_WRONLY);
	if (fd) {
		write(fd, buffer, n);
		close(fd);
	}
	return n;
}

#define send(n, args...) sendBuffer(n,(char[]){args})

bool eot(char **argv) {
	return *argv == NULL;
}

bool token(char *match, char ***argvp) {
	if (**argvp && strcmp(match, **argvp)==0) {
		(*argvp)++;
		return true;
	}
	return false;
}

char character(char ***argvp) {
	if (**argvp && **argvp[1] == '\0') {
		char c = ***argvp;
		(*argvp)++;
		return c;
	}
	return 0;
}

char number(char ***argvp) {
	if (**argvp) {
		char *end;
		long n = strtol(**argvp, &end, 10);
		if (*end == '\0' && n != 0) {
			(*argvp)++;
			return (char) n;
		}
	}
	return 0;
}

size_t interpret(char **args) {
	if (token("exit", &args)) {
		if (eot(args)) return send(2, COMMAND_EXIT, 0);
	} else if (token("reload", &args)) {
		if (eot(args)) return send(2, COMMAND_RELOAD, 0);
	} else if (token("close", &args)) {
		if (token("focused", &args)) {
			if (eot(args)) return send(2, COMMAND_CLOSE, 0);
		}
	} else if (token("move", &args)) {
		if (token("focused", &args)) {
			char d = '\0';
			if (token("up", &args)) d='u';
			else if (token("down", &args)) d='d';
			else if (token("left", &args)) d='l';
			else if (token("right", &args)) d='r';
			if (d && eot(args)) return send(2, COMMAND_MOVE, d);
		}
	} else if (token("look", &args)) {
		char d = '\0';
		if (token("up", &args)) d='u';
		else if (token("down", &args)) d='d';
		else if (token("left", &args)) d='l';
		else if (token("right", &args)) d='r';
		if (d && eot(args)) return send(2, COMMAND_LOOK, d);
	} else if (token("send", &args)) {
		if (token("focused", &args) && token("to", &args)) {
			char w = number(&args);
			if (w && eot(args)) return send(2, COMMAND_SEND, w);
		}
	} else if (token("switch", &args)) {
		if (token("to", &args)) {
			char w = number(&args);
			if (w && eot(args)) return send(2, COMMAND_SWITCH, w);
		}
	} else if (token("shrink", &args)) {
		if (token("focused", &args)) {
			bool v=false, h=false;
			v = token("vertically", &args);
			if (!v) h = token("horizontally", &args);
			
			if ((v || h) && token("by", &args)) {
				char n = number(&args);
				if (n>0 && eot(args)) {
					return send(2, v ? COMMAND_GROW_VERTICAL : COMMAND_GROW_HORIZONTAL, -n);
				}
			}
		}
	} else if (token("grow", &args)) {
		if (token("focused", &args)) {
			bool v=false, h=false;
			v = token("vertically", &args);
			if (!v) h = token("horizontally", &args);
			
			if ((v || h) && token("by", &args)) {
				char n = number(&args);
				if (n>0 && eot(args)) {
					return send(2, v ? COMMAND_GROW_VERTICAL : COMMAND_GROW_HORIZONTAL, n);
				}
			}
		}
	} else {
		enum Commandcodes command = COMMAND_NULL;
		
		if (token("padding", &args)) {
			if (token("all", &args)) command = COMMAND_PADDING_ALL;
			else if (token("horizontal", &args)) command = COMMAND_PADDING_HORIZONTAL;
			else if (token("vertical", &args)) command = COMMAND_PADDING_VERTICAL;
		} else if (token("margin", &args)) {
			if (token("all", &args)) command = COMMAND_MARGIN_ALL;
			else if (token("top", &args)) command = COMMAND_MARGIN_TOP;
			else if (token("bottom", &args)) command = COMMAND_MARGIN_BOTTOM;
			else if (token("left", &args)) command = COMMAND_MARGIN_LEFT;
			else if (token("right", &args)) command = COMMAND_MARGIN_RIGHT;
		} else if (token("thickness", &args)) command = COMMAND_BORDER_THICKNESS;
		
		if (command != COMMAND_NULL) {
			return send(2, command, number(&args));
		}
	}
	fprintf(stderr, "Bad crwmctl command.\n");
	return 0;
}
