#include <stdlib.h>

#include <unistd.h>

#include <stdio.h>
#include <string.h>

#include <xcb/xcb.h>

void interpret(char **args);

void send(char command, char where, xcb_window_t window);

int token(char *str, char ***from);
char tokenchar(char ***from);
xcb_window_t tokenwindow(char ***from);
long tokennumber(char ***from);

xcb_window_t towindow(char *str);
long tonumber(char *str);
void die(char *message);

void closeWindow(xcb_window_t window);

xcb_connection_t *connection;

xcb_atom_t WM_PROTOCOLS, WM_DELETE_WINDOW;

int main(int argc, char **argv) {
	if (!getenv("DISPLAY")) die("Could not get display.");;
	connection = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(connection)) die("Unable to connect to X.\n");
	
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
		connection,xcb_intern_atom(connection,0,12,"WM_PROTOCOLS"),NULL
	);
	WM_PROTOCOLS = reply ? reply->atom : XCB_NONE;
	free(reply);
	reply = xcb_intern_atom_reply(
		connection,xcb_intern_atom(connection,0,16,"WM_DELETE_WINDOW"),NULL
	);
	WM_DELETE_WINDOW = reply ? reply->atom : XCB_NONE;
	free(reply);
	
	if (chdir("/tmp/crwm.d")!=0) die("Could not locate control file.");
	
	interpret(argv+1);
	
	xcb_disconnect(connection);
	return 0;
}

void interpret(char **args) {
	if (token("close", &args)) {
		xcb_window_t window = tokenwindow(&args);
		if (!window) die("Missing window for close command.");
		closeWindow(window);
	} else if (token("exit", &args)) {
		send('x',0,XCB_NONE);
	} else if (token("move", &args)) {
		xcb_window_t window = tokenwindow(&args);
		if (!window) die("Missing window for move command.");
		
		char direction = 0;
		if (token("up", &args)) direction = 'u';
		else if (token("down", &args)) direction = 'd';
		else if (token("left", &args)) direction = 'l';
		else if (token("right", &args)) direction = 'r';
		else die("Missing direction in move command.");
		
		send('m', direction, window);
	} else if (token("look", &args)) {
		xcb_window_t window = 0;
		char direction = 'c';
		if (token("up", &args)) direction = 'u';
		else if (token("down", &args)) direction = 'd';
		else if (token("left", &args)) direction = 'l';
		else if (token("right", &args)) direction = 'r';
		
		if (token("at", &args)) window = tokenwindow(&args);
		else if (token("from", &args)) window = tokenwindow(&args);
		
		send('l', direction, window);
	} else if (token("grow", &args)) {
		xcb_window_t window = tokenwindow(&args);
		
		char axis = 0;
		if (token("vertically", &args)) axis = 'v';
		else if (token("horizontally", &args)) axis = 'h';
		else die("Missing axis in grow command.");
		
		long amount = 1;
		if (!token("by", &args)) amount = tokennumber(&args);
		
		if (amount >= 1 && amount <= 26) send(axis, 'A'+amount-1, window);
		else if (amount <= -1 && amount >= -26) send(axis, 'a'-amount-1, window);
		else if (amount != 0) die("Growth amount too large.");
	} else if (token("send", &args)) {
		xcb_window_t window = tokenwindow(&args);
		if (!token("to", &args)) die("Missing 'to' in send command.");
		if (!token("table", &args)) die("Missing 'table' in send command.");
		char page = tokenchar(&args);
		if (!page) die("Missing page ID in send command.");
		send('s',page,window);
	} else if (token("go", &args)) {
		char page = tokenchar(&args);
		if (!page) die("Missing page ID in table command.");
		if (!token("to", &args)) die("Missing 'to' in send command.");
		if (!token("table", &args)) die("Missing 'table' in send command.");
		send('t',page,XCB_NONE);
	} else die("Unrecognised crwm command.");
}

void send(char command, char where, xcb_window_t window) {
	FILE *file = fopen(getenv("DISPLAY"), "w");
	if (where) {
		if (window) fprintf(file, "%c[%c]0x%x", command, where, window);
		else fprintf(file, "%c[%c]", command, where);
	} else fprintf(file, "%c", command);
	fclose(file);
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
}

xcb_window_t tokenwindow(char ***from) {
	if (!**from) return 0;
	xcb_window_t w = towindow(**from);
	(*from)++;
	return w;
}

long tokennumber(char ***from) {
	if (!**from) return 0;
	long n = tonumber(**from);
	(*from)++;
	return n;
}

xcb_window_t towindow(char *str) {
	xcb_window_t window;
	if (strcmp(str, "focused")==0) {
		xcb_get_input_focus_reply_t *focusReply = xcb_get_input_focus_reply(
			connection, xcb_get_input_focus(connection), NULL
		);
		window = focusReply->focus;
		free(focusReply);
	} else {
		window = tonumber(str);
	}
	return window;
}

long tonumber(char *str) {
	return strtol(str, NULL, 0);
}

void die(char *message) {
	fprintf(stderr, "%s\n", message);
	if (connection) xcb_disconnect(connection);
	exit(1);
}

void closeWindow(xcb_window_t window) {
	if (WM_PROTOCOLS && WM_DELETE_WINDOW) {
		xcb_client_message_event_t ev;
		
		memset(&ev, 0, sizeof(ev));
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = window;
		ev.format = 32;
		ev.type = WM_PROTOCOLS;
		ev.data.data32[0] = WM_DELETE_WINDOW;
		ev.data.data32[1] = XCB_CURRENT_TIME;
		
		xcb_send_event(connection, 0, window, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
	} else {
		xcb_kill_client(connection, window);
	}
	xcb_flush(connection);
}
