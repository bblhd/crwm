
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_keysyms.h>

#include "main.h"
#include "pages.h"

//there is a lot of code duplication atm, need to fix

#define PAGE_COUNT 9
struct Page pages[PAGE_COUNT];
int mapped;

void decrementClientIndexColumn(struct ClientIndex *index);
void incrementClientIndexColumn(struct ClientIndex *index);
bool clientIterMarkColumns(struct ClientIndex *index);
 
void swapRowWithAdjacent(struct ClientIndex *row);
void updateWidthForSingleRow(struct ClientIndex *row);
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
	for (int p = 0; p < PAGE_COUNT; p++) {
		pages[p] = (struct Page) {
			.columns=malloc(sizeof(struct Column)),
			.columnsMax=1, .columnsLength=0,
			.rows=malloc(sizeof(struct Row)),
			.rowsMax=1, .rowsLength=0
		};
	}
}

void cleanupPages() {
	for (int p = 0; p < PAGE_COUNT; p++) {
		if (pages[p].columns) free(pages[p].columns);
		if(pages[p].rows) free(pages[p].rows);
	}
}

bool managed(xcb_window_t window, struct ClientIndex *index) {
	struct ClientIndex _index;
	if (index == NULL) index = &_index;
	
	for (int p = 0; p < PAGE_COUNT; p++) {
		*index = (struct ClientIndex) {p, 0, 0, 0};
		while (checkRow(index) && getRow(index)->window != window) {
			clientIterMarkColumns(index);
		}
		if (checkRow(index)) return TRUE;
	}
	return FALSE;
}

void manage(xcb_window_t window, struct ClientIndex *index) {
	struct ClientIndex _index;
	if (index == NULL) index = &_index;
	if (index->p != mapped) *index = (struct ClientIndex) {mapped, 0, 0, 0};
	
	if (!managed(window, NULL)) {
		xcb_map_window(conn, window);
		
		if (getPage(index)->columnsLength == 0) {
			insertColumn(index);
		}
		insertRow(index);
		
		getRow(index)->window = window;
		
		updateWidthForSingleRow(index);
		updateHeights(index);
	}
}

void unmanage(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		if (getRow(&client)->dontUnmanageYet) {
			getRow(&client)->dontUnmanageYet = FALSE;
			return;
		}
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

void sendPage(xcb_window_t window, uint16_t p) {
	if (p >= PAGE_COUNT) return;
	struct ClientIndex client;
	if (managed(window, &client)) {
		if (p == client.p) return;
		if (client.p == mapped) {
			getRow(&client)->dontUnmanageYet = TRUE;
			xcb_unmap_window(conn, window);
		}
		
		struct Row _temp = *getRow(&client);
		
		if (getColumn(&client)->length == 1) {
			removeColumn(&client);
			client = (struct ClientIndex) {client.p, 0, 0, 0};
			updateWidths(&client);
		} else {
			removeRow(&client);
			client.r = 0;
			updateHeights(&client);
		}
		
		client = (struct ClientIndex) {p, 0, 0, 0};
		if (getPage(&client)->columnsLength == 0) {
			insertColumn(&client);
		}
		insertRow(&client);
		*getRow(&client) = _temp;
		
		
		updateWidthForSingleRow(&client);
		updateHeights(&client);
	}
}

void switchPage(uint16_t p) {
	if (p >= PAGE_COUNT) return;
	if (p == mapped) return;
	for (int rr = 0; rr < pages[mapped].rowsLength; rr++) {
		pages[mapped].rows[rr].dontUnmanageYet = TRUE;
		xcb_unmap_window(conn, pages[mapped].rows[rr].window);
	}
	mapped = p;
	for (int rr = 0; rr < pages[mapped].rowsLength; rr++) {
		xcb_map_window(conn, pages[mapped].rows[rr].window);
	}
}

void changeWeights(xcb_window_t window, int16_t x, int16_t y) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		if (x != 0) {
			getColumn(&client)->weight += x;
			if (getColumn(&client)->weight < 1) getColumn(&client)->weight = 1;
			updateWidths(&(struct ClientIndex) {.p=client.p, .c=0, .cr=0, .r=0});
		}
		if (y != 0) {
			getRow(&client)->weight += y;
			if (getRow(&client)->weight < 1) getRow(&client)->weight = 1;
			updateHeights(&(struct ClientIndex) {.p=client.p, .c=client.c, .cr=client.cr, .r=0});
		}
	}
}

