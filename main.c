#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"
#include "controls.h"

#include "ctheme.h"

xcb_connection_t *conn;
xcb_screen_t *screen;
xcb_window_t focused;

xcb_gcontext_t graphics;

xcb_atom_t atoms[ATOM_FINAL];

xcb_window_t barWindow = XCB_NONE;
char *barCommand;
int doDrawBar = 0;

color_t barFG;
color_t barBG;

color_t unfocusedBorder;
color_t focusedBorder;

bool shouldCloseWM = 0;

void getCommandlineArguments(char **args, int n);
void setup();
void cleanup();
void eventHandler();
void bar();
int shouldRemainOpen();

int main(int argc, char *argv[]) {
	getCommandlineArguments(argv+1, argc-1);
	setup();
	while (shouldRemainOpen()) eventHandler();
	wait(NULL);
	cleanup();
}

int shouldRemainOpen() {
	return !shouldCloseWM && !xcb_connection_has_error(conn);
}

void die(char *errstr) {
	exit(write(STDERR_FILENO, errstr, strlen(errstr)) < 0 ? -1 : 1);
}

void getCommandlineArguments(char **args, int n) {
	while (n-- > 0) {
		if (strcmp(args[0], "-b")==0) {
			if (n-- > 0) {
				doDrawBar = 1;
				barCommand = args[1];
				args+=2;
			} else die("Invalid command line argument.\n");
		} else {
			die("Invalid command line argument.\n");
		}
	}
}

void setupAtoms();
void setupGraphics();

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
	setupGraphics();
	
	xcb_flush(conn);
}

void cleanupGraphics();

void cleanup() {
	cleanupGraphics();
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
	atoms[WM_PROTOCOLS] = xcb_atom_get("WM_PROTOCOLS");
	atoms[WM_DELETE_WINDOW] = xcb_atom_get("WM_DELETE_WINDOW");
}

void createBarTimer();

void reloadColors() {
	ctheme_clear();
	if (!ctheme_load(NULL)) {
		ctheme_set(COLORSCHEME_DEFAULT, 1, 0xffffff, BGR);
		ctheme_set(COLORSCHEME_DEFAULT, 2, 0x000000, BGR);
		ctheme_set(COLORSCHEME_DEFAULT, 4, 0x9eeeee, BGR);
		ctheme_set(COLORSCHEME_SELECTED, 1, 0x000000, BGR);
		ctheme_set(COLORSCHEME_SELECTED, 2, 0xffffff, BGR);
		ctheme_set(COLORSCHEME_SELECTED, 4, 0x55aaaa, BGR);
	}
	barFG = ctheme_get(COLORSCHEME_DEFAULT, 1, BGR);
	barBG = ctheme_get(COLORSCHEME_DEFAULT, 2, BGR);
	unfocusedBorder = ctheme_get(COLORSCHEME_DEFAULT, 4, BGR);
	focusedBorder = ctheme_get(COLORSCHEME_SELECTED, 4, BGR);
	
	if (doDrawBar) {
		if (barWindow != XCB_NONE) xcb_destroy_window(conn, barWindow);
		barWindow = xcb_generate_id(conn);
		xcb_create_window(
			conn, XCB_COPY_FROM_PARENT,
			barWindow, screen->root,
			0, (DRAW_BAR==1) ? screen->height_in_pixels-BAR_SIZE : 0, screen->width_in_pixels, BAR_SIZE, 0,
			XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
			XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
			(uint32_t[]) {barBG, XCB_EVENT_MASK_EXPOSURE}
		);
		xcb_map_window(conn, barWindow);
	}
	
	xcb_change_gc(conn, graphics, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, (uint32_t[]) {barFG, barBG});
}

void setupGraphics() {
	const char *fontname = "lucidasans-12";
	xcb_font_t font = xcb_generate_id(conn);
	xcb_open_font(conn, font, strlen(fontname), fontname);
	graphics = xcb_generate_id(conn);
	xcb_create_gc(
		conn, graphics, screen->root,
		XCB_GC_FONT | XCB_GC_GRAPHICS_EXPOSURES,
		(uint32_t[]) {font, 1}
	);
	xcb_close_font(conn, font);
	
	reloadColors();
	
	if (doDrawBar) createBarTimer();
}

void cleanupGraphics() {
	xcb_destroy_window(conn, barWindow);
	xcb_free_gc(conn, graphics);
}

void bar() {
	char barMessage[128];
	FILE *pf = popen(barCommand,"r");
	if (fgets(barMessage, 128, pf) == NULL || pclose(pf) != 0) {
		strcpy(barMessage, "Command did not execute correctly");
	}
	xcb_clear_area(conn, 0, barWindow, 0, 0, 0, 0);
	xcb_image_text_8(conn, strlen(barMessage)-1, barWindow, graphics, 0, BAR_SIZE-5, barMessage);
	xcb_flush(conn);
}

timer_t tid;

void createBarTimer() {
	timer_create(CLOCK_REALTIME, &(struct sigevent) {
		.sigev_notify = SIGEV_THREAD,
		.sigev_notify_function = bar,
		.sigev_notify_attributes = NULL,
		.sigev_value.sival_ptr = &tid
	}, &tid);
	
	timer_settime(tid, 0, &(struct itimerspec) {{1, 0}, {1, 0}}, 0);
}

uint16_t getBarHeightReduction() {
	return doDrawBar ? BAR_SIZE : 0;
}

void focus(xcb_window_t window) {
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

void setBorderColor(xcb_window_t window, color_t c) {
	if (BORDER_WIDTH > 0 && screen->root != window && 0 != window) {
		xcb_change_window_attributes(conn, window, XCB_CW_BORDER_PIXEL, (uint32_t[]) {c});
	}
}

bool isMouseWithin(xcb_window_t window) {
	uint16_t x, y;
	uint16_t w, h;
	
	xcb_query_pointer_reply_t *pointer = xcb_query_pointer_reply(conn, xcb_query_pointer(conn, window), NULL);
	x = pointer->win_x;
	y = pointer->win_y;
	free(pointer);
	
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, window), NULL);
	w = geom->width;
	h = geom->height;
	free(geom);
	
	return x>0 && x<w && y>0 && y<h;
}

void handleEnterNotify(xcb_enter_notify_event_t *event) {
	//when cursor enters window
	focus(event->event);
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
	setBorderColor(event->window, unfocusedBorder);
	if (isMouseWithin(event->window)) focus(event->window);
}


void handleFocusIn(xcb_focus_in_event_t *event) {
	//self explanatory
	setBorderColor(event->event, focusedBorder);
}

void handleFocusOut(xcb_focus_out_event_t *event) {
	//self explanatory
	setBorderColor(event->event, unfocusedBorder);
}

void eventHandler() {
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
}

void closeWM() {
	shouldCloseWM = 1;
}
