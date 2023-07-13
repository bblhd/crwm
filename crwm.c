#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>

typedef struct Monitor monitor_t;
typedef struct Table table_t;
typedef struct Column column_t;
typedef struct Row row_t;

typedef void (*event_handler_t)(xcb_generic_event_t *);

struct Monitor {
	table_t *table;
	monitor_t *next;
	uint16_t x, y, width, height;
};

struct Table {
	monitor_t *monitor;
	column_t *columns;
	table_t *next;
	char id;
};

struct Column {
	table_t *table;
	column_t *next, *previous;
	row_t *rows;
	uint16_t weight;
};

struct Row {
	column_t *column;
	row_t *next, *previous;
	xcb_window_t window;
	uint16_t weight;
	bool wasUnmappedByWM;
};

void options(int argc, char **argv);

void setup();
void setupX();
void createControlFile();
void setupTables();
void setupMonitors();
void createMonitor(xcb_randr_output_t output);

bool good();
void *thread_control(pthread_t *);
void *thread_events(pthread_t *);
void control();
void events();
void controlLock();
void eventsLock();

void handleEnterNotify(xcb_enter_notify_event_t *event);
void handleFocusIn(xcb_focus_in_event_t *event);
void handleFocusOut(xcb_focus_out_event_t *event);
void handleMapRequest(xcb_map_request_event_t *event);
void handleDestroyNotify(xcb_destroy_notify_event_t *event);

void cleanup();
void cleanupMonitors();
void cleanupTables();
void cleanupColumns(column_t *column);
void cleanupRows(row_t *row);

row_t *managed(xcb_window_t window);
row_t *manage(xcb_window_t window);
xcb_window_t unmanage(row_t *row);

void moveRowUp(row_t *row);
void moveRowDown(row_t *row);
void moveRowLeft(row_t *row);
void moveRowRight(row_t *row);

void growColumn(column_t *column, int amount);
void growRow(row_t *row, int amount);

table_t *searchForTable(char id);
monitor_t *getActiveMonitor();
table_t *getFirstUnusedTable();

void sendTableToMonitor(monitor_t *monitor, table_t *table);
void sendRowToTable(table_t *table, row_t *row);
void sendRowToColumn(column_t *column, row_t *row);

void disconnectColumn(column_t *);
void disconnectRow(row_t *);

void swapRowWithBelow(row_t *row);
void realignTable(table_t *table);
void realignColumns(table_t *table);
void realignRows(column_t *column);
void realignRowWidth(row_t *row);

void showTable(table_t *table);
void hideTable(table_t *table);
void hideRow(row_t *row);

void warpMouseToCenterOfWindow(xcb_window_t window);
void closeWindow(xcb_window_t window);

uint32_t tocolor(char *str);
long tonumber(char *str);
int extractNumeral(char c);
void usleep(unsigned long us);
void die(char *errstr);

xcb_connection_t *connection;
xcb_window_t root;

monitor_t *monitors;
table_t *tables;
row_t *focused;

bool exitWMFlag = false;
bool controlLockFlag = false;
bool eventsLockFlag = false;

xcb_atom_t WM_PROTOCOLS, WM_DELETE_WINDOW;

const event_handler_t eventHandlers[] = {
	[XCB_ENTER_NOTIFY] = (event_handler_t) handleEnterNotify,
	[XCB_FOCUS_IN] = (event_handler_t) handleFocusIn,
	[XCB_FOCUS_OUT] = (event_handler_t) handleFocusOut,
	[XCB_MAP_REQUEST] = (event_handler_t) handleMapRequest,
	[XCB_DESTROY_NOTIFY] = (event_handler_t) handleDestroyNotify,
	[XCB_UNMAP_NOTIFY] = (event_handler_t) handleDestroyNotify,
};

char *tablesIDString = "123456789";

uint16_t borderThickness = 1;
uint32_t focusedColor = 0x9eeeee;
uint32_t unfocusedColor = 0x55aaaa;

uint16_t padding = 0;

uint16_t topMargin = 0;
uint16_t bottomMargin = 0;
uint16_t leftMargin = 0;
uint16_t rightMargin = 0;

