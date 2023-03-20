#ifndef CRWM_LAYOUT_H
#define CRWM_LAYOUT_H

#include <xcb/xcb.h>

#include <stdint.h>
#include <stdbool.h>

#include "ctheme.h"

typedef struct Instance instance_t;
typedef struct Monitor monitor_t;
typedef struct Table table_t;
typedef struct Column column_t;
typedef struct Row row_t;

struct Instance {
	xcb_connection_t *connection;
	xcb_window_t root;
	struct {
		int16_t vertical, horizontal;
	} padding;
	struct {
		int16_t top, bottom, left, right;
	} margin;
	struct {
		int16_t thickness;
		color_t focused, unfocused;
	} border;
};

struct Monitor {
	table_t *table;
	uint16_t x,y,width,height;
	instance_t *instance;
};

struct Table {
	monitor_t *monitor;
	column_t *columns;
	
	xcb_window_t bar;
};

struct Column {
	column_t *next;
	column_t *previous;
	table_t *table;
	row_t *rows;
	
	uint16_t weight;
};

struct Row {
	row_t *next;
	row_t *previous;
	column_t *column;
	
	xcb_window_t window;
	uint16_t weight;
	uint8_t shown;
	uint8_t hidden;
};

void recalculateTable(table_t *);

void sendTableToMonitor(monitor_t *, table_t *);
void cleanupTable(table_t *);

row_t *newRowAtTable(table_t *, xcb_window_t);
row_t *newRowAtColumn(column_t *, xcb_window_t);
row_t *newRowAtRow(row_t *, xcb_window_t);

void growVertically(row_t *row, int amount);
void growHorizontally(row_t *row, int amount);

void moveRowUp(row_t *row);
void moveRowDown(row_t *row);
void moveRowLeft(row_t *row);
void moveRowRight(row_t *row);

void moveRowToTable(table_t *to, row_t *from);
void moveRowToColumn(column_t *to, row_t *from);
void moveRowToRow(row_t *to, row_t *from);

row_t *findRowInTable(table_t *, xcb_window_t);
void removeRow(row_t *);

bool checkUnmapping(row_t *);
bool checkMapping(row_t *);

#endif
