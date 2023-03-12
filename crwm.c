#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <pthread.h>

#include <ctype.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>

#include "ctheme.h"
#include "layout.h"

void setup();
void cleanup();

void *thread_commands(pthread_t *a);
void *thread_events(pthread_t *a);

void reloadColors();
void setupMonitors();
void setupAtoms();

typedef void (*event_handler_t)(xcb_generic_event_t *);
const event_handler_t eventHandlers[XCB_GE_GENERIC];

typedef unsigned int uint;

void spawn(char **command);
void killclient(xcb_window_t);
void die(char *errstr);
void setBorderColor(xcb_window_t window, color_t c);
void unfocus(xcb_window_t window);
void focus(xcb_window_t window);
void lookAtWindow(xcb_window_t);
void warpMouseToCenterOfMonitor(monitor_t *monitor);

monitor_t *getActiveMonitor();
row_t *manage(xcb_window_t);
void unmanage(row_t *client);
row_t *managed(xcb_window_t);

bool exitWMFlag = false;

char *fifopath;

instance_t global;
row_t *focused = NULL;

enum AtomOfXCB {WM_PROTOCOLS, WM_DELETE_WINDOW, ATOMCOUNT};
const char *atomStrings[ATOMCOUNT] = {"WM_PROTOCOLS", "WM_DELETE_WINDOW"};
xcb_atom_t atoms[ATOMCOUNT];

#define MAX_MONITORS 8
monitor_t monitors[MAX_MONITORS];
size_t monitorCount = 0;

#define TABLE_COUNT 9
table_t tables[TABLE_COUNT];

enum Commandcodes {
	COMMAND_NULL,
	COMMAND_EXIT,
	COMMAND_RELOAD,
	COMMAND_CLOSE,
	COMMAND_MOVE,
	COMMAND_LOOK,
	COMMAND_SEND,
	COMMAND_SWITCH,
	COMMAND_GROW_VERTICAL,
	COMMAND_GROW_HORIZONTAL,
	COMMAND_BORDER_THICKNESS,
	COMMAND_PADDING_ALL,
	COMMAND_MARGIN_ALL,
	COMMAND_PADDING_HORIZONTAL,
	COMMAND_PADDING_VERTICAL,
	COMMAND_MARGIN_TOP,
	COMMAND_MARGIN_BOTTOM,
	COMMAND_MARGIN_LEFT,
	COMMAND_MARGIN_RIGHT,
};

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;
	setup();
	
	pthread_t eventThread;
	pthread_t commandThread;
	
	pthread_create(&eventThread, NULL, (void*(*)(void*)) thread_events, &commandThread);
	pthread_create(&commandThread, NULL, (void*(*)(void*)) thread_commands, &eventThread);
	
	pthread_join(eventThread, NULL);
	pthread_join(commandThread, NULL);
	
	cleanup();
}

bool setupFIFO();

void setup() {
	if (!setupFIFO()) {
		die("Unable to setup crwm fifo.\n");
	}
	
	global.connection = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(global.connection)) {
		die("Unable to connect to X.\n");
	}
	
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(global.connection)).data;
	global.root = screen->root;
	
	xcb_change_window_attributes_checked(
		global.connection, global.root, XCB_CW_EVENT_MASK, (uint32_t []) {
			XCB_EVENT_MASK_STRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
			| XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT
		}
	);
	
	global.border.thickness = 1;
	
	global.padding.horizontal = 0;
	global.padding.vertical = 0;
	
	global.margin.top = 0;
	global.margin.bottom = 0;
	global.margin.left = 0;
	global.margin.right = 0;
	
	reloadColors();
	setupAtoms();
	setupMonitors();
	
	xcb_flush(global.connection);
}

void cleanupFIFO();

void cleanup() {
	for (table_t *table = tables; table < tables+TABLE_COUNT; table++) {
		cleanupTable(table);
	}
	xcb_disconnect(global.connection);
	cleanupFIFO();
}

bool good() {
	return !exitWMFlag && !xcb_connection_has_error(global.connection);
}

void events();
void *thread_events(pthread_t *other) {
	while (good()) events();
	pthread_cancel(*other);
	return NULL;
}

void commands();
void *thread_commands(pthread_t *other) {
	(void) other;
	while (good()) commands();
	pthread_cancel(*other);
	return NULL;
}

void events() {
	xcb_generic_event_t *event = xcb_wait_for_event(global.connection);
	do {
		uint8_t evtype = event->response_type & ~0x80;
		if (evtype < sizeof(eventHandlers)/sizeof(event_handler_t) && eventHandlers[evtype]) {
			//printf("SUPPORTED %u\n", evtype);
			eventHandlers[evtype](event);
		}// else printf("UNSUPPORTED %u\n", evtype);
		free(event);
		xcb_flush(global.connection);
	} while ((event = xcb_poll_for_event(global.connection)));
}

