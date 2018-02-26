/******************************************************************************/
/* ZEN MENU for µGFX                                                          */
/*                                                                            */
/******************************************************************************/

#ifndef _ZEN_OVERFLOW_H_
#define _ZEN_OVERFLOW_H_

#include "gfx.h"
#include "gui.h"
#include "gui_router.h"

/* LABELS */

/* IMAGES */

/* BUTTONS */
extern GHandle ghBtn_CancelOverflow;
extern GHandle ghBtn_SetOverflow;

/* FUNCTIONS */
void create_PageOverflow(void);
extern GHandle ghContainer_PageOverflow;
extern GUIWindow winOverflowMenu;


#endif /* _ZEN_OVERFLOW_H_ */