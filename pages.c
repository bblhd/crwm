#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <pages.h>

struct Page pages[9];

size_t screens_max, screens_len;
struct Screen *screens;

void screens_add(xcb_screen_t *screen) {
	screens_len++;
	if (screens_len > screens_max) {
		while (screens_max > screens_len) screens_max++;
		screens = realloc(screens, screens_max);
	}
	
	screens[screens_len-1].root = screen->root;
	screens[screens_len-1].width = screen->width_in_pixels;
	screens[screens_len-1].height = screen->height_in_pixels;
	
	xcb_change_window_attributes_checked(
		conn, screen->root, XCB_CW_EVENT_MASK, (uint32_t []) {
			XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
			| XCB_EVENT_MASK_PROPERTY_CHANGE
		}
	);
}

int screens_addif(xcb_screen_t *screen) {
	for (size_t i = 0; i < screens_len; i++) if (screen->root == screens[i].root) return 0;
	screens_add(screen);
	return 1;
}

void screens_setup(xcb_connection_t *conn) {
	const xcb_setup_t *setup = xcb_get_setup(conn);
	for (xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup); iter.rem; xcb_screen_next(&iter)) {
		screens_addif(iter.data);
	}
}