void reloadColors() {
	ctheme_clear();
	if (!ctheme_load(NULL)) {
		ctheme_set(COLORSCHEME_DEFAULT, 1, 0xffffff, BGR);
		ctheme_set(COLORSCHEME_DEFAULT, 2, 0x000000, BGR);
		ctheme_set(COLORSCHEME_DEFAULT, 4, 0x9eeeee, BGR);
		ctheme_set(COLORSCHEME_SELECTED, 1, 0x000000, BGR);
		ctheme_set(COLORSCHEME_SELECTED, 2, 0xffffff, BGR);
		ctheme_set(COLORSCHEME_SELECTED, 4, 0x55aaaa, BGR);
	}
	global.border.unfocused = ctheme_get(COLORSCHEME_DEFAULT, 4, BGR);
	global.border.focused = ctheme_get(COLORSCHEME_SELECTED, 4, BGR);
}

void setupAtoms() {
	xcb_intern_atom_cookie_t cookies[ATOMCOUNT];
	for (enum AtomOfXCB i = WM_PROTOCOLS; i < ATOMCOUNT; i++) {
		cookies[i] = xcb_intern_atom(global.connection, 0, strlen(atomStrings[i]), atomStrings[i]);
	}
	for (enum AtomOfXCB i = WM_PROTOCOLS; i < ATOMCOUNT; i++) {
		xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(global.connection, cookies[i], NULL);
		atoms[i] = reply ? reply->atom : XCB_NONE;
		free(reply);
	}
}

//code adapted from https://stackoverflow.com/questions/36966900/xcb-get-all-monitors-ands-their-x-y-coordinates
void setupMonitors() {
	xcb_randr_get_screen_resources_current_reply_t *resources
		= xcb_randr_get_screen_resources_current_reply(
			global.connection, xcb_randr_get_screen_resources_current(
				global.connection, global.root
			), NULL
		);

	xcb_timestamp_t timestamp = resources->config_timestamp;
	monitorCount = xcb_randr_get_screen_resources_current_outputs_length(resources);
	if (monitorCount > MAX_MONITORS) monitorCount = MAX_MONITORS;
	xcb_randr_output_t *randr_outputs = xcb_randr_get_screen_resources_current_outputs(resources);
	
	for (size_t i = 0; i < monitorCount; i++) {
		xcb_randr_get_output_info_reply_t *output
			= xcb_randr_get_output_info_reply(
				global.connection,
				xcb_randr_get_output_info(global.connection, randr_outputs[i], timestamp),
				XCB_NONE
			);
		
		if (
			output == NULL
			|| output->crtc == XCB_NONE
			|| output->connection == XCB_RANDR_CONNECTION_DISCONNECTED
		) continue;

		xcb_randr_get_crtc_info_reply_t *crtc = xcb_randr_get_crtc_info_reply(
			global.connection, xcb_randr_get_crtc_info(global.connection, output->crtc, timestamp), NULL
		);
	    free(output);
		
		monitors[i] = (struct Monitor) {
			.table=&tables[i], .instance=&global,
			.x=crtc->x, .y=crtc->y, .width=crtc->width, .height=crtc->height
		};
		tables[i].monitor = &monitors[i];
		
	    free(crtc);
	}

	free(resources);
}

