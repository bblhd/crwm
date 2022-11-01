
#include <stdbool.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

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
}

void screens_update() {
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
}
