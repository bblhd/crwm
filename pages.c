#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include <pages.h>

#define PAGE_COUNT 9
struct Page pages[PAGE_COUNT];

size_t screens_max, screens_len;
struct Screen *screens;

struct Screen *screens_getFromRoot(xcb_drawable_t root) {
	for (size_t i = 0; i < screens_len; i++) if (root == screens[i].root) return screens + i;
	return NULL;
}

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
	if (screens_getFromRoot(screen->root)) return 0;
	else screens_add(screen);
	return 1;
}

void screens_setup(xcb_connection_t *conn) {
	for (xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(conn)); iter.rem; xcb_screen_next(&iter)) {
		screens_addif(iter.data);
	}
}


void pages_manage(xcb_drawable_t window) {
	for (int p = 0; p < PAGE_COUNT; p++) {
		for (int p = 0; p < PAGE_COUNT; p++) {
		for (int p = 0; p < PAGE_COUNT; p++) {
		
	}
	}
	}
}

void pages_unmanage(xcb_drawable_t window) {
	
}