bool setupFIFO() {
	if (!getenv("DISPLAY")) return false;
	size_t pathlen = strlen("/tmp/crwm.d/")+strlen(getenv("DISPLAY"))+1;
	fifopath = malloc(pathlen);
	snprintf(fifopath, pathlen, "/tmp/crwm.d/%s", getenv("DISPLAY"));
	
	if (mkdir("/tmp/crwm.d", S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
		if (mkfifo(fifopath, S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) return true;
		rmdir("/tmp/crwm.d");
	}
	return false;
}

void cleanupFIFO() {
	remove(fifopath);
	free(fifopath);
	rmdir("/tmp/crwm.d");
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

void commands() {
	int fd = open(fifopath, O_RDONLY);
	
	if (!fd) return;
	
	char command[2];
	if (read(fd, command, 2) != 2) {
		close(fd);
		return;
	} else close(fd);
	
	switch (command[0]) {
		case COMMAND_EXIT:
		exitWMFlag = true;
		break;
		case COMMAND_RELOAD:
		reloadColors();
		break;
		
		case COMMAND_CLOSE:
		if (focused) killclient(focused->window);
		break;
		
		case COMMAND_MOVE:
		if (focused) switch (command[1]) {
			case 'u': moveRowUp(focused); break;
			case 'd': moveRowDown(focused); break;
			case 'l': moveRowLeft(focused); break;
			case 'r': moveRowRight(focused); break;
			default:
		}
		break;
		case COMMAND_LOOK:
		if (focused) {
			switch (command[1]) {
				case 'u': focused = focused->previous ? focused->previous : focused; break;
				case 'd': focused = focused->next ? focused->next : focused; break;
				case 'l': focused = focused->column->previous ? focused->column->previous->rows : focused; break;
				case 'r': focused = focused->column->next ? focused->column->next->rows : focused; break;
				default:
			}
			lookAtWindow(focused->window);
		}
		break;
		
		case COMMAND_SEND:
		if (focused && command[1]-1 >= 0 && command[1]-1 < TABLE_COUNT) {
			moveRowToTable(&tables[command[1]-1], focused);
		}
		break;
		case COMMAND_SWITCH:
		if (command[1]-1 >= 0 && command[1]-1 < TABLE_COUNT) {
			sendTableToMonitor(getActiveMonitor(), &tables[command[1]-1]);
		}
		break;
		
		case COMMAND_GROW_VERTICAL:
		if (focused) growVertically(focused, (int) command[1]);
		break;
		case COMMAND_GROW_HORIZONTAL:
		if (focused) growHorizontally(focused, (int) command[1]);
		break;
		
		case COMMAND_BORDER_THICKNESS:
		global.border.thickness = (uint16_t) command[1];
		break;
		
		case COMMAND_PADDING_ALL:
		global.padding.horizontal = (uint16_t) command[1];
		global.padding.vertical = (uint16_t) command[1];
		break;
		case COMMAND_PADDING_HORIZONTAL:
		global.padding.horizontal = (uint16_t) command[1];
		break;
		case COMMAND_PADDING_VERTICAL:
		global.padding.vertical = (uint16_t) command[1];
		break;
		
		case COMMAND_MARGIN_ALL:
		global.margin.top = (uint16_t) command[1];
		global.margin.bottom = (uint16_t) command[1];
		global.margin.left = (uint16_t) command[1];
		global.margin.right = (uint16_t) command[1];
		break;
		case COMMAND_MARGIN_TOP:
		global.margin.top = (uint16_t) command[1];
		break;
		case COMMAND_MARGIN_BOTTOM:
		global.margin.bottom = (uint16_t) command[1];
		break;
		case COMMAND_MARGIN_LEFT:
		global.margin.left = (uint16_t) command[1];
		break;
		case COMMAND_MARGIN_RIGHT:
		global.margin.right = (uint16_t) command[1];
		break;
		default:
	}
	
	switch(command[1]) {
		case COMMAND_BORDER_THICKNESS:
		case COMMAND_MARGIN_ALL:
		case COMMAND_MARGIN_TOP:
		case COMMAND_MARGIN_BOTTOM:
		case COMMAND_MARGIN_LEFT:
		case COMMAND_MARGIN_RIGHT:
		case COMMAND_PADDING_ALL:
		case COMMAND_PADDING_HORIZONTAL:
		case COMMAND_PADDING_VERTICAL:
		for (size_t m = 0; m < monitorCount; m++) {
			if (monitors[m].table) recalculateTable(monitors[m].table);
		}
		break;
		default:
	}
}

void handleEnterNotify(xcb_enter_notify_event_t *event) {
	focus(event->event);
}

void handleLeaveNotify(xcb_leave_notify_event_t *event) {
	unfocus(event->event);
}


void handleFocusIn(xcb_focus_in_event_t *event) {
	setBorderColor(event->event, global.border.focused);
}

void handleFocusOut(xcb_focus_out_event_t *event) {
	setBorderColor(event->event, global.border.unfocused);
}

void handleMapRequest(xcb_map_request_event_t *event) {
	xcb_configure_window(
		global.connection, event->window, XCB_CONFIG_WINDOW_BORDER_WIDTH, (uint32_t[]) {
			global.border.thickness
		}
	);
	xcb_change_window_attributes_checked(
		global.connection, event->window, XCB_CW_EVENT_MASK, (uint32_t[]) {
			XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW
			| XCB_EVENT_MASK_FOCUS_CHANGE
			| XCB_EVENT_MASK_RESIZE_REDIRECT
		}
	);
	setBorderColor(event->window, global.border.unfocused);
	manage(event->window);
}

void handleDestroyNotify(xcb_destroy_notify_event_t *event) {
	row_t *client = managed(event->window);
	if (client && !checkUnmapping(client)) {
		if (focused && focused->window == client->window) focused = NULL;
		unmanage(client);
	}
}

const event_handler_t eventHandlers[] = {
	[XCB_ENTER_NOTIFY] = (event_handler_t) handleEnterNotify,
	[XCB_LEAVE_NOTIFY] = (event_handler_t) handleLeaveNotify,
	[XCB_FOCUS_IN] = (event_handler_t) handleFocusIn,
	[XCB_FOCUS_OUT] = (event_handler_t) handleFocusOut,
	[XCB_MAP_REQUEST] = (event_handler_t) handleMapRequest,
	[XCB_DESTROY_NOTIFY] = (event_handler_t) handleDestroyNotify,
	[XCB_UNMAP_NOTIFY] = (event_handler_t) handleDestroyNotify,
};

monitor_t *getActiveMonitor() {
	if (focused && focused->column->table->monitor) {
		return focused->column->table->monitor;
	}
	
	xcb_query_pointer_reply_t *pointer = xcb_query_pointer_reply(
		global.connection, xcb_query_pointer(global.connection, global.root), XCB_NONE
	);
	uint16_t x=pointer->root_x, y=pointer->root_y;
	free(pointer);
	
	for (size_t m = 0; m < monitorCount; m++) {
		if (
			x >= monitors[m].x && x < monitors[m].x + monitors[m].width
			&& y >= monitors[m].y && y < monitors[m].y + monitors[m].height
		) return &monitors[m];
	}
	
	return monitors;
}

row_t *managed(xcb_window_t window) {
	for (table_t *table = tables; table < tables+TABLE_COUNT; table++) {
		row_t *row = findRowInTable(table, window);
		if (row) return row;
	}
	return NULL;
}


row_t *manage(xcb_window_t window) {
	row_t *row = managed(window);
	if (row) return row;
	monitor_t *m = getActiveMonitor();
	if (focused && focused->column->table->monitor == m) return newRowAtRow(focused, window);
	else return newRowAtTable(m->table, window);
}

void unmanage(row_t *client) {
	removeRow(client);
}

void killclient(xcb_window_t window) {
	if (atoms[WM_PROTOCOLS] && atoms[WM_DELETE_WINDOW]) {
		xcb_client_message_event_t ev;
		
		memset(&ev, 0, sizeof(ev));
		ev.response_type = XCB_CLIENT_MESSAGE;
		ev.window = window;
		ev.format = 32;
		ev.type = atoms[WM_PROTOCOLS];
		ev.data.data32[0] = atoms[WM_DELETE_WINDOW];
		ev.data.data32[1] = XCB_CURRENT_TIME;
		
		xcb_send_event(global.connection, 0, window, XCB_EVENT_MASK_NO_EVENT, (char *) &ev);
	} else {
		xcb_kill_client(global.connection, window);
	}
}

void setBorderColor(xcb_window_t window, color_t c) {
	if (global.border.thickness > 0 && window != global.root && window != 0) {
		xcb_change_window_attributes(global.connection, window, XCB_CW_BORDER_PIXEL, (uint32_t[]) {c});
	}
}

void focus(xcb_window_t window) {
	if (window > 0 && window != global.root) {
		xcb_set_input_focus(global.connection, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
		focused = managed(window);
	}
}

void unfocus(xcb_window_t window) {
	if (focused && focused->window == window) {
		xcb_set_input_focus(global.connection, XCB_INPUT_FOCUS_POINTER_ROOT, global.root, XCB_CURRENT_TIME);
		focused = NULL;
	}
}

void spawn(char **command) {
	if (fork() == 0) {
		if (fork() != 0) _exit(0);
		setsid();
		execvp(command[0], command);
		_exit(0);
	}
	wait(NULL);
}

void warpMouseToCenterOfWindow(xcb_window_t window) {
	uint16_t x, y;
	xcb_get_geometry_reply_t *geom = xcb_get_geometry_reply(
		global.connection, xcb_get_geometry(global.connection, window), NULL
	);
	x = geom->width >> 1;
	y = geom->height >> 1;
	free(geom);
	xcb_warp_pointer(global.connection, XCB_NONE, window, 0,0,0,0, x,y);
}

void warpMouseToCenterOfMonitor(monitor_t *monitor) {
	xcb_warp_pointer(
		global.connection, XCB_NONE, global.root, 0,0,0,0,
		monitor->x+(monitor->width>>1),monitor->y+(monitor->height>>1)
	);
}

void lookAtWindow(xcb_window_t window) {
	warpMouseToCenterOfWindow(window);
	focus(window);
}

void die(char *errstr) {
	exit(write(STDERR_FILENO, errstr, strlen(errstr)) < 0 ? -1 : 1);
}