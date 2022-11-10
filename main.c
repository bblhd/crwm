
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "pages.h"
#include "main.h"

xcb_connection_t *conn;
struct Row *focusedRow;

enum XcbAtomLabel {
	UTF8_STRING,
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,
	WM_STATE,
	WM_TAKE_FOCUS,
	_NET_ACTIVE_WINDOW,
	_NET_SUPPORTED,
	_NET_WM_NAME,
	_NET_WM_STATE,
	_NET_SUPPORTING_WM_CHECK,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_CLIENT_LIST,
	ATOM_FINAL
};

xcb_atom_t atoms[ATOM_FINAL];

void die(char *errstr) {
	exit(write(STDERR_FILENO, errstr, strlen(errstr)) < 0 ? -1 : 1);
}

char *termcmd[] = { "st", NULL };
void spawn(char **command) {
	if (fork() == 0) {
		setsid();
		if (fork() != 0) _exit(0);
		execvp(command[0], command);
		_exit(0);
	}
	wait(NULL);
}

void setup();
int eventHandler(void);

int main(int argc, char *argv[]) {
	DISREGARD(argc);
	DISREGARD(argv);
	setup();
	while (eventHandler());
	xcb_disconnect(conn);
}

void setupAtoms();

void setup() {
	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		die("Couldn't connect to X.\n");
	}
	screens_setup(conn);
	setupAtoms();
	
}

xcb_atom_t xcb_atom_get(char *name) {
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
		conn,
		xcb_intern_atom(conn, 0, strlen(name), name),
		NULL
	);
	xcb_atom_t atom = reply ? reply->atom : XCB_NONE;
	free(reply);
	return atom;
}

void setupAtoms() {
	atoms[UTF8_STRING] = xcb_atom_get("UTF8_STRING");
	atoms[WM_PROTOCOLS] = xcb_atom_get("WM_PROTOCOLS");
	atoms[WM_DELETE_WINDOW] = xcb_atom_get("WM_DELETE_WINDOW");
	atoms[WM_STATE] = xcb_atom_get("WM_STATE");
	atoms[WM_TAKE_FOCUS] = xcb_atom_get("WM_TAKE_FOCUS");
	atoms[_NET_ACTIVE_WINDOW] = xcb_atom_get("_NET_ACTIVE_WINDOW");
	atoms[_NET_SUPPORTED] = xcb_atom_get("_NET_SUPPORTED");
	atoms[_NET_WM_NAME] = xcb_atom_get("_NET_WM_NAME");
	atoms[_NET_WM_STATE] = xcb_atom_get("_NET_WM_STATE");
	atoms[_NET_SUPPORTING_WM_CHECK] = xcb_atom_get("_NET_SUPPORTING_WM_CHECK");
	atoms[_NET_WM_STATE_FULLSCREEN] = xcb_atom_get("_NET_WM_STATE_FULLSCREEN");
	atoms[_NET_WM_WINDOW_TYPE] = xcb_atom_get("_NET_WM_WINDOW_TYPE");
	atoms[_NET_WM_WINDOW_TYPE_DIALOG] = xcb_atom_get("_NET_WM_WINDOW_TYPE_DIALOG");
	atoms[_NET_CLIENT_LIST] = xcb_atom_get("_NET_CLIENT_LIST");
}

void handleEnterNotify(xcb_enter_notify_event_t *event) {
	//when cursor enters window
	DISREGARD(event);
}

void handleDestroyNotify(xcb_destroy_notify_event_t *event) {
	//when a window is destroyed
	unmanage(event->window);
}

void handleMappingNotify(xcb_mapping_notify_event_t *event) {
	//when keyboard mapping changes, i think
	DISREGARD(event);
}

void handleUnmapNotify(xcb_unmap_notify_event_t *event) {
	//when a program chooses to hide its own window
	//desired behaviour is to act pretty much as if it was destroyed
	unmanage(event->window);
}

void handleButtonPress(xcb_button_press_event_t *event) {
	//mouse press
	DISREGARD(event);
	spawn(termcmd);
}

void handleButtonRelease(xcb_button_release_event_t *event) {
	//mouse release
	DISREGARD(event);
}

void handleMotionNotify(xcb_motion_notify_event_t *event) {
	//mouse movement
	DISREGARD(event);
}

void handleKeyPress(xcb_key_press_event_t *event) {
	//self explanatory
	DISREGARD(event);
}

void handleMapRequest(xcb_map_request_event_t *event) {
	//happens when a new window wants to be shown
	manage(event->window);
}


void handleFocusIn(xcb_focus_in_event_t *event) {
	//self explanatory
	DISREGARD(event);
}

void handleFocusOut(xcb_focus_out_event_t *event) {
	//self explanatory
	DISREGARD(event);
}

int eventHandler(void) {
	if (xcb_connection_has_error(conn)) return 0;
	
	xcb_generic_event_t *ev = xcb_wait_for_event(conn);
	do {
		switch (ev->response_type & ~0x80) {
			case XCB_ENTER_NOTIFY: handleEnterNotify((xcb_enter_notify_event_t *) ev); break;
			case XCB_DESTROY_NOTIFY: handleDestroyNotify((xcb_destroy_notify_event_t *) ev); break;
			case XCB_MAPPING_NOTIFY: handleMappingNotify((xcb_mapping_notify_event_t *) ev); break;
			case XCB_UNMAP_NOTIFY: handleUnmapNotify((xcb_unmap_notify_event_t *) ev); break;
			
			case XCB_MOTION_NOTIFY: handleMotionNotify((xcb_motion_notify_event_t *) ev); break;
			case XCB_BUTTON_PRESS: handleButtonPress((xcb_button_press_event_t *) ev); break;
			case XCB_BUTTON_RELEASE: handleButtonRelease((xcb_button_release_event_t *) ev); break;
			case XCB_KEY_PRESS: handleKeyPress((xcb_key_press_event_t *) ev); break;
			
			case XCB_MAP_REQUEST: handleMapRequest((xcb_map_request_event_t *) ev); break;
			
			case XCB_FOCUS_IN: handleFocusIn((xcb_focus_in_event_t *) ev); break;
			case XCB_FOCUS_OUT: handleFocusOut((xcb_focus_out_event_t *) ev); break;
			default: break;
		}
		free(ev);
		xcb_flush(conn);
	} while ((ev = xcb_poll_for_event(conn))!=NULL); 
	return 1;
}