xcb_window_t lookUp(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		client.r--;
		if (checkRow(&client)) return getRow(&client)->window;
	}
	return window;
}

xcb_window_t lookDown(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		client.r++;
		if (checkRow(&client)) return getRow(&client)->window;
	}
	return window;
}

xcb_window_t lookRight(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		incrementClientIndexColumn(&client);
		if (checkRow(&client)) return getRow(&client)->window;
	}
	return window;
}

xcb_window_t lookLeft(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		decrementClientIndexColumn(&client);
		if (checkRow(&client)) return getRow(&client)->window;
	}
	return window;
}

void moveUp(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		if (client.r != 0) {
			client.r--;
			swapRowWithAdjacent(&client);
		}
	}
}

void moveDown(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		if (client.r != getColumn(&client)->length-1) {
			swapRowWithAdjacent(&client);
		}
	}
}

void moveLeft(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		bool isLeftSide = (client.c == 0);
		bool isOnlyChild = pages[client.p].columns[client.c].length <= 1;
		
		if (isLeftSide && isOnlyChild) return;
		
		struct Row _temp = *getRow(&client);
		
		if (isOnlyChild) {
			removeColumn(&client);
		} else {
			removeRow(&client);
			updateHeights(&client);
		}
		decrementClientIndexColumn(&client);
		client.r = 0;
		
		if (isLeftSide) insertColumn(&client);
		
		insertRow(&client);
		*getRow(&client) = _temp;
		
		updateHeights(&client);
		if (isLeftSide || isOnlyChild) updateWidths(&client);
		else updateWidthForSingleRow(&client);
	}
}

void moveRight(xcb_window_t window) {
	struct ClientIndex client;
	if (managed(window, &client)) {
		bool isRightSide = (client.c == pages[client.p].columnsLength-1);
		bool isOnlyChild = pages[client.p].columns[client.c].length <= 1;
		
		if (isRightSide && isOnlyChild) return;
		
		struct Row _temp = *getRow(&client);
		
		if (isOnlyChild) {
			removeColumn(&client);
		} else {
			removeRow(&client);
			updateHeights(&client);
			incrementClientIndexColumn(&client);
		}
		client.r = 0;
		
		if (isRightSide) insertColumn(&client);
		
		insertRow(&client);
		*getRow(&client) = _temp;
		
		updateHeights(&client);
		if (isRightSide || isOnlyChild) updateWidths(&client);
		else updateWidthForSingleRow(&client);
	}
}

