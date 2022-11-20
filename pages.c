
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"

#define PAGE_COUNT 9
struct Page pages[PAGE_COUNT];
int mapped;
 
void updateWidths(struct ClientIndex *page);
void updateHeights(struct ClientIndex *column);

static inline bool checkPage(struct ClientIndex *index);
static inline bool checkColumn(struct ClientIndex *index);
static inline bool checkRow(struct ClientIndex *index);
static inline struct Row *getRow(struct ClientIndex *index);
static inline struct Column *getColumn(struct ClientIndex *index);
static inline struct Page *getPage(struct ClientIndex *index);

void insertColumn(struct ClientIndex *index);
void insertRow(struct ClientIndex *index);
void removeColumn(struct ClientIndex *index);
void removeRow(struct ClientIndex *index);

void setupPages() {
	
}

void cleanupPages() {
	for (int p = 0; p < PAGE_COUNT; p++) {
		free(pages[p].columns);
		free(pages[p].rows);
	}
}

bool clientIterMarkColumns(struct ClientIndex *index) {
	index->r++;
	if (index->r == getColumn(index)->length) {
		index->cr += index->r;
		index->r = 0;
		return checkRow(index);
	}
	return FALSE;
}

bool managed(xcb_drawable_t window, struct ClientIndex *index) {
	struct ClientIndex _index;
	if (index == NULL) index = &_index;
	
	for (int p = 0; p < PAGE_COUNT; p++) {
		index->p = p;
		index->c = 0;
		index->cr = 0;
		index->r = 0;
		while (checkRow(index) && getRow(index)->window != window) {
			clientIterMarkColumns(index);
		}
		if (checkRow(index)) return TRUE;
	}
	return FALSE;
}

void manage(xcb_drawable_t window, struct ClientIndex *index) {
	struct ClientIndex _index;
	if (index == NULL) index = &_index;
	
	if (!managed(window, NULL)) {
		index->p = mapped;
		index->c = 0;
		index->cr = 0;
		index->r = 0;
		
		if (getPage(index)->columnsLength == 0) {
			insertColumn(index);
		}
		insertRow(index);
		
		getRow(index)->window = window;
		
		updateWidths(index);
		updateHeights(index);
	}
}

void unmanage(xcb_drawable_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		if (getColumn(&client)->length == 1) {
			removeColumn(&client);
			client.c = 0;
			client.cr = 0;
			client.r = 0;
			updateWidths(&client);
		} else {
			removeRow(&client);
			client.r = 0;
			updateHeights(&client);
		}
	}
}

void moveUp(xcb_drawable_t window) {
	(void) window;
}

void moveDown(xcb_drawable_t window) {
	(void) window;
}

void moveLeft(xcb_drawable_t window) {
	(void) window;
}

void moveRight(xcb_drawable_t window) {
	(void) window;
}

void updateWidths(struct ClientIndex *index) {
	if (!checkPage(index)) return;
	
	uint16_t totalWeight = 0;
	for (int c = 0; c < getPage(index)->columnsLength; c++) {
		totalWeight += pages[index->p].columns[c].weight;
	}
	
	struct ClientIndex i = {.p=index->p, .c=0, .cr=0, .r=0};
	uint16_t cx = 0, w = (uint16_t) ((float) screen->width_in_pixels * getColumn(&i)->weight / totalWeight);
	while (checkRow(&i)) {
		xcb_configure_window(conn, getRow(&i)->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {
			cx, w-BORDER_WIDTH*2
		});
		
		if (clientIterMarkColumns(index)) {
			cx += w;
			w = (uint16_t) ((float) screen->width_in_pixels * getColumn(&i)->weight / totalWeight);
		}
	}
}

void updateHeights(struct ClientIndex *index) {
	if (!checkColumn(index)) return;
	
	uint16_t totalWeight = 0;
	for (int r = 0; r < getColumn(index)->length; r++) {
		totalWeight += pages[index->p].rows[index->cr + index->r].weight;
	}
	
	struct ClientIndex i = {.p=index->p, .c=index->c, .cr=index->cr, .r=0};
	uint16_t ry = 0, h = (uint16_t) ((float) screen->height_in_pixels * getRow(&i)->weight / totalWeight);
	while (checkRow(&i)) {
		xcb_configure_window(conn, getRow(&i)->window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]) {
			ry, h-BORDER_WIDTH*2
		});
		
		ry += h;
		h = (uint16_t) ((float) screen->height_in_pixels * getRow(&i)->weight / totalWeight);
		
		if (clientIterMarkColumns(index)) break;
	}
}

void insertColumn(struct ClientIndex *index) {
	if (!checkPage(index)) return;
	
	getPage(index)->columnsLength++;
	if (getPage(index)->columnsLength > getPage(index)->columnsMax) {
		getPage(index)->columnsMax *= 2;
		getPage(index)->columns = realloc(getPage(index)->columns, getPage(index)->columnsMax * sizeof(struct Column));
	}
	
	for (int c = getPage(index)->columnsLength - 2; c >= index->c; c--) {
		getPage(index)->columns[c + 1] = getPage(index)->columns[c];
	}
	
	getColumn(index)->weight = 1;
	getColumn(index)->length = 0;
}

void insertRow(struct ClientIndex *index) {
	if (!checkColumn(index)) return;
	
	getPage(index)->rowsLength++;
	if (getPage(index)->rowsLength > getPage(index)->rowsMax) {
		getPage(index)->rowsMax += 2;
		getPage(index)->rows = realloc(getPage(index)->rows, getPage(index)->rowsMax * sizeof(struct Row));
	}
	
	for (int rr = getPage(index)->rowsLength - 2; rr >= index->cr + index->r; rr--) {
		getPage(index)->rows[rr + 1] = getPage(index)->rows[rr];
	}
	
	getColumn(index)->length++;
	
	getRow(index)->weight = 1;
	getRow(index)->window = 0;
}

void removeColumn(struct ClientIndex *index) {
	if (!checkColumn(index)) return;
	
	uint16_t rowShift = getColumn(index)->length;
	
	for (int rr = index->cr; rr >= getPage(index)->rowsLength - 1 - rowShift; rr++) {
		getPage(index)->rows[rr] = getPage(index)->rows[rr + rowShift];
	}
	getPage(index)->rowsLength -= rowShift;
	
	for (int c = index->c; c >= getPage(index)->columnsLength - 2; c++) {
		getColumn(index)->length -= rowShift;
		getPage(index)->columns[c] = getPage(index)->columns[c + 1];
	}
	getPage(index)->columnsLength--;
}

void removeRow(struct ClientIndex *index) {
	if (!checkRow(index)) return;
	
	for (int rr = index->cr; rr >= getPage(index)->rowsLength - 2; rr++) {
		getPage(index)->rows[rr] = getPage(index)->rows[rr + 1];
	}
	getPage(index)->rowsLength--;
	
	getColumn(index)->length--;
}


inline bool checkPage(struct ClientIndex *index) {
	return index->p < PAGE_COUNT;
}

inline bool checkColumn(struct ClientIndex *index) {
	return checkPage(index) && index->c < pages[index->p].columnsLength;
}
inline bool checkRow(struct ClientIndex *index) {
	return checkColumn(index) && index->r < pages[index->p].columns[index->c].length;
}

inline struct Row *getRow(struct ClientIndex *index) {
	return pages[index->p].rows+(index->cr + index->r);
}

inline struct Column *getColumn(struct ClientIndex *index) {
	return pages[index->p].columns+(index->c);
}

inline struct Page *getPage(struct ClientIndex *index) {
	return pages+(index->p);
}
