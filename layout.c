#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

#include <xcb/xcb.h>

#include "layout.h"

void cleanupColumns(column_t *columns);
void cleanupRows(row_t *rows);

column_t *createColumn();
row_t *createRow(xcb_window_t);

void connectColumn(column_t *column, column_t *next, column_t *previous);
void connectRow(row_t *row, row_t *next, row_t *previous);

void disconnectColumn(column_t *column);
void disconnectRow(row_t *row);

void recalculateTable(table_t *table);
void hideTable(table_t *table);
void showTable(table_t *table);
void hideRow(row_t *row);

void sendTableToMonitor(monitor_t *monitor, table_t *table) {
	if (table->monitor == monitor) return;
	
	monitor_t *otherMonitor = table->monitor;
	table_t *otherTable = monitor->table;
		
	monitor->table = table;
	table->monitor = monitor;
	
	if (!otherMonitor) showTable(table);
	
	recalculateTable(table);
	
	if (otherMonitor) otherMonitor->table = otherTable;
	else if (otherTable) hideTable(otherTable);
	if (otherTable) {
		otherTable->monitor = otherMonitor;
		recalculateTable(otherTable);
	}
}

void cleanupTable(table_t *table) {
	cleanupColumns(table->columns);
}

void cleanupColumns(column_t *column) {
	while (column) {
		column_t *next = column->next;
		cleanupRows(column->rows);
		free(column);
		column = next;
	}
}

void cleanupRows(row_t *row) {
	while (row) {
		row_t *next = row->next;
		free(row);
		row = next;
	}
}

row_t *newRowAtTable(table_t *table, xcb_window_t window) {
	if (table->columns == NULL) {
		table->columns = createColumn();
		table->columns->table = table;
	}
	return newRowAtColumn(table->columns, window);
}

row_t *newRowAtColumn(column_t *column, xcb_window_t window) {
	row_t *row = createRow(window);
	row->column = column;
	connectRow(row, column->rows, NULL);
	column->rows = row;
	recalculateTable(column->table);
	if (column->table->monitor) {
		xcb_map_window(column->table->monitor->instance->connection, window);
	}
	return column->rows;
}

row_t *newRowAtRow(row_t *row, xcb_window_t window) {
	row_t *new = createRow(window);
	new->column = row->column;
	connectRow(new, row->next, row);
	row->next = new;
	recalculateTable(row->column->table);
	if (row->column->table->monitor) {
		xcb_map_window(row->column->table->monitor->instance->connection, window);
	}
	return row->next;
}

void growVertically(row_t *row, int amount) {
	if (amount < 0 && row->weight < (uint16_t) -amount) amount = -row->weight;
	row->weight += amount;
	recalculateTable(row->column->table);
}

void growHorizontally(row_t *row, int amount) {
	if (amount < 0 && row->column->weight < (uint16_t) -amount) amount = -row->column->weight;
	row->column->weight += amount;
	recalculateTable(row->column->table);
}

void moveRowDown(row_t *row) {
	if (row->next) {
		moveRowToRow(row->next, row);
	}
}

void moveRowUp(row_t *row) {
	if (row->previous) {
		if (row->previous->previous) {
			moveRowToRow(row->previous->previous, row);
		} else {
			moveRowToColumn(row->column, row);
		}
	}
}

void moveRowLeft(row_t *row) {
	if (!row->column->previous) {
		if (!row->previous && !row->next) return;
		column_t *new = createColumn();
		new->table = row->column->table;
		connectColumn(new, row->column, NULL);
	}
	moveRowToColumn(row->column->previous, row);
}

void moveRowRight(row_t *row) {
	if (!row->column->next) {
		if (!row->previous && !row->next) return;
		column_t *new = createColumn();
		new->table = row->column->table;
		connectColumn(new, NULL, row->column);
	}
	moveRowToColumn(row->column->next, row);
}

void moveRowToTable(table_t *table, row_t *row) {
	if (row->column->table == table) return;
	if (table->columns == NULL) {
		table->columns = createColumn(table);
		table->columns->table = table;
	}
	moveRowToColumn(table->columns, row);
}

void moveRowToColumn(column_t *column, row_t *row) {
	if (row->previous == NULL && row->column == column) return;
	table_t *oldTable = row->column->table;
	if (oldTable->monitor && !column->table->monitor) {
		hideRow(row);
	}
	disconnectRow(row);
	row->column = column;
	connectRow(row, column->rows, NULL);
	recalculateTable(column->table);
	if (oldTable != column->table) recalculateTable(oldTable);
}

void moveRowToRow(row_t *to, row_t *from) {
	if (to->next == from) return;
	table_t *oldTable = from->column->table;
	if (oldTable->monitor && !to->column->table->monitor) {
		hideRow(from);
	}
	disconnectRow(from);
	from->column = to->column;
	connectRow(from, to->next, to);
	recalculateTable(to->column->table);
	if (oldTable != from->column->table) recalculateTable(oldTable);
}

void removeRow(row_t *row) {
	table_t *table = row->column->table;
	disconnectRow(row);
	free(row);
	recalculateTable(table);
}

bool checkUnmapping(row_t *row) {
	if (row->hidden > 0) {
		row->hidden--;
		return true;
	}
	return false;
}