void insertColumn(struct ClientIndex *index) {
	if (!checkPage(index)) return;
	
	getPage(index)->columnsLength++;
	if (getPage(index)->columnsLength > getPage(index)->columnsMax) {
		getPage(index)->columnsMax *= 2;
		getPage(index)->columns = realloc(getPage(index)->columns, getPage(index)->columnsMax * sizeof(struct Column));
		if (getPage(index)->columns == NULL) {
			printf("AHHAHAHAHH! %u\n", getPage(index)->columnsMax);
			exit(1);
		}
	}
	
	for (int c = getPage(index)->columnsLength - 2; c >= index->c; c--) {
		pages[index->p].columns[c + 1] = pages[index->p].columns[c];
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
	getRow(index)->dontUnmanageYet = FALSE;
}

void removeColumn(struct ClientIndex *index) {
	if (!checkColumn(index)) return;
	
	uint16_t rowShift = getColumn(index)->length;
	
	if (rowShift > 0) {
		for (int rr = index->cr; rr <= getPage(index)->rowsLength - 1 - rowShift; rr++) {
			getPage(index)->rows[rr] = getPage(index)->rows[rr + rowShift];
		}
		getPage(index)->rowsLength -= rowShift;
	}
	
	for (int c = index->c; c <= getPage(index)->columnsLength - 2; c++) {
		getPage(index)->columns[c] = getPage(index)->columns[c + 1];
	}
	getPage(index)->columnsLength--;
}

void removeRow(struct ClientIndex *index) {
	if (!checkRow(index)) return;
	
	for (int rr = index->cr+index->r; rr <= getPage(index)->rowsLength - 2; rr++) {
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

void swapRowWithAdjacent(struct ClientIndex *index) {
	struct Row *row = pages[index->p].rows + (index->cr + index->r);
	uint16_t ay, bh;
	xcb_get_geometry_reply_t *geom;
	
	geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, row[0].window), NULL);
	ay = geom->y;
	free(geom);
	
	geom = xcb_get_geometry_reply(conn, xcb_get_geometry(conn, row[1].window), NULL);
	bh = geom->height;
	free(geom);
	
	xcb_configure_window(conn, row[0].window, XCB_CONFIG_WINDOW_Y, (uint32_t[]) {ay + bh + 2*BORDER_WIDTH});
	xcb_configure_window(conn, row[1].window, XCB_CONFIG_WINDOW_Y, (uint32_t[]) {ay});
	
	struct Row _temp = row[0];
	row[0] = row[1];
	row[1] = _temp;
}

void updateWidthForSingleRow(struct ClientIndex *index) {
	if (!checkRow(index)) return;
	
	if (getPage(index)->columnsLength == 0) return;
	
	uint16_t totalWeight = 0;
	for (int c = 0; c < getPage(index)->columnsLength; c++) {
		totalWeight += pages[index->p].columns[c].weight;
	}
	
	struct ClientIndex i = {.p=index->p, .c=0, .cr=0, .r=0};
	uint16_t cx = 0, w = (uint16_t) ((float) screen->width_in_pixels * getColumn(&i)->weight / totalWeight);
	while (checkRow(&i) && (i.c != index->c || i.r != index->r)) {
		if (clientIterMarkColumns(&i)) {
			cx += w;
			w = (uint16_t) ((float) screen->width_in_pixels * getColumn(&i)->weight / totalWeight);
		}
	}
	if (checkRow(&i)) {
		xcb_configure_window(conn, getRow(&i)->window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH, (uint32_t[]) {
			cx, w-BORDER_WIDTH*2
		});
	}
	xcb_flush(conn);
}

void updateWidths(struct ClientIndex *index) {
	if (!checkPage(index)) return;
	
	if (getPage(index)->columnsLength == 0) return;
	
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
		
		if (clientIterMarkColumns(&i)) {
			cx += w;
			w = (uint16_t) ((float) screen->width_in_pixels * getColumn(&i)->weight / totalWeight);
			if (i.c == getPage(index)->columnsLength-1) w = screen->width_in_pixels - cx;
		}
	}
	xcb_flush(conn);
}

void updateHeights(struct ClientIndex *index) {
	if (!checkColumn(index)) return;
	
	if (getColumn(index)->length == 0) return;
	
	uint16_t totalWeight = 0;
	for (int r = 0; r < getColumn(index)->length; r++) {
		totalWeight += pages[index->p].rows[index->cr + r].weight;
	}
	
	struct ClientIndex i = {.p=index->p, .c=index->c, .cr=index->cr, .r=0};
	uint16_t ry = 0;
	while (checkRow(&i)) {
		uint16_t h = (uint16_t) ((float) screen->height_in_pixels * getRow(&i)->weight / totalWeight);
		if (i.r == getColumn(&i)->length-1) h = screen->height_in_pixels - ry;
		
		xcb_configure_window(conn, getRow(&i)->window, XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT, (uint32_t[]) {
			ry, h-BORDER_WIDTH*2
		});
		
		ry += h;
		
		if (clientIterMarkColumns(&i)) break;
	}
	xcb_flush(conn);
}

void decrementClientIndexColumn(struct ClientIndex *index) {
	index->r=0;
	if (index->c > 0) {
		index->c--;
		if (checkColumn(index)) {
			index->cr -= getColumn(index)->length;
		}
	}
}
void incrementClientIndexColumn(struct ClientIndex *index) {
	if (!checkPage(index)) return;
	index->r=0;
	if (index->c < getPage(index)->columnsLength) {
		index->cr += getColumn(index)->length;
		index->c++;
	}
}

bool clientIterMarkColumns(struct ClientIndex *index) {
	index->r++;
	if (index->r == getColumn(index)->length) {
		index->cr += index->r;
		index->r = 0;
		index->c++;
		return checkRow(index);
	}
	return FALSE;
}
