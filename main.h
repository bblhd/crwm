#ifndef MAIN_H
#define MAIN_H

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>

/* DEFAULT KEYS
* The following are the possible mask definitions.  Note
* that these definitions may vary between X implementations
* and keyboard models.
*     XCB_MOD_MASK_1       -> Alt_L Alt_R Meta_L
*     XCB_MOD_MASK_2       -> Num_Lock
*     XCB_MOD_MASK_3       -> ISO_Level3_Shift
*     XCB_MOD_MASK_4       -> Super_L Super_R SuperL Hyper_L
*     XCB_MOD_MASK_5       -> ISO_Level5_Shifta
*     XCB_MOD_MASK_SHIFT
*     XCB_MOD_MASK_CONTROL
*     XCB_MOD_MASK_LOCK
*     XCB_MOD_MASK_ANY
*/

#define MOD1 XCB_MOD_MASK_4
#define MOD2 XCB_MOD_MASK_SHIFT

#define BORDER_WIDTH 1
#define BORDER_COLOR_UNFOCUSED 0x9eeeee /* 0xRRGGBB */
#define BORDER_COLOR_FOCUSED   0x55aaaa /* 0xRRGGBB */

extern xcb_connection_t *conn;
extern xcb_screen_t *screen;
extern xcb_drawable_t focusedWindow;
extern xcb_gcontext_t graphical;

extern struct Page testpage;

xcb_query_pointer_reply_t *queryPointer(xcb_drawable_t window);
xcb_get_geometry_reply_t *queryGeometry(xcb_drawable_t window);

#endif