bool checkMapping(row_t *row) {
	if (row->shown > 0) {
		row->shown--;
		return true;
	}
	return false;
}

void connectColumn(column_t *column, column_t *next, column_t *previous) {
	//todo: assert that ((next->previous==NULL) == (previous==NULL))
	if (next) next->previous = column;
	if (previous) previous->next = column;
	else if (column->table) column->table->columns = column;
	column->next = next;
	column->previous = previous;
}

void connectRow(row_t *row, row_t *next, row_t *previous) {
	//todo: assert that ((next->previous==NULL) == (previous==NULL))
	if (next) next->previous = row;
	if (previous) previous->next = row;
	else if (row->column) row->column->rows = row;
	row->next = next;
	row->previous = previous;
}

void disconnectColumn(column_t *column) {
	if (column->next) column->next->previous = column->previous;
	if (column->previous) column->previous->next = column->next;
	else if (column->table) column->table->columns = column->next;
	column->next = NULL;
	column->previous = NULL;
	column->table = NULL;
};

void disconnectRow(row_t *row) {
	if (row->next) row->next->previous = row->previous;
	if (row->previous) row->previous->next = row->next;
	else if (row->column) {
		if (row->next) row->column->rows = row->next;
		else {
			disconnectColumn(row->column);
			free(row->column);
		}
	}
	row->next = NULL;
	row->previous = NULL;
	row->column = NULL;
}

column_t *createColumn() {
	column_t *new = malloc(sizeof(column_t));
	*new = (column_t) {.previous=NULL, .next=NULL, .table=NULL, .weight=0};
	return new;
}

row_t *createRow(xcb_window_t window) {
	row_t *new = malloc(sizeof(row_t));
	*new = (row_t) {
		.previous=NULL, .next=NULL, .column=NULL,
		.weight=0, .window=window,
		.hidden=0, .shown=0
	};
	return new;
}

row_t *findRowInTable(table_t *table, xcb_window_t window) {
	for (column_t *column = table->columns; column; column = column->next) {
		for (row_t *row = column->rows; row; row = row->next) {
			if (row->window == window) return row;
		}
	}
	return NULL;
}

void recalculateTable(table_t *table) {
	if (!table->monitor) return;
	instance_t *instance = table->monitor->instance;
	
	uint16_t totalColumnWeight = 0;
	uint16_t workingWidth = table->monitor->width
		- instance->margin.left - instance->margin.right
			+ instance->padding.horizontal;
	for (column_t *column = table->columns; column; column = column->next) {
		totalColumnWeight += column->weight+1;
		workingWidth -= instance->padding.horizontal;
	}
	
	uint16_t x = table->monitor->x + instance->margin.left;
	for (column_t *column = table->columns; column; column = column->next) {
		uint16_t w = 0;
		if (column->next) w = (column->weight+1) * workingWidth / totalColumnWeight;
		else w = (table->monitor->width - instance->margin.left - instance->margin.right)
			- (x - table->monitor->x - instance->margin.left);
		
		uint16_t totalRowWeight = 0;
		uint16_t workingHeight = table->monitor->height
			- instance->margin.top - instance->margin.bottom
			+ instance->padding.vertical;
		for (row_t *row = column->rows; row; row = row->next) {
			totalRowWeight += row->weight+1;
			workingHeight -= instance->padding.vertical;
		}
		
		uint16_t y = table->monitor->y + instance->margin.top;
		for (row_t *row = column->rows; row; row = row->next) {
			uint16_t h = 0;
			if (row->next) h = (row->weight+1) * workingHeight / totalRowWeight;
			else h = (table->monitor->height - instance->margin.top - instance->margin.bottom)
				- (y - table->monitor->y - instance->margin.top);
			
			uint16_t border = instance->border.thickness < 0 ? 0 : instance->border.thickness;
			
			uint16_t mask = XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y
				| XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT
				| XCB_CONFIG_WINDOW_BORDER_WIDTH;
			xcb_configure_window(instance->connection, row->window, mask, (uint32_t[]) {
				x, y, w - 2*border, h - 2*border, border
			});
			
			y += h + instance->padding.vertical;
		}
		x += w + instance->padding.horizontal;
	}
	xcb_flush(instance->connection);
}

void hideRow(row_t *row) {
	if (!row->column->table->monitor) return;
	instance_t *instance = row->column->table->monitor->instance;
	row->hidden++;
	xcb_unmap_window(instance->connection, row->window);
}

void hideTable(table_t *table) {
	if (!table->monitor) return;
	instance_t *instance = table->monitor->instance;
	for (column_t *column = table->columns; column; column = column->next) {
		for (row_t *row = column->rows; row; row = row->next) {
			row->hidden++;
			xcb_unmap_window(instance->connection, row->window);
		}
	}
	xcb_flush(instance->connection);
}

void showTable(table_t *table) {
	if (!table->monitor) return;
	instance_t *instance = table->monitor->instance;
	for (column_t *column = table->columns; column; column = column->next) {
		for (row_t *row = column->rows; row; row = row->next) {
			row->shown++;
			xcb_map_window(instance->connection, row->window);
		}
	}
	xcb_flush(instance->connection);
}
