#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"
#include "controls.h"

xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_drawable_t focused;

bool shouldCloseWM = 0;

void closeWM() {
	shouldCloseWM = 1;
}

xcb_atom_t atoms[ATOM_FINAL];

void die(char *errstr) {
	exit(write(STDERR_FILENO, errstr, strlen(errstr)) < 0 ? -1 : 1);
}

void setup();
int eventHandler(void);
void cleanup();

int main(int argc, char *argv[]) {
	DISREGARD(argc);
	DISREGARD(argv);
	setup();
	while (eventHandler());
	cleanup();
}

void setupAtoms();

void setup() {
	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		die("Couldn't connect to X.\n");
	}
	
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	
	xcb_change_window_attributes_checked(
		conn, screen->root, XCB_CW_EVENT_MASK, (uint32_t []) {
			XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_PROPERTY_CHANGE
		}
	);
	
	setupAtoms();
	setupControls();
	setupPages();
	
	xcb_flush(conn);
}

void cleanup() {
	cleanupControls();
	xcb_disconnect(conn);
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
	//commented lines are unused atoms that may be useful later.
	
	//atoms[UTF8_STRING] = xcb_atom_get("UTF8_STRING");
	atoms[WM_PROTOCOLS] = xcb_atom_get("WM_PROTOCOLS");
	atoms[WM_DELETE_WINDOW] = xcb_atom_get("WM_DELETE_WINDOW");
	//atoms[WM_STATE] = xcb_atom_get("WM_STATE");
	//atoms[WM_TAKE_FOCUS] = xcb_atom_get("WM_TAKE_FOCUS");
	//atoms[_NET_ACTIVE_WINDOW] = xcb_atom_get("_NET_ACTIVE_WINDOW");
	//atoms[_NET_SUPPORTED] = xcb_atom_get("_NET_SUPPORTED");
	//atoms[_NET_WM_NAME] = xcb_atom_get("_NET_WM_NAME");
	//atoms[_NET_WM_STATE] = xcb_atom_get("_NET_WM_STATE");
	//atoms[_NET_SUPPORTING_WM_CHECK] = xcb_atom_get("_NET_SUPPORTING_WM_CHECK");
	//atoms[_NET_WM_STATE_FULLSCREEN] = xcb_atom_get("_NET_WM_STATE_FULLSCREEN");
	//atoms[_NET_WM_WINDOW_TYPE] = xcb_atom_get("_NET_WM_WINDOW_TYPE");
	//atoms[_NET_WM_WINDOW_TYPE_DIALOG] = xcb_atom_get("_NET_WM_WINDOW_TYPE_DIALOG");
	//atoms[_NET_CLIENT_LIST] = xcb_atom_get("_NET_CLIENT_LIST");
}

void setBorderColor(xcb_window_t window, uint32_t c) {
	if (BORDER_WIDTH > 0 && screen->root != window && 0 != window) {
		xcb_change_window_attributes(conn, window, XCB_CW_BORDER_PIXEL, (uint32_t[]) {c});
	}
}

void handleEnterNotify(xcb_enter_notify_event_t *event) {
	//when cursor enters window
	xcb_drawable_t window = event->event;
	if (window > 0 && window != screen->root) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
		xcb_configure_window(
			conn, window, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]) {
				XCB_STACK_MODE_ABOVE
			}
		);
		focused = window;
	}
}

void handleDestroyNotify(xcb_destroy_notify_event_t *event) {
	//when a window is destroyed
	if (focused && focused == event->window) focused = 0;
	unmanage(event->window);
}

void handleMappingNotify(xcb_mapping_notify_event_t *event) {
	//when keyboard mapping changes, i think
	DISREGARD(event);
	updateControls();
}

void handleUnmapNotify(xcb_unmap_notify_event_t *event) {
	//when a program chooses to hide its own window
	//desired behaviour is to act pretty much as if it was destroyed
	if (focused && focused == event->window) focused = 0;
	unmanage(event->window);
}

void handleKeyPress(xcb_key_press_event_t *event) {
	//self explanatory
	keybinding(event->state, event->detail);
}

void handleMapRequest(xcb_map_request_event_t *event) {
	//happens when a new window wants to be shown
	struct ClientIndex focusedIndex;
	manage(event->window, managed(focused, &focusedIndex) ? &focusedIndex : NULL);
	
	xcb_configure_window(
		conn, event->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, (uint32_t[]) {
			BORDER_WIDTH
		}
	);
	xcb_change_window_attributes_checked(
		conn, event->window, XCB_CW_EVENT_MASK, (uint32_t[]) {
			XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE
		}
	);
	setBorderColor(event->window, BORDER_COLOR_UNFOCUSED);
	if (!focused) focused = event->window;
}


void handleFocusIn(xcb_focus_in_event_t *event) {
	//self explanatory
	setBorderColor(event->event, BORDER_COLOR_FOCUSED);
}

void handleFocusOut(xcb_focus_out_event_t *event) {
	//self explanatory
	setBorderColor(event->event, BORDER_COLOR_UNFOCUSED);
}

int eventHandler(void) {
	if (shouldCloseWM || xcb_connection_has_error(conn)) return 0;
	
	xcb_generic_event_t *ev = xcb_wait_for_event(conn);
	do {
		switch (ev->response_type & ~0x80) {
			case XCB_ENTER_NOTIFY: handleEnterNotify((xcb_enter_notify_event_t *) ev); break;
			case XCB_DESTROY_NOTIFY: handleDestroyNotify((xcb_destroy_notify_event_t *) ev); break;
			case XCB_MAPPING_NOTIFY: handleMappingNotify((xcb_mapping_notify_event_t *) ev); break;
			case XCB_UNMAP_NOTIFY: handleUnmapNotify((xcb_unmap_notify_event_t *) ev); break;
			
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
