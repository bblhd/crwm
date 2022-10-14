#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "main.h"
#include "page.h"
#include "controls.h"

#define DISREGARD(x) ((void)(x))

xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_drawable_t focusedWindow;

int streq(char *a, char *b) {
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return *a == *b;
}
size_t strlen(const char *s) {
	size_t n = 0;
	while (*s++) n++;
	return n;
}

int die(char *errstr) {
	exit(write(STDERR_FILENO, errstr, strlen(errstr)) < 0 ? -1 : 1);
}

xcb_query_pointer_reply_t *queryPointer(xcb_drawable_t window) {
	return xcb_query_pointer_reply(conn, xcb_query_pointer(conn, window), NULL); 	
}
xcb_get_geometry_reply_t *queryGeometry(xcb_drawable_t window) {
	return xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), NULL);
}

xcb_keycode_t *xcb_get_keycodes(xcb_keysym_t keysym) {
	xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(conn);
	xcb_keycode_t *keycode;
	keycode = (!keysyms ? NULL : xcb_key_symbols_get_keycode(keysyms, keysym));
	xcb_key_symbols_free(keysyms);
	return keycode;
}

xcb_keysym_t xcb_get_keysym(xcb_keycode_t keycode) {
	xcb_key_symbols_t *keysyms = xcb_key_symbols_alloc(conn);
	xcb_keysym_t keysym;
	keysym = (!keysyms ? 0 : xcb_key_symbols_get_keysym(keysyms, keycode, 0));
	xcb_key_symbols_free(keysyms);
	return keysym;
}

struct Page testpage;

void setup(void);
int eventHandler(void);

int main(int argc, char *argv[]) {
	setup();
	
	page_init(&testpage);
	page_map(&testpage);
	
	while (eventHandler());
	
	page_free(&testpage);
	
	return 0;
}

void setup(void) {
	conn = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(conn)) {
		die("xcb connection error\n");
	}
	
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
	
	xcb_change_window_attributes_checked(conn, screen->root, XCB_CW_EVENT_MASK, (uint32_t[]) {
		XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		| XCB_EVENT_MASK_STRUCTURE_NOTIFY
		| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
		| XCB_EVENT_MASK_PROPERTY_CHANGE
	});
	
	//grab controls
	
	xcb_flush(conn);
	
	xcb_ungrab_key(conn, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);
	
	xcb_flush(conn);
	
	for (int i = 0; keys[i].func != NULL; i++) {
		xcb_keycode_t *keycode = xcb_get_keycodes(keys[i].keysym);
		if (keycode != NULL) {
			xcb_grab_key(
				conn, 1, screen->root,
				keys[i].mod, *keycode,
				XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC
			);
		}
	}
	
	xcb_flush(conn);
	
	xcb_grab_button(
		conn, 0, screen->root,
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
		XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE,
		XCB_BUTTON_INDEX_1, MOD1
	);
	
	xcb_grab_button(
		conn, 0, screen->root,
		XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
		XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, screen->root, XCB_NONE,
		XCB_BUTTON_INDEX_3, MOD1
	);
	
	xcb_flush(conn);
}

void handleEnterNotify(xcb_enter_notify_event_t *);
void handleDestroyNotify(xcb_destroy_notify_event_t *);
void handleButtonPress(xcb_button_press_event_t *);
void handleButtonRelease(xcb_button_release_event_t *);
void handleMotionNotify(xcb_motion_notify_event_t *);
void handleKeyPress(xcb_key_press_event_t *);
void handleMapRequest(xcb_map_request_event_t *);
void handleFocusIn(xcb_focus_in_event_t *);
void handleFocusOut(xcb_focus_out_event_t *);

int eventHandler(void) {
	if (xcb_connection_has_error(conn)) return 0;
	xcb_generic_event_t *ev = xcb_wait_for_event(conn);
	
	if (ev != NULL) {
		switch (ev->response_type & ~0x80) {
			case XCB_ENTER_NOTIFY: handleEnterNotify((xcb_enter_notify_event_t *) ev); break;
			case XCB_DESTROY_NOTIFY: handleDestroyNotify((xcb_destroy_notify_event_t  *) ev); break;
			case XCB_BUTTON_PRESS: handleButtonPress((xcb_button_press_event_t *) ev); break;
			case XCB_BUTTON_RELEASE: handleButtonRelease((xcb_button_release_event_t *) ev); break;
			case XCB_MOTION_NOTIFY: handleMotionNotify((xcb_motion_notify_event_t *) ev); break;
			case XCB_KEY_PRESS: handleKeyPress((xcb_key_press_event_t *) ev); break;
			case XCB_MAP_REQUEST: handleMapRequest((xcb_map_request_event_t *) ev); break;
			case XCB_FOCUS_IN: handleFocusIn((xcb_focus_in_event_t *) ev); break;
			case XCB_FOCUS_OUT: handleFocusOut((xcb_focus_out_event_t *) ev); break;
			default: break;
		}
		free(ev);
		xcb_flush(conn);
	}
	return 1;
}

void setFocus(xcb_drawable_t window) {
	if (window > 0 && window != screen->root) {
		xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
		xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]) {XCB_STACK_MODE_ABOVE});
		focusedWindow = window;
	}
}

void setFocusColor(xcb_window_t window, uint32_t c) {
	if (BORDER_WIDTH > 0 && screen->root != window && 0 != window) {
		xcb_change_window_attributes(conn, window, XCB_CW_BORDER_PIXEL, (uint32_t[]) {c});
	}
}

void keybinding(uint16_t mod, xcb_keysym_t keysym) {
	for (int i = 0; keys[i].func != NULL; i++) {
		if (mod == keys[i].mod && keysym == keys[i].keysym) {
			keys[i].func();
		}
	}
}

void handleEnterNotify(xcb_enter_notify_event_t *event) {
	setFocus(event->event);
}

void handleDestroyNotify(xcb_destroy_notify_event_t *event) {
	if (focusedWindow == event->window) focusedWindow = 0;
	page_remove(&testpage, event->window);
	xcb_kill_client(conn, event->window);
}

void handleButtonPress(xcb_button_press_event_t *e) {
	DISREGARD(e);
}

void handleButtonRelease(xcb_button_release_event_t *e) {
	DISREGARD(e);
}

void handleMotionNotify(xcb_motion_notify_event_t *e) {
	DISREGARD(e);
}

void handleKeyPress(xcb_key_press_event_t *event) {
	keybinding(event->state, xcb_get_keysym(event->detail));
}

void handleMapRequest(xcb_map_request_event_t *event) {
	xcb_map_window(conn, event->window);
	xcb_flush(conn);
	xcb_configure_window(conn, event->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, (uint32_t[]) {BORDER_WIDTH});
	xcb_change_window_attributes_checked(conn, event->window, XCB_CW_EVENT_MASK, (uint32_t[]) {
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE
	});
	setFocus(event->window);
	xcb_flush(conn);
	
	xcb_query_pointer_reply_t *pointer = queryPointer(screen->root);
	page_insert(&testpage, event->window, pointer->root_x, pointer->root_y);
	free(pointer);
}

void handleFocusIn(xcb_focus_in_event_t *event) {
	setFocusColor(event->event, BORDER_COLOR_FOCUSED);
}

void handleFocusOut(xcb_focus_out_event_t *event) {
	setFocusColor(event->event, BORDER_COLOR_UNFOCUSED);
}
