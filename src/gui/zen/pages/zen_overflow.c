#include <stdio.h>
#include <string.h>

#include "gfx.h"
#include "gui.h"
#include "src/gwin/gwin_keyboard_layout.h"

#ifdef UGFXSIMULATOR

	#include <stdlib.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <errno.h>
	//#include <linux/input.h>

#else

	#include "stm32f4xx_hal.h"
	#include "cmsis_os.h"
	#include "XCore407I.h"

#endif

#include "zen_menu.h"
#include "skin/zenstyle.h"
#include "gui_router.h"

#include "pages/zen_overflow.h"
#include "pages/zen_main_two.h"
#include "helpers/zen_util.h"


/* PAGE CONTAINER */
GHandle ghContainer_PageOverflow;


/* BUTTONS */
GHandle ghBtn_CancelOverflow;
GHandle ghBtn_SetOverflow;

/* IMAGES */


/* EVENT LISTENER */
static GListener gl;

/* LABELS */

/* INPUT FIELDS AND KEYBOARD */
static GHandle ghTexteditOverflow;
static GHandle ghKeyboard;

//Keyboard layouts
static const GVSpecialKey KeyboardSpecialKeys[] = {
	{ "\001", 0, GVKEY_SINGLESET, 1 },				// \001 (1)	= Shift Lower to Upper
	{ "\001", 0, GVKEY_INVERT|GVKEY_LOCKSET, 2 },	// \002 (2)	= Shift Upper to Upper Lock
	{ "\002", 0, GVKEY_INVERT|GVKEY_LOCKSET, 0 },	// \003 (3)	= Shift Upper Lock to Lower
	{ "123", 0, GVKEY_LOCKSET, 3 },					// \004 (4)	= Change to Numbers
	{ "\010", "\b", 0, 0 }							// \005 (5)	= Backspace
};

//NumPad with digits only (with none of the above)
static const char *numpadKeySetArray[] = { "423", "10\005", 0 };
static const GVKeySet numpadKeySet[] = { numpadKeySetArray, 0 };
static const GVKeyTable numpadKeyboard = { KeyboardSpecialKeys, numpadKeySet };


void create_PageOverflow(void) {

	GWidgetInit wi;

	gwinWidgetClearInit(&wi);

	// create container widget: ghContainer_PageOverflow
	wi.g.show = FALSE;
	wi.g.x = 0;
	wi.g.y = 24;
	wi.g.width = 320;
	wi.g.height = 192;
	wi.g.parent = 0;
	wi.text = "Container";
	wi.customDraw = 0;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghContainer_PageOverflow = gwinContainerCreate(0, &wi, 0);

	// HANDLE THE PRESS OF THE DELETE KEY WHEN NO TextEdit IS SELECTED
	// Create the keyboard
	wi.g.show = TRUE;
	wi.g.x = 0; 
	wi.g.y = 58;
	wi.g.width = gdispGetWidth() / 2 + 80; 
	wi.g.height = gdispGetHeight() / 2 + 22;
	wi.g.parent = ghContainer_PageOverflow;
	ghKeyboard = gwinKeyboardCreate(0, &wi);

	// TexteditOverflow
	wi.g.show = TRUE;
	wi.g.x = 135;
	wi.g.y = 5;
	wi.g.width = 50;
	wi.g.height = 30;
	wi.g.parent = ghContainer_PageOverflow;
	wi.text = "";
	ghTexteditOverflow = gwinTexteditCreate(0, &wi, 1);

	
	// create button widget: ghBtn_SetMembrane
	wi.g.show = TRUE;
	wi.g.x = gdispGetWidth() - 80;
	wi.g.y = 58;
	wi.g.width = 80;
	wi.g.height = 69;
	wi.g.parent = ghContainer_PageOverflow;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_done;
	wi.customStyle = &color_eight;
	ghBtn_SetOverflow = gwinButtonCreate(0, &wi);

	// create button widget: ghBtn_CancelMembrane
	wi.g.show = TRUE;
	wi.g.x = gdispGetWidth() - 80;
	wi.g.y = 127;
	wi.g.width = 80;
	wi.g.height = 69;
	wi.g.parent = ghContainer_PageOverflow;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_cancel;
	wi.customStyle = &color_five;
	ghBtn_CancelOverflow = gwinButtonCreate(0, &wi);

	geventListenerInit(&gl);
	geventAttachSource(&gl, ginputGetKeyboard(0), 0);
	gwinKeyboardSetLayout(ghKeyboard, &numpadKeyboard);
	
	
}


static void guiOverflowMenu_onShow(GUIWindow *win) {

	gui_set_title(win);

	char suctionOverflowAsArray[2];
    snprintf(suctionOverflowAsArray, sizeof(suctionOverflowAsArray), "%u", get_suction_overflow());

   	gwinSetText(ghTexteditOverflow, suctionOverflowAsArray, TRUE);

   	// This function was manually added to the µGFX library under ugfx/src/gwin/gwin_textedit.h
   	// Move the cursor away so it is not visible. We don't actually need it here, because we edit the text
   	// of the TextEdit with the help of the buttons and not with the keyboard 
   	gwinTextEditSetCursorPosition((GTexteditObject*) ghTexteditOverflow, strlen(suctionOverflowAsArray));

}

static void guiOverflowMenu_onClose(GUIWindow *win) {

	(void) win;

}

static int guiOverflowMenu_handleEvent(GUIWindow *win, GEvent *pe) {
    
    (void) win;

    switch (pe->type) {

        case GEVENT_GWIN_BUTTON: {

        	GEventGWinButton *peb = (GEventGWinButton *)pe;

            if(peb->gwin == ghBtn_CancelOverflow) {

            	guiWindow_Show(&winMainMenuTwo);

            } else if(peb->gwin == ghBtn_SetOverflow) {

            	
            	if(strlen(gwinGetText(ghTexteditOverflow)) < 1) {
            		return 1;
            	}

            	unsigned short suctionOverflow;

            	sscanf(gwinGetText(ghTexteditOverflow), "%hu", &suctionOverflow);

            	set_suction_overflow((uint8_t) suctionOverflow);

            	guiWindow_Show(&winMainMenuTwo);

            }

            return 1;
        }

        break;
    }
	
    return 0;
	
}



GUIWindow winOverflowMenu = {

/* Title   */	 "Suction Overflow in weeks (0 - 4)",
/* onInit  */    guiWindow_onInit,
/* onShow  */    guiOverflowMenu_onShow,
/* onClose */    guiOverflowMenu_onClose,
/* onEvent */    guiOverflowMenu_handleEvent,
/* handle  */    0

};