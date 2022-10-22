#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <sys/wait.h>
#include <unistd.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "page.h"

void page_init(struct Page *page) {
	page->cols = malloc(sizeof(struct Column));
	page->cols_max = 1;
	page->cols_len = 0;
	
	page->rows = malloc(sizeof(struct Row));
	page->rows_max = 1;
	page->rows_len = 0;
}

void page_free(struct Page *page) {
	free(page->cols);
	free(page->rows);
}

void page_updateColumnsLength(struct Page *page, size_t len) {
	page->cols_len = len;
	if (page->cols_max < page->cols_len) {
		while (page->cols_max < page->cols_len) {
			page->cols_max *= 2;
		}
		page->cols = realloc(page->cols, page->cols_max * sizeof(struct Column));
	}
}

void page_updateRowsLength(struct Page *page, size_t len) {
	page->rows_len = len;
	if (page->rows_max < page->rows_len) {
		while (page->rows_max < page->rows_len) {
			page->rows_max *= 2;
		}
		page->rows = realloc(page->rows, page->rows_max * sizeof(struct Column));
	}
}

void window_update_height(xcb_drawable_t window, uint16_t ry, uint16_t h) {
	xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]) {ry, h-BORDER_WIDTH*2});
}

void window_update_width(xcb_drawable_t window, uint16_t cx, uint16_t w) {
	xcb_configure_window(conn, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {cx, w-BORDER_WIDTH*2});
}

unsigned int udec(unsigned int x) {
	return x > 0 ? x-1 : 0;
}

void page_updateWidths(struct Page *page) {
	if (page->cols_len > 0) {
		uint16_t total_weight = 0;
		for (size_t c = 0; c < page->cols_len; c++) {
			total_weight += page->cols[c].weight;
		}
		
		size_t rr = 0;
		uint16_t cx = 0;
		for (size_t c = 0; c < page->cols_len - 1; c++) {
			const uint16_t width = screen->width_in_pixels * page->cols[c].weight / total_weight;
			for (size_t r = 0; rr < page->rows_len && r < page->cols[c].span; r++, rr++) {
				window_update_width(page->rows[rr].window, cx, width);
			}
			cx+=width;
		}
	
		for (size_t r = 0; rr < page->rows_len && r < page->cols[page->cols_len-1].span; r++, rr++) {
			window_update_width(page->rows[rr].window, cx, screen->width_in_pixels - cx);
		}
	}
}

void page_updateHeights(struct Page *page, size_t c, size_t cr) {
	if (page->cols[c].span > 0) {
		uint16_t total_weight = 0;
		for (size_t rr = cr; rr < cr + page->cols[c].span; rr++) {
			total_weight += page->rows[rr].weight;
		}
		
		uint16_t ry = 0;
		for (size_t rr = cr; rr < cr + page->cols[c].span-1; rr++) {
			const uint16_t height = screen->height_in_pixels * page->rows[rr].weight / total_weight;
			window_update_height(page->rows[rr].window, ry, height);
			ry += height;
		}
		window_update_height(page->rows[cr + page->cols[c].span-1].window, ry, screen->height_in_pixels - ry);
	}
}

void page_addRow(struct Page *page, size_t c, size_t cr, size_t r, xcb_drawable_t window) {
	page_updateRowsLength(page, page->rows_len+1);
	page->cols[c].span += 1;
	for (size_t i = udec(page->rows_len); i > cr+r; i--) {
		page->rows[i] = page->rows[i-1];
	}
	
	page->rows[cr+r].window = window;
	page->rows[cr+r].weight = 1;
}

void page_removeRow(struct Page *page, size_t c, size_t cr, size_t r) {
	for (size_t i = cr+r; i < udec(page->rows_len); i++) {
		page->rows[i] = page->rows[i+1];
	}
	page_updateRowsLength(page, udec(page->rows_len));
	page->cols[c].span -= 1;
}

void page_addColumn(struct Page *page, size_t c) {
	page_updateColumnsLength(page, page->cols_len+1);
	for (size_t i = page->cols_len-1; i > c; i--) {
		page->cols[i] = page->cols[i-1];
	}
	
	page->cols[c].span = 0;
	page->cols[c].weight = 1;
}

void page_removeColumn(struct Page *page, size_t c) {
	for (size_t i = c; i < udec(page->cols_len); i++) {
		page->cols[i] = page->cols[i+1];
	}
	page_updateColumnsLength(page, udec(page->cols_len));
}

void page_insert(struct Page *page, xcb_drawable_t window, uint16_t x, uint16_t y) {
	DISREGARD(y);
	size_t c = 0;
	size_t cr = 0;
	size_t r = 0;
	
	uint16_t total_weight = 0;
	for (size_t i = 0; i < page->cols_len; i++) {
		total_weight += page->cols[i].weight;
	}
	
	uint16_t cx = 0;
	if (page->cols_len > 0) {
		while (c < page->cols_len) {
			const uint16_t width = screen->width_in_pixels * page->cols[c].weight / total_weight;
			if (x <= cx + width) break;
			cr += page->cols[c].span;
			cx += width;
			c++;
		}
	} else {
		page_addColumn(page,0);
		total_weight += page->cols[c].weight;
	}
	
	window_update_width(window, cx, screen->width_in_pixels * page->cols[c].weight / total_weight);
	
	total_weight = 0;
	for (size_t rr = cr; rr < cr + page->cols[c].span; rr++) {
		total_weight += page->rows[rr].weight;
	}
	
	uint16_t ry = 0;
	while (r < page->cols[c].span) {
		const uint16_t height = screen->height_in_pixels * page->rows[cr+r].weight / total_weight;
		if (y <= ry + height) break;
		ry += height;
		r++;
	}
	
	page_addRow(page, c, cr, r, window);
	
	page_updateHeights(page, c, cr);
}

void page_insertThrow(struct Page *page, xcb_drawable_t window) {
	page_insert(page, window, 0, 0);
}

void findWindow(struct Page *page, xcb_drawable_t window, size_t *c, size_t *cr, size_t *r) {
	*c = 0;
	*cr = 0;
	*r = 0;
	while (page->cols[*c].span == 0) {
		(*c)++;
	}
	while (*cr + *r < page->rows_len) {
		if (page->rows[*cr + *r].window == window) return;
		(*r)++;
		while (*r >= page->cols[*c].span) {
			*cr += *r;
			(*c)++;
			*r = 0;
		}
	}
	*c = (size_t) -1;
}

void page_remove(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	page_removeRow(page, c, cr, r);
	page_updateHeights(page, c, cr);
	if (page->cols[c].span == 0) {
		page_removeColumn(page, c);
		page_updateWidths(page);
	}
}

void page_swapY(struct Page *page, size_t rr) {
	uint16_t ay, bh;
	xcb_get_geometry_reply_t *geom;
	
	geom = queryGeometry(page->rows[rr].window);
	ay = geom->y;
	free(geom);
	
	geom = queryGeometry(page->rows[rr+1].window);
	bh = geom->height;
	free(geom);
	
	xcb_configure_window(conn, page->rows[rr].window, XCB_CONFIG_WINDOW_Y, (uint32_t[]) {ay + bh + 2*BORDER_WIDTH});
	xcb_configure_window(conn, page->rows[rr+1].window, XCB_CONFIG_WINDOW_Y, (uint32_t[]) {ay});
	
	struct Row _temp = page->rows[rr];
	page->rows[rr] = page->rows[rr+1];
	page->rows[rr+1] = _temp;
}

void page_moveUp(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (r != 0) {
		page_swapY(page, cr+r-1);
	}
}

void page_moveDown(struct Page *page, xcb_drawable_t window) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (r != page->cols[c].span-1) {
		page_swapY(page, cr+r);
	}
}

void page_moveRight(struct Page *page, xcb_drawable_t window) {
	//todo: refactor to more efficently move windows (replace removeRow-addRow with a shift)
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (c == page->cols_len-1 && page->cols[c].span == 1) return;
	if (c == page->cols_len-1) {
		page_addColumn(page, page->cols_len);
	}
	page_removeRow(page, c, cr, r);
	if (page->cols[c].span == 0) {
		page_removeColumn(page, c);
		c-=1;
		cr-=page->cols[c].span;
	} else {
		page_updateHeights(page, c, cr);
	}
	page_addRow(page, c+1, cr+page->cols[c].span, 0, window);
	page_updateWidths(page);
	page_updateHeights(page, c+1, cr+page->cols[c].span);
}

void page_moveLeft(struct Page *page, xcb_drawable_t window) {
	//todo: refactor to more efficently move windows (replace removeRow-addRow with a shift)
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (c == 0 && page->cols[c].span == 1) return;
	if (c == 0) {
		page_addColumn(page, 0);
		c++;
	}
	page_removeRow(page, c, cr, r);
	if (page->cols[c].span == 0) {
		page_removeColumn(page, c);
	} else {
		page_updateHeights(page, c, cr);
	}
	page_addRow(page, c-1, cr-page->cols[c-1].span, 0, window);
	page_updateWidths(page);
	page_updateHeights(page, c-1, cr+1-page->cols[c-1].span);
}

void page_changeColumnWeight(struct Page *page, xcb_drawable_t window, int amount) {
	if (page->cols_len < 2) return;
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (page->cols[c].weight+amount <= 0) return;
	page->cols[c].weight += amount;
	page_updateWidths(page);
}

void page_changeRowWeight(struct Page *page, xcb_drawable_t window, int amount) {
	size_t c, cr, r;
	findWindow(page, window, &c, &cr, &r);
	if (!~c) return;
	if (page->cols[c].span < 2) return;
	if (page->rows[cr+r].weight+amount <= 0) return;
	page->rows[cr+r].weight += amount;
	page_updateHeights(page, c, cr);
}

void page_map(struct Page *page) {
	for (size_t i = 0; i < page->rows_len; i++) {
		xcb_map_window(conn, page->rows[i].window);
	}
}

void page_unmap(struct Page *page) {
	for (size_t i = 0; i < page->rows_len; i++) {
		xcb_unmap_window(conn, page->rows[i].window);
	}
}

