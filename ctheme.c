#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "ctheme.h"

struct ColorEntry {
	enum {NO, YES} isRef;
	union {
		color_t exact;
		struct {
			colorscheme_id_t id;
			colorscheme_level_t level;
		} reference;
	};
};

#define MAX_ENTRIES_PER_COLORSCHEME 4
struct ColorEntry colortheme[COLORSCHEME_FINAL][MAX_ENTRIES_PER_COLORSCHEME];

void ctheme_clear() {
	for (colorscheme_id_t id = COLORSCHEME_DEFAULT; id != COLORSCHEME_FINAL; id++) {
		colortheme[id][0] = (struct ColorEntry) {.isRef=YES, .reference={COLORSCHEME_DEFAULT, REQUISITE_LEVEL}};
		for (colorscheme_level_t level = 1; level < MAX_ENTRIES_PER_COLORSCHEME; level++) {
			colortheme[id][level] = (struct ColorEntry) {.isRef=YES, .reference={id, 0}};
		}
	}
	colortheme[COLORSCHEME_DEFAULT][0] = (struct ColorEntry) {.isRef=NO, .exact=0x000000};
}

char *colorschemeNames[COLORSCHEME_FINAL] = {
	"default", "selected",
	"dark", "light", "red", "green", "yellow", "blue", "cyan", "magenta",
	"numbers", "strings",
	"operators", "keywords", "comments", "special",
	"constants", "variables", "arrays", "functions", "classes"
};

void ctheme_stepThroughNewline(char **data) {
	while (**data == '\n') (*data)++;
}

void ctheme_stepThroughWhitespace(char **data) {
	while (**data == ' ' || **data == '\t') (*data)++;
}

colorscheme_id_t ctheme_readID(char **data) {
	ctheme_stepThroughWhitespace(data);
	for (colorscheme_id_t id = COLORSCHEME_DEFAULT; id != COLORSCHEME_FINAL; id++) {
		size_t s = strlen(colorschemeNames[id]);
		if (s <= strlen(*data)) {
			if (strncmp(*data, colorschemeNames[id], s)==0) {
				(*data) += s;
				return id;
			}
		}
	}
	return COLORSCHEME_FINAL;
}

int ctheme_readEntry(char **data, struct ColorEntry *dest) {
	struct ColorEntry entry;
	ctheme_stepThroughWhitespace(data);
	if (**data == '#') {
		entry.isRef = NO;
		(*data)++;
		entry.exact = strtol(*data, data, 16);
	} else {
		entry.isRef = YES;
		
		entry.reference.id = ctheme_readID(data);
		if (entry.reference.id == COLORSCHEME_FINAL) return 0;
		
		if (**data >= '0' && **data <= '9') {
			entry.reference.level = **data - '0';
			if (entry.reference.level > 0) entry.reference.level--;
			(*data)++;
		} else {
			entry.reference.level = REQUISITE_LEVEL;
		}
	}
	*dest = entry;
	return 1;
}

int ctheme_readLine(char **data) {
	ctheme_stepThroughNewline(data);
	colorscheme_id_t id = ctheme_readID(data);
	if (id == COLORSCHEME_FINAL) return 0;
	colorscheme_level_t level = 0;
	while (ctheme_readEntry(data, &colortheme[id][level])) level++;
	return 1;
}

void ctheme_readSettingsFile(char *data) {
	while (ctheme_readLine(&data));
}

FILE *ctheme_openHomeFolderFile(char *file, char *flags) {
	char *home = getenv("HOME");
	if (home) {
		size_t hl = strlen(home);
		size_t fl = strlen(file);
		char path[hl+fl+2];
		memcpy(path, home, hl);
		path[hl] = '/';
		memcpy(path+hl+1, file, fl);
		path[hl+1+fl] = '\0';
		return fopen(path, flags);
	}
	return NULL;
}

int ctheme_load(char *path) {
	FILE *file;
	if (path == NULL) {
		file = ctheme_openHomeFolderFile(".ctheme", "rb");
	} else if (path[0] == '/') {
		file = fopen(path, "rb");
	} else {
		file = ctheme_openHomeFolderFile(path, "rb");
	}
	int good = 0;
	if (file) {
		fseek(file, 0, SEEK_END);
		size_t length = ftell(file);
		rewind(file);
		char *data = malloc(length+1);
		if (data) {
			fread(data, 1, length, file);
			data[length] = 0;
			ctheme_readSettingsFile(data);
			free(data);
			good = 1;
		};
		fclose(file);
	};
	return good;
}

void ctheme_set(colorscheme_id_t id, colorscheme_level_t level, color_t color, color_format_t format) {
	if (level > 0) level--;
	if (level >= MAX_ENTRIES_PER_COLORSCHEME) {
		level = MAX_ENTRIES_PER_COLORSCHEME-1;
	}
	if (id == COLORSCHEME_FINAL) id = COLORSCHEME_DEFAULT;
	
	switch (format) {
		case RGBA:
		case BGRA:
		color = color & 0xffffff;
		break;
		case ARGB:
		case ABGR:
		color = color >> 8;
		default:
	}
	
	switch (format) {
		case BGR:
		case BGRA:
		case ABGR:
		color = ((color&0xff)<<16)
			| (color&0xff00)
			| ((color&0xff0000)>>16);
		break;
		default:
	}
	
	colortheme[id][level].isRef = NO;
	colortheme[id][level].exact = color;
}

color_t ctheme_get(colorscheme_id_t id, colorscheme_level_t level, color_format_t format) {
	if (level > 0) level--;
	if (level >= MAX_ENTRIES_PER_COLORSCHEME) {
		level = MAX_ENTRIES_PER_COLORSCHEME-1;
	}
	if (id == COLORSCHEME_FINAL) id = COLORSCHEME_DEFAULT;
	
	colorscheme_level_t baseLevel = level;
	
	struct ColorEntry gotten = colortheme[id][level];
	while (gotten.isRef) {
		if (gotten.reference.id == id && gotten.reference.level == level) {
			id = COLORSCHEME_DEFAULT;
			level = 0;
		} else if (gotten.reference.id == id && gotten.reference.level == REQUISITE_LEVEL) {
			level = 0;
		} else if (gotten.reference.level == REQUISITE_LEVEL) {
			id = gotten.reference.id;
			level = baseLevel;
		} else {
			id = gotten.reference.id;
			level = gotten.reference.level;
		}
		gotten = colortheme[id][level];
	}
	
	color_t c = gotten.exact;
	
	switch (format) {
		case BGR:
		case BGRA:
		case ABGR:
		c = ((c&0xff)<<16)
			| (c&0xff00)
			| ((c&0xff0000)>>16);
		break;
		default:
	}
	
	switch (format) {
		case RGBA:
		case BGRA:
		c = c | 0xff000000;
		break;
		case ARGB:
		case ABGR:
		c = (c<<8) | 0xff;
		default:
	}
	
	return c;
}