bool controlFileIsManaged = false;
char *controlFile = NULL;

int main(int argc, char **argv) {
	options(argc, argv);
	setup();
	
	pthread_t eventThread;
	pthread_t controlThread;
	
	pthread_create(&eventThread, NULL, (void*(*)(void*)) thread_events, &controlThread);
	pthread_create(&controlThread, NULL, (void*(*)(void*)) thread_control, &eventThread);
	
	pthread_join(eventThread, NULL);
	pthread_join(controlThread, NULL); 
	
	while (good()) events(); 
	cleanup();
	return 0;
}

void options(int argc, char **argv) {
	int i = 1;
	while (i < argc) {
		if (i+1 >= argc) die("Option provided with no parameters.");
		if (strcmp(argv[i], "--tables") == 0) {
			tablesIDString = argv[i+1];
			i+=2;
		} else if (strcmp(argv[i], "--file") == 0) {
			controlFile = argv[i+1];
			i+=2;
		} else if (strcmp(argv[i], "--padding") == 0) {
			padding = (uint16_t) tonumber(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--margin") == 0) {
			uint16_t amount = (uint16_t) tonumber(argv[i+1]);
			topMargin = amount;
			bottomMargin = amount;
			leftMargin = amount;
			rightMargin = amount;
			i+=2;
		} else if (strcmp(argv[i], "--top") == 0) {
			topMargin = (uint16_t) tonumber(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--bottom") == 0) {
			bottomMargin = (uint16_t) tonumber(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--left") == 0) {
			leftMargin = (uint16_t) tonumber(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--right") == 0) {
			rightMargin = (uint16_t) tonumber(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--focused") == 0) {
			focusedColor = (uint32_t) tocolor(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--unfocused") == 0) {
			unfocusedColor = (uint32_t) tocolor(argv[i+1]);
			i+=2;
		} else if (strcmp(argv[i], "--border") == 0) {
			borderThickness = (uint16_t) tonumber(argv[i+1]);
			i+=2;
		} else die("Unrecognised commandline argument.");
	}
}

void setup() {
	setupX();
	createControlFile();
	setupTables();
	setupMonitors();
}

void createControlFile() {
	if (controlFile) return;
	controlFileIsManaged = true;
	
	size_t pathlen = strlen("/tmp/crwm.d/")+strlen(getenv("DISPLAY"))+1;
	controlFile = malloc(pathlen);
	if (!controlFile) die("Could not allocate control file path.");
	snprintf(controlFile, pathlen, "/tmp/crwm.d/%s", getenv("DISPLAY"));
	
	if (mkdir("/tmp/crwm.d", S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
		if (mkfifo(controlFile, S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) return;
		rmdir("/tmp/crwm.d");
	}
	die("Unable to create control file.");
}

void setupTables() {
	table_t **where = &tables;
	for (int i = 0; tablesIDString[i]; i++) {
		*where = calloc(1,sizeof(table_t));
		if (!*where) die("Could not allocate table.");
		(*where)->id = tablesIDString[i];
		where = &(*where)->next;
	}
}

void setupX() {
	if (!getenv("DISPLAY")) die("Could not get display.");
	connection = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(connection)) die("Unable to connect to X.\n");
	const xcb_query_extension_reply_t *randrfacts = xcb_get_extension_data(connection, &xcb_randr_id);
	if (!randrfacts->present) die("Randr extension is not present.\n");
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;
	root = screen->root;
	xcb_change_window_attributes_checked(
		connection, root, XCB_CW_EVENT_MASK, (uint32_t []) {
			XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		}
	);
	xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(
		connection,xcb_intern_atom(connection,0,12,"WM_PROTOCOLS"),NULL
	);
	WM_PROTOCOLS = reply ? reply->atom : XCB_NONE;
	free(reply);
	reply = xcb_intern_atom_reply(
		connection,xcb_intern_atom(connection,0,16,"WM_DELETE_WINDOW"),NULL
	);
	WM_DELETE_WINDOW = reply ? reply->atom : XCB_NONE;
	free(reply);
	
	xcb_flush(connection);
}

void setupMonitors() {
	xcb_randr_get_screen_resources_current_reply_t *resources
		= xcb_randr_get_screen_resources_current_reply(
			connection, xcb_randr_get_screen_resources_current(
				connection, root
			), NULL
		);

	xcb_randr_output_t *randr_outputs
		= xcb_randr_get_screen_resources_current_outputs(resources);
	int n = xcb_randr_get_screen_resources_current_outputs_length(resources);
	for (int i = 0; i < n; i++) createMonitor(randr_outputs[i]);

	free(resources);
}

void createMonitor(xcb_randr_output_t output) {
	xcb_randr_get_output_info_reply_t *outputinfo = xcb_randr_get_output_info_reply(
		connection,
		xcb_randr_get_output_info(connection, output, XCB_CURRENT_TIME),
		XCB_NONE
	);
	if (
		outputinfo != NULL
		&& outputinfo->crtc != XCB_NONE
		&& outputinfo->connection != XCB_RANDR_CONNECTION_DISCONNECTED
	) {
		xcb_randr_get_crtc_info_reply_t *crtcinfo = xcb_randr_get_crtc_info_reply(
			connection, xcb_randr_get_crtc_info(
				connection, outputinfo->crtc, XCB_CURRENT_TIME
			), NULL
		);
		monitor_t *monitor = calloc(1,sizeof(monitor_t));
		if (!monitor) die("Could not allocate monitor.");
		table_t *table = getFirstUnusedTable();
		if (!table) die("More monitors than tables.");
		*monitor = (monitor_t) {
			table, .next=monitors,
			crtcinfo->x, crtcinfo->y, crtcinfo->width, crtcinfo->height
		};
		table->monitor = monitor;
		monitors = monitor;
		free(crtcinfo);
	}
	free(outputinfo);
}

bool good() {
	return !exitWMFlag && !xcb_connection_has_error(connection);
}

void controlLock() {
	do {
		controlLockFlag = false;
		while (eventsLockFlag) usleep(100);
		controlLockFlag = true;
	} while (eventsLockFlag);
}

void eventsLock() {
	eventsLockFlag = true;
	while (controlLockFlag) usleep(100);
}

void *thread_control(pthread_t *other) {
	(void) other;
	while (good()) control();
	pthread_cancel(*other);
	return NULL;
}

void control() {
	FILE *file = fopen(controlFile, "rw");
	if (!file) return;
	
	controlLock();
	
	char command=0;
	char where=0;
	long windownum=0;
	fscanf(file, "%c[%c]%li", &command, &where, &windownum);
	if (command > 0x20) {
		row_t *client = focused;
		if (windownum) client = managed((xcb_window_t) windownum);
		
		switch (command) {
			case 'x':
			if (where == 0) exitWMFlag = true;
			else if (where == 'w' && client) closeWindow(client->window);
			break;
			case 'm':
			if (client) switch (where) {
				case 'u': moveRowUp(client); break;
				case 'd': moveRowDown(client); break;
				case 'l': moveRowLeft(client); break;
				case 'r': moveRowRight(client); break;
			}
			break;
			case 'l':
			if (client) {
				switch (where) {
					case 'u': client = client->previous ? client->previous : client; break;
					case 'd': client = client->next ? client->next : client; break;
					case 'l': client = client->column->previous ? client->column->previous->rows : client; break;
					case 'r': client = client->column->next ? client->column->next->rows : client; break;
				}
				sendTableToMonitor(getActiveMonitor(), client->column->table);
				warpMouseToCenterOfWindow(client->window);
			}
			break;
			case 'v':
			if (client) switch (where) {
				case '+': growRow(client, 1); break;
				case '-': growRow(client, -1); break;
				case '=': growRow(client, -client->weight); break;
				default: growRow(client, extractNumeral(where));
			}
			break;
			case 'h':
			if (client) switch (where) {
				case '+': growColumn(client->column, 1); break;
				case '-': growColumn(client->column, -1); break;
				case '=': growColumn(client->column, -client->column->weight); break;
				default: growColumn(client->column, extractNumeral(where));
			}
			break;
			case 's':
			if (client) sendRowToTable(searchForTable(where), client);
			break;
			case 't':
			sendTableToMonitor(getActiveMonitor(), searchForTable(where));
			break;
		}
	} 
	fclose(file);
	
	controlLockFlag = false;
}

void *thread_events(pthread_t *other) {
	while (good()) events();
	pthread_cancel(*other);
	return NULL;
}

void events() {
	xcb_generic_event_t *event = xcb_wait_for_event(connection);
	
	eventsLock();
	
	do {
		uint8_t evtype = event->response_type & ~0x80;
		if (evtype < sizeof(eventHandlers)/sizeof(event_handler_t) && eventHandlers[evtype]) {
			eventHandlers[evtype](event);
		}
		free(event);
		xcb_flush(connection);
	} while ((event = xcb_poll_for_event(connection)));
	
	eventsLockFlag = false;
}

void handleEnterNotify(xcb_enter_notify_event_t *event) {
	xcb_set_input_focus(
		connection, XCB_INPUT_FOCUS_POINTER_ROOT,
		event->event, XCB_CURRENT_TIME
	);
	focused = managed(event->event);
}

void handleFocusIn(xcb_focus_in_event_t *event) {
	xcb_change_window_attributes(
		connection, event->event,
		XCB_CW_BORDER_PIXEL, (uint32_t[]) {focusedColor}
	);
}

void handleFocusOut(xcb_focus_out_event_t *event) {
	xcb_change_window_attributes(
		connection, event->event,
		XCB_CW_BORDER_PIXEL, (uint32_t[]) {unfocusedColor}
	);
}

void handleMapRequest(xcb_map_request_event_t *event) {
	xcb_change_window_attributes_checked(
		connection, event->window, XCB_CW_EVENT_MASK, (uint32_t[]) {
			XCB_EVENT_MASK_ENTER_WINDOW
			| XCB_EVENT_MASK_FOCUS_CHANGE
		}
	);
	manage(event->window);
}

void handleDestroyNotify(xcb_destroy_notify_event_t *event) {
	row_t *client = managed(event->window);
	if (!client) return;
	
	if (client->wasUnmappedByWM) {
		client->wasUnmappedByWM = false;
	} else {
		if (focused && focused->window == client->window) focused = NULL;
		unmanage(client);
	}
}

void cleanup() {
	if (controlFileIsManaged) {
		remove(controlFile);
		free(controlFile);
		rmdir("/tmp/crwm.d");
	}
	cleanupMonitors();
	cleanupTables();
	xcb_disconnect(connection);
}

void cleanupMonitors() {
	monitor_t *monitor = monitors;
	while (monitor) {
		monitor_t *next = monitor->next;
		free(monitor);
		monitor = next;
	}
}

void cleanupTables() {
	table_t *table = tables;
	while (table) {
		table_t *next = table->next;
		cleanupColumns(table->columns);
		free(table);
		table = next;
	}
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

row_t *managed(xcb_window_t window) {
	for (table_t *table = tables; table; table = table->next) {
		for (column_t *column = table->columns; column; column = column->next) {
			for (row_t *row = column->rows; row; row = row->next) {
				if (row->window == window) return row;
			}
		}
	}
	return NULL;
}

row_t *manage(xcb_window_t window) {
	row_t *row = managed(window);
	if (!row) {
		row = calloc(1,sizeof(row_t));
		if (!row) die("Could not allocate row.");
		row->window = window;
		if (focused) {
			if (!focused->column->next) {
				focused->column->next = calloc(1,sizeof(column_t));
				if (!focused->column->next) die("Could not allocate column.");
				focused->column->next->table = focused->column->table;
				focused->column->next->previous = focused->column;
				realignColumns(focused->column->table);
			}
			sendRowToColumn(focused->column->next, row);
		} else sendRowToTable(getActiveMonitor()->table, row);
		
		xcb_configure_window(
			connection, row->window, XCB_CONFIG_WINDOW_BORDER_WIDTH,
			(uint32_t[]) {borderThickness}
		);
		xcb_change_window_attributes(
			connection, row->window,
			XCB_CW_BORDER_PIXEL, (uint32_t[]) {unfocusedColor}
		);
		if (row->column->table->monitor) realignRowWidth(row);
		xcb_flush(connection);
	}
	return row;
}

xcb_window_t unmanage(row_t *row) {
	if (row == focused) focused = NULL;
	xcb_window_t window = row->window;
	disconnectRow(row);
	free(row);
	return window;
}

void moveRowUp(row_t *row) {
	if (row->previous) swapRowWithBelow(row->previous);
}

void moveRowDown(row_t *row) {
	swapRowWithBelow(row);
}

void moveRowLeft(row_t *row) {
	if (!row->column->previous) {
		if (!row->next && !row->previous) return;
		row->column->previous = calloc(1,sizeof(column_t));
		if (!row->column->previous) die("Could not allocate column.");
		row->column->previous->table = row->column->table;
		row->column->previous->next = row->column;
		row->column->table->columns = row->column->previous;
		realignColumns(focused->column->table);
	}
	sendRowToColumn(row->column->previous, row);
}

void moveRowRight(row_t *row) {
	if (!row->column->next) {
		if (!row->next && !row->previous) return;
		row->column->next = calloc(1,sizeof(column_t));
		if (!row->column->next) die("Could not allocate column.");
		row->column->next->table = row->column->table;
		row->column->next->previous = row->column;
		realignColumns(focused->column->table);
	}
	sendRowToColumn(row->column->next, row);
}

void growColumn(column_t *column, int amount) {
	if (-amount >= (int) column->weight) column->weight = 0;
	else column->weight = (uint16_t) ((int)column->weight + amount);
	realignColumns(column->table);
}

void growRow(row_t *row, int amount) {
	if (-amount >= (int) row->weight) row->weight = 0;
	else row->weight = (uint16_t) ((int)row->weight + amount);
	realignRows(row->column);
}

table_t *searchForTable(char id) {
	for (table_t *table = tables; table; table = table->next) {
		if (table->id == id) return table;
	}
	return NULL;
}

table_t *getFirstUnusedTable() {
	for (table_t *table = tables; table; table = table->next) {
		if (!table->monitor) return table;
	}
	return NULL;
}

monitor_t *getActiveMonitor() {
	if (focused && focused->column->table->monitor) {
		return focused->column->table->monitor;
	}
	
	xcb_query_pointer_reply_t *pointer = xcb_query_pointer_reply(
		connection, xcb_query_pointer(connection, root), XCB_NONE
	);
	uint16_t x=pointer->root_x, y=pointer->root_y;
	free(pointer);
	
	for (monitor_t *monitor = monitors; monitor; monitor = monitor->next) {
		if (
			x >= monitor->x && x < monitor->x + monitor->width
			&& y >= monitor->y && y < monitor->y + monitor->height
		) return monitor;
	}
	return monitors;
}

void sendTableToMonitor(monitor_t *monitor, table_t *table) {
	if (!monitor || !table) return;
	if (monitor == table->monitor) return;
	if (table->monitor) table->monitor->table = monitor->table;
	if (monitor->table) {
		monitor->table->monitor = table->monitor;
		if (table->monitor) realignTable(monitor->table);
		else hideTable(monitor->table);
	}
	table->monitor = monitor;
	monitor->table = table;
	showTable(table);
	realignTable(table);
}

void sendRowToTable(table_t *table, row_t *row) {
	if (!table->columns) {
		table->columns = calloc(1,sizeof(column_t));
		if (!table->columns) die("Could not allocate column.");
		table->columns->table = table;
	}
	sendRowToColumn(table->columns, row);
}

void sendRowToColumn(column_t *column, row_t *row) {
	bool hide = row->column && (row->column->table->monitor && !column->table->monitor);
	bool show = !row->column || (!row->column->table->monitor && column->table->monitor);
	
	disconnectRow(row);
	row->next = column->rows;
	row->previous = NULL;
	row->column = column;
	column->rows = row;
	if (row->next) row->next->previous = row;
	
	if (column->table->monitor) {
		realignRows(row->column);
		realignRowWidth(row);
	}
	if (hide) hideRow(row);
	if (show) xcb_map_window(connection, row->window);
	xcb_flush(connection);
}

void disconnectColumn(column_t *column) {
	if (column->next) column->next->previous = column->previous;
	if (column->previous) column->previous->next = column->next;
	else column->table->columns = column->next;
	if (column->table->monitor) realignColumns(column->table);
}

void disconnectRow(row_t *row) {
	if (row->next) row->next->previous = row->previous;
	if (row->previous) row->previous->next = row->next;
	else if (row->column) row->column->rows = row->next;
	if (row->column) {
		if (!row->column->rows) {
			disconnectColumn(row->column);
			free(row->column);
		} else if (row->column->table->monitor) realignRows(row->column);
	}
}

void swapRowWithBelow(row_t *row) {
	if (!row->next) return;
	row_t *other = row->next;
	
	table_t *table = row->column->table;
	uint16_t effectiveHeight = table->monitor->height + padding - bottomMargin - topMargin;
	if (bottomMargin + topMargin >= table->monitor->height + padding) effectiveHeight = 0;
	
	uint16_t totalRowWeights = 0;
	for (row_t *r = row->column->rows; r; r = r->next) {
		totalRowWeights += r->weight+1;
		effectiveHeight -= padding;
	}
	
	uint16_t yoff = row->column->table->monitor->y + topMargin;
	uint16_t weighttally = 0;
	for (row_t *r = row->previous; r; r = r->previous) {
		weighttally += row->weight+1;
		yoff += padding;
	}
	
	uint16_t newY = yoff+padding+effectiveHeight * (weighttally+other->weight+1) / totalRowWeights;
	
	xcb_configure_window(
		connection, row->window, XCB_CONFIG_WINDOW_Y,
		(uint32_t[]) {newY}
	);
	xcb_configure_window(
		connection, other->window, XCB_CONFIG_WINDOW_Y,
		(uint32_t[]) {yoff+effectiveHeight*weighttally/totalRowWeights}
	);
	
	xcb_flush(connection);
	
	if (!row->previous) row->column->rows = other;
	row->next = other->next;
	if (row->next) row->next->previous = row;
	other->previous = row->previous;
	if (other->previous) other->previous->next = other;
	row->previous = other;
	other->next = row;
}

void realignTable(table_t *table) {
	realignColumns(table);
	for (column_t *column = table->columns; column; column = column->next) {
		realignRows(column);
	}
}

void realignColumns(table_t *table) {
	uint16_t effectiveWidth = table->monitor->width + padding - rightMargin - leftMargin;
	if (rightMargin + leftMargin >= table->monitor->width + padding) effectiveWidth = 0;
	
	uint16_t totalColumnWeights = 0;
	for (column_t *column = table->columns; column; column = column->next) {
		totalColumnWeights += column->weight+1;
		effectiveWidth -= padding;
	}
	uint16_t xoff = table->monitor->x + leftMargin;
	uint16_t weighttally = 0;
	for (column_t *column = table->columns; column; column = column->next) {
		uint16_t width = effectiveWidth * (column->weight+1) / totalColumnWeights;
		for (row_t *row = column->rows; row; row = row->next) {
			xcb_configure_window(
				connection, row->window, 
				XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH,
				(uint32_t[]) {
					xoff + effectiveWidth * weighttally / totalColumnWeights, 
					width<2*borderThickness ? 0 : width-2*borderThickness
				}
			);
		}
		weighttally += (column->weight+1);
		xoff += padding;
		xcb_flush(connection);
	}
}

void realignRows(column_t *column) {
	uint16_t effectiveHeight = column->table->monitor->height + padding - bottomMargin - topMargin;
	if (bottomMargin + topMargin >= column->table->monitor->height + padding) effectiveHeight = 0;
	
	uint16_t totalRowWeights = 0;
	for (row_t *row = column->rows; row; row = row->next) {
		totalRowWeights += row->weight+1;
		effectiveHeight -= padding;
	}
	
	uint16_t yoff = column->table->monitor->y + topMargin;
	uint16_t weighttally = 0;
	for (row_t *row = column->rows; row; row = row->next) {
		uint16_t height = effectiveHeight * (row->weight+1) / totalRowWeights;
		xcb_configure_window(
			connection, row->window, 
			XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_HEIGHT,
			(uint32_t[]) {
				yoff + effectiveHeight * weighttally / totalRowWeights, 
				height<2*borderThickness ? 0 : height-2*borderThickness
			}
		);
		weighttally += (row->weight+1);
		yoff += padding;
	}
	xcb_flush(connection);
}

void realignRowWidth(row_t *row) {
	table_t *table = row->column->table;
	uint16_t effectiveWidth = table->monitor->width + padding - rightMargin - leftMargin;
	if (rightMargin + leftMargin >= table->monitor->width + padding) effectiveWidth = 0;
	
	uint16_t totalColumnWeights = 0;
	for (column_t *column = table->columns; column; column = column->next) {
		totalColumnWeights += column->weight+1;
		effectiveWidth -= padding;
	}
	uint16_t xoff = table->monitor->x + leftMargin;
	uint16_t weighttally = 0;
	for (column_t *column = row->column->previous; column; column = column->previous) {
		weighttally += (column->weight+1);
		xoff += padding;
	}
	uint16_t width = effectiveWidth * (row->column->weight+1) / totalColumnWeights;
	xcb_configure_window(
		connection, row->window, 
		XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_WIDTH,
		(uint32_t[]) {
			xoff + effectiveWidth * weighttally / totalColumnWeights,
			width<2*borderThickness ? 0 : width-2*borderThickness
		}
	);
	xcb_flush(connection);
}

void showTable(table_t *table) {
	for (column_t *column = table->columns; column; column = column->next) {
		for (row_t *row = column->rows; row; row = row->next) {
			xcb_map_window(connection, row->window);
		}
		xcb_flush(connection);
	}
}

void hideTable(table_t *table) {
	for (column_t *column = table->columns; column; column = column->next) {
		for (row_t *row = column->rows; row; row = row->next) {
			hideRow(row);
		}
		xcb_flush(connection);
	}
}

void hideRow(row_t *row) {
	row->wasUnmappedByWM = true;
	if (row == focused) focused = NULL;
	xcb_unmap_window(connection, row->window);
}

void warpMouseToCenterOfWindow(xcb_window_t window) {
	uint16_t x, y;
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(
		connection, xcb_get_geometry(connection, window), NULL
	);
	x = geom->width >> 1;
	y = geom->height >> 1;
	free(geom);
	xcb_warp_pointer(connection, XCB_NONE, window, 0,0,0,0, x,y);
	xcb_flush(connection);
}

void closeWindow(xcb_window_t window) {
	if (WM_PROTOCOLS && WM_DELETE_WINDOW) {
		xcb_client_message_event_t ev;
		
		memset(&ev, 0, sizeof(ev));
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = window;
		ev.format = 32;
		ev.type = WM_PROTOCOLS;
		ev.data.data32[0] = WM_DELETE_WINDOW;
		ev.data.data32[1] = XCB_CURRENT_TIME;
		
		xcb_send_event(connection, 0, window, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
	} else {
		xcb_kill_client(connection, window);
	}
	xcb_flush(connection);
}

long tonumber(char *str) {
	return strtol(str, NULL, 0);
}

uint32_t tocolor(char *str) {
	if (str && *str == '#') return strtol(str+1, NULL, 16);
	else return 0;
}

int extractNumeral(char c) {
	return c>='A' &&  c<='Z' ? c-'A'+1 : c>='a' && c<='z' ? 'a'-c-1 : 0;
}

void usleep(unsigned long us) {
	struct timespec ts;
	ts.tv_sec = us / 1000000;
	ts.tv_nsec = (us % 1000000) * 1000;
	nanosleep(&ts, &ts);
}

void die(char *message) {
	fprintf(stderr, "%s\n", message);
	exit(1);
}
