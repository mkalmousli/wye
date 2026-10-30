/* Routines page parts (page_routines*.c). */
#ifndef WYE_UI_ROUTINES_H
#define WYE_UI_ROUTINES_H

#include "ui.h"

#include "features/routine.h"

void routines_record_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel);
/* Look at the folder again soon, and select this file when it shows up (or NULL). */
void routines_rescan(const char *select_path);

/* The event editor (page_routines_edit.c). */
void routines_edit_open(struct wye_ui *ui, const struct wye_routine_info *sel); /* follows the selection */
void routines_edit_card(struct wye_ui *ui, Rectangle r, const struct wye_routine_info *sel);
void routines_edit_close(void);
bool routines_edit_dirty(void);
/* What an event does, in words (page_routines_text.c). */
const char *routines_event_text(const struct wye_routine_event *ev);

#endif
