#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <time.h>

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

void msleep(unsigned long m) {
    struct timespec ts;
    ts.tv_sec = m / 1000;
    ts.tv_nsec = (m % 1000) * 1000000;
    nanosleep(&ts, &ts);
}

bool setupFIFO() {
	if (!getenv("DISPLAY")) return false;
	size_t pathlen = strlen("/tmp/crwm.d/")+strlen(getenv("DISPLAY"))+1;
	fifopath = malloc(pathlen);
	snprintf(fifopath, pathlen, "/tmp/crwm.d/%s", getenv("DISPLAY"));
	
	if (mkdir("/tmp/crwm.d", S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
		if (mkfifo(fifopath, S_IRWXU | S_IRWXG | S_IRWXO) || errno == EEXIST) return true;
		rmdir("/tmp/crwm.d");
	}
	return false;
}

void cleanupFIFO() {
	free(fifopath);
	rmdir("/tmp/crwm.d");
}

enum Commandcodes {
	COMMAND_NULL=0,
	COMMAND_EXIT=1,
	COMMAND_RELOAD=2,
	COMMAND_CLOSE=3,
	COMMAND_MOVE=4,
	COMMAND_LOOK=5,
	COMMAND_SEND=6,
	COMMAND_SWITCH=7,
	COMMAND_GROW_VERTICAL=8,
	COMMAND_GROW_HORIZONTAL=9,
	COMMAND_START=10,
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
		bool v=false, h=false;
		v = token("vertically", &args);
		if (!v) h = token("horizontally", &args);
		
		if ((v || h) && token("by", &args)) {
			char n = number(&args);
			if (n>0 && eot(args)) return send(2, v ? COMMAND_GROW_VERTICAL : COMMAND_GROW_HORIZONTAL, -n);
		}
	} else if (token("grow", &args)) {
		bool v=false, h=false;
		v = token("vertically", &args);
		if (!v) h = token("horizontally", &args);
		
		if ((v || h) && token("by", &args)) {
			char n = number(&args);
			if (n>0 && eot(args)) return send(2, v ? COMMAND_GROW_VERTICAL : COMMAND_GROW_HORIZONTAL, n);
		}
	} else if (token("start", &args)) {
		char buffer[129];
		buffer[0] = COMMAND_START;
		buffer[1] = 0;
		size_t n = 2;
		while (*args && n < 129) {
			n += snprintf(buffer+n, 129-n, "%s", *args);
			n++;
			args++;
		}
		buffer[1] = n-2;
		return sendBuffer(n, buffer);
	}
	fprintf(stderr, "Bad crwmctl command.\n");
	return 0;
}
