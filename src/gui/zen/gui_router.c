

#include "gfx.h"

#include "gui.h"

#include "gui_router.h"

#include "pages/zen_main_one.h"
#include "pages/zen_main_two.h"


GUIWindow *curWindow = 0;


void guiWindow_onInit(GUIWindow *win, GHandle handle) {
    
    (void) win;

    win->handle = handle;
    //gwinResize(win->handle,w,h); gwinMove(win->handle);
}


void guiWindow_Show(GUIWindow *win) {
    
    if (win == curWindow)
        return;
    
    if (curWindow && curWindow->onClose)
        curWindow->onClose(curWindow);
    
    if (curWindow)
        gwinHide(curWindow->handle);
    
    curWindow = win;
    
    if (curWindow->onShow)
        curWindow->onShow(curWindow);

    gwinShow(curWindow->handle);

}




