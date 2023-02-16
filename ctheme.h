#ifndef CTHEME_H
#define CTHEME_H

#include <stdint.h>

typedef uint32_t color_t;

typedef enum {
	COLORSCHEME_DEFAULT,
	COLORSCHEME_SELECTED,
	
	COLORSCHEME_DARK,
	COLORSCHEME_LIGHT,
	COLORSCHEME_RED,
	COLORSCHEME_GREEN,
	COLORSCHEME_YELLOW,
	COLORSCHEME_BLUE,
	COLORSCHEME_CYAN,
	COLORSCHEME_MAGENTA,
	
	COLORSCHEME_NUMBERS,
	COLORSCHEME_STRINGS,
	
	COLORSCHEME_OPERATORS,
	COLORSCHEME_KEYWORDS,
	COLORSCHEME_COMMENTS,
	COLORSCHEME_SPECIAL,
	
	COLORSCHEME_CONSTANTS,
	COLORSCHEME_VARIABLES,
	COLORSCHEME_ARRAYS,
	COLORSCHEME_FUNCTIONS,
	COLORSCHEME_CLASSES,
	
	COLORSCHEME_FINAL
} colorscheme_id_t;

// standard levels are: 0=foreground, 1=background, 2=borders, 3=wm_decorations
typedef uint8_t colorscheme_level_t;
#define REQUISITE_LEVEL ((colorscheme_level_t) -1)

typedef enum {RGB, BGR, RGBA, BGRA, ARGB, ABGR} color_format_t;

void ctheme_clear();
int ctheme_load(char *path);
color_t ctheme_get(colorscheme_id_t, colorscheme_level_t, color_format_t);
void ctheme_set(colorscheme_id_t, colorscheme_level_t, color_t, color_format_t);

#endif
