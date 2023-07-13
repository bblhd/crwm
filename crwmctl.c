#include <stdlib.h>
#include <stdint.h>

#include <unistd.h>

#include <stdio.h>
#include <string.h>

char **setupControlFile(char **args);
void interpret(char **args);

void send(char command, char where, uint32_t window);

int token(char *str, char ***from);
char tokenchar(char ***from);
uint32_t tokenwindow(char ***from);
long tokennumber(char ***from);

uint32_t towindow(char *str);
long tonumber(char *str);
void die(char *message);

char *controlFile = NULL;

int main(int argc, char **argv) {
	(void) argc;
	interpret(setupControlFile(argv+1));
	return 0;
}

char **setupControlFile(char **args) {
	if (token("--file", &args)) {
		if (*args) controlFile = *args++;
		else die("No control file path given to '--file'.");
	} else {
		controlFile = getenv("DISPLAY");
		if (!controlFile) die("Could not get display.");
		if (chdir("/tmp/crwm.d")!=0) die("Could not locate control file.");
	}
	return args;
}

void interpret(char **args) {
	if (token("close", &args)) {
		uint32_t window = tokenwindow(&args);
		send('x','w',window);
	} else if (token("exit", &args)) {
		send('x',0,0);
	} else if (token("move", &args)) {
		uint32_t window = tokenwindow(&args);
		
		char direction = 0;
		if (token("up", &args)) direction = 'u';
		else if (token("down", &args)) direction = 'd';
		else if (token("left", &args)) direction = 'l';
		else if (token("right", &args)) direction = 'r';
		else die("Missing direction in move command.");
		
		send('m', direction, window);
	} else if (token("look", &args)) {
		char direction = 'c';
		if (token("up", &args)) direction = 'u';
		else if (token("down", &args)) direction = 'd';
		else if (token("left", &args)) direction = 'l';
		else if (token("right", &args)) direction = 'r';
		
		uint32_t window = 0;
		if (token("at", &args)) window = tokenwindow(&args);
		else if (token("from", &args)) window = tokenwindow(&args);
		
		send('l', direction, window);
	} else if (token("grow", &args)) {
		uint32_t window = tokenwindow(&args);
		
		char axis = 0;
		if (token("vertically", &args)) axis = 'v';
		else if (token("horizontally", &args)) axis = 'h';
		else die("Missing axis in grow command.");
		
		long amount = 1;
		if (!token("by", &args)) amount = tokennumber(&args);
		
		if (amount >= 1 && amount <= 26) send(axis, 'A'+amount-1, window);
		else if (amount <= -1 && amount >= -26) send(axis, 'a'-amount-1, window);
		else if (amount != 0) die("Growth amount too large.");
	} else if (token("shrink", &args)) {
		uint32_t window = tokenwindow(&args);
		
		char axis = 0;
		if (token("vertically", &args)) axis = 'v';
		else if (token("horizontally", &args)) axis = 'h';
		else die("Missing axis in shrink command.");
		
		long amount = 1;
		if (!token("by", &args)) amount = tokennumber(&args);
		
		if (amount >= 1 && amount <= 26) send(axis, 'a'+amount-1, window);
		else if (amount <= -1 && amount >= -26) send(axis, 'A'-amount-1, window);
		else if (amount != 0) die("Shrinking amount too large.");
	} else if (token("send", &args)) {
		uint32_t window = tokenwindow(&args);
		if (!token("to", &args)) die("Missing 'to' in send command.");
		char page = tokenchar(&args);
		if (!page) die("Missing page ID in send command.");
		send('s',page,window);
	} else if (token("switch", &args)) {
		if (!token("to", &args)) die("Missing 'to' in switch command.");
		char page = tokenchar(&args);
		if (!page) die("Missing page ID in switch command.");
		send('t',page,0);
	} else die("Unrecognised crwm command.");
}

void send(char command, char where, uint32_t window) {
	FILE *file = fopen(controlFile, "w");
	if (file) {
		if (window) fprintf(file, "%c[%c]0x%x", command, where, window);
		else if (where) fprintf(file, "%c[%c]", command, where);
		else fprintf(file, "%c", command);
		fclose(file);
	}
}

int token(char *str, char ***from) {
	if (!**from) return 0;
	if (strcmp(str, **from)==0) {
		(*from)++;
		return 1;
	}
	return 0;
}

char tokenchar(char ***from) {
	if (!**from) return 0;
	if ((**from)[1] == 0) {
		char c = ***from;
		(*from)++;
		return c;
	}
	return 0;
}

uint32_t tokenwindow(char ***from) {
	if (!**from) return 0;
	uint32_t w = towindow(**from);
	(*from)++;
	return w;
}

long tokennumber(char ***from) {
	if (!**from) return 0;
	long n = tonumber(**from);
	(*from)++;
	return n;
}

uint32_t towindow(char *str) {
	if (strcmp(str, "focused")==0) return 0;
	else {
		uint32_t window = tonumber(str);
		if (window == 0) die("No window provided.");
		return window;
	}
}

long tonumber(char *str) {
	return strtol(str, NULL, 0);
}

void die(char *message) {
	fprintf(stderr, "%s\n", message);
	exit(1);
}
