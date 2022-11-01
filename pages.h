
#include <stdint.h>
#include <stdbool.h>

struct Screen {
	uint16_t width, height;
	xcb_drawable_t root;
};

struct Page {
	struct Screen *mapped;
	struct Column *columns;
	uint16_t max, len;
};

struct Column {
	struct Page *parent;
	struct Row *rows;
	uint16_t max, len;
	uint16_t weight
};

struct Row {
	struct Column *parent;
	xcb_drawable_t window;
	uint16_t weight;
};
