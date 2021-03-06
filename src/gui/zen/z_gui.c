/*
 * Copyright (c) 2012, 2013, Joel Bodenmann aka Tectu <joel@unormal.org>
 * Copyright (c) 2012, 2013, Andrew Hannam aka inmarket
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>

#include "gfx.h"
#include "gui.h"

#ifdef UGFXSIMULATOR

	#include <stdlib.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <linux/input.h>

#else

	#include "calibration.h"

	#include "stm32f4xx_hal.h"
	#include "cmsis_os.h"
	#include "XCore407I.h"

	#include "rtc_clock.h"

#endif

#include "vt100.h"

#include "skin/zenstyle.h"
#include "pages/zen_menu.h"





#if GFX_USE_GWIN

/* The variables we need */
GListener glistener;

// GHandles
GHandle ghContainerPage1;

GHandle ghNavBar;
GHandle ghContainer;
GHandle ghTopBar;
GHandle ghFooter;

GHandle ghButton1;
GHandle ghButton2;
GHandle ghButton3;
GHandle ghButton4;
GHandle ghButton5;
GHandle ghButton6;
GHandle ghButton7;
GHandle ghButton8;

GHandle ghConsole;

GHandle ghImagebox1;

GHandle ghlabel1;

// Fonts
//font_t dejavu_sans_10;
font_t dejavu_sans_16;
font_t fixed_7x14;

// Icons
gdispImage ic_home;
gdispImage ic_settings;
gdispImage ic_forward;
gdispImage ic_local_drink;
gdispImage ic_alarm;
gdispImage ic_public;
gdispImage ic_thumbs_up_down;
gdispImage ic_search;
gdispImage ic_live_help;

gdispImage chibios;
gdispImage testbild;




static void createWidgets(void) {

	GWidgetInit		wi;
	coord_t			pagewidth;

	pagewidth = gdispGetWidth()/2;

	gwinWidgetClearInit(&wi);

	// create container widget: ghContainerPage1
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
	ghContainerPage1 = gwinContainerCreate(0, &wi, 0);

#if 0
	// create container widget: ghTopBar
	wi.g.show = FALSE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = 320;
	wi.g.height = 24;
	wi.g.parent = ghContainerPage1;
	wi.text = "TopBar";
	wi.customDraw = gwinContainerDraw_Std;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghTopBar = gwinContainerCreate(0, &wi, 0);


	// create container widget: ghFooter
	wi.g.show = FALSE;
	wi.g.x = 0;
	wi.g.y = 216;
	wi.g.width = 320;
	wi.g.height = 24;
	wi.g.parent = ghContainerPage1;
	wi.text = "Footer";
	wi.customDraw = gwinContainerDraw_Std;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghFooter = gwinContainerCreate(0, &wi, 0);
#endif

	gdispFillArea(0, 0, 320, 24, HTML2COLOR(0x262626) );

	gdispFillArea(0, 216, 320, 240, HTML2COLOR(0x262626) );

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_CONFIGURATION;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_settings;
	wi.customStyle = &color_one;
	ghButton1 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 80;
	wi.g.y = 0;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_CLEANING;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_local_drink;
	wi.customStyle = &color_two;
	ghButton2 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 160;
	wi.g.y = 0;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_TIMERS;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_alarm;
	wi.customStyle = &color_five;
	ghButton3 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 240;
	wi.g.y = 0;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_NETWORK;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_public;
	wi.customStyle = &color_four;
	ghButton4 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 96;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_TESTMODE;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_thumbs_up_down;
	wi.customStyle = &color_six;
	ghButton5 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 80;
	wi.g.y = 96;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_STATUS;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_search;
	wi.customStyle = &color_eight;
	ghButton6 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 160;
	wi.g.y = 96;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_HELP;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_live_help;
	wi.customStyle = &color_seven;
	ghButton7 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 240;
	wi.g.y = 96;
	wi.g.width = 80;
	wi.g.height = 96;
	wi.g.parent = ghContainerPage1;
	wi.text = MENU_TITLE_NEXT;
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_forward;
	wi.customStyle = &color_three;
	ghButton8 = gwinButtonCreate(0, &wi);





#if 0
	// create container widget: ghFooter
	wi.g.show = TRUE;
	wi.g.x = 50;
	wi.g.y = 210;
	wi.g.width = 270;
	wi.g.height = 30;
	wi.g.parent = ghContainerPage1;
	wi.text = "Footer";
	wi.customDraw = gwinContainerDraw_Std;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghFooter = gwinContainerCreate(0, &wi, 0);

	// create container widget: ghTopBar
	wi.g.show = TRUE;
	wi.g.x = 50;
	wi.g.y = 0;
	wi.g.width = 270;
	wi.g.height = 30;
	wi.g.parent = ghContainerPage1;
	wi.text = "TopBar";
	wi.customDraw = gwinContainerDraw_Std;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghTopBar = gwinContainerCreate(0, &wi, 0);


	// create container widget: ghNavBar
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = 50;
	wi.g.height = 240;
	wi.g.parent = ghContainerPage1;
	wi.text = "NavBar";
	wi.customDraw = gwinContainerDraw_Std;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghNavBar = gwinContainerCreate(0, &wi, 0);


	// create container widget: ghContainer
	wi.g.show = TRUE;
	wi.g.x = 50;
	wi.g.y = 30;
	wi.g.width = 270;
	wi.g.height = 180;
	wi.g.parent = ghNavBar;
	wi.text = "Container";
	wi.customDraw = gwinContainerDraw_Std;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghContainer = gwinContainerCreate(0, &wi, 0);




	// create button widget: ghButton1
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 0;
	wi.g.width = 50;
	wi.g.height = 48;
	wi.g.parent = ghNavBar;
	wi.text = "";
	wi.customDraw = gwinButtonDraw_Image_Icon;
	wi.customParam = &ic_home;
	wi.customStyle = 0;
	ghButton1 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton2
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 48;
	wi.g.width = 50;
	wi.g.height = 48;
	wi.g.parent = ghNavBar;
	wi.text = "2";
	wi.customDraw = gwinButtonDraw_Clear;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghButton2 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton3
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 96;
	wi.g.width = 50;
	wi.g.height = 48;
	wi.g.parent = ghNavBar;
	wi.text = "3";
	wi.customDraw = gwinButtonDraw_Clear;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghButton3 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton4
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 144;
	wi.g.width = 50;
	wi.g.height = 48;
	wi.g.parent = ghNavBar;
	wi.text = "4";
	wi.customDraw = gwinButtonDraw_Clear;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghButton4 = gwinButtonCreate(0, &wi);

	// create button widget: ghButton5
	wi.g.show = TRUE;
	wi.g.x = 0;
	wi.g.y = 192;
	wi.g.width = 50;
	wi.g.height = 48;
	wi.g.parent = ghNavBar;
	wi.text = "5";
	wi.customDraw = gwinButtonDraw_Clear;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghButton5 = gwinButtonCreate(0, &wi);



#endif



	/* Console */
	wi.g.show = FALSE;
	wi.g.x = pagewidth;
	wi.g.y = gdispGetHeight()/2;
	wi.g.width = gdispGetWidth()/2;
	wi.g.height = gdispGetHeight()/2;
	wi.g.parent = 0;
	ghConsole = gwinConsoleCreate(0, &wi.g);

	gwinSetColor(ghConsole, Black);
	gwinSetBgColor(ghConsole, HTML2COLOR(0xF0F0F0));

#if 0
	/* Label for time */
	wi.g.show = TRUE;
	wi.g.x = pagewidth;
	wi.g.y = 0;
	wi.g.width = gdispGetWidth()/2 - 3;
	wi.g.height = 20;
	wi.g.parent = ghTopBar;
	wi.text = "00:00:00";
	wi.customDraw = gwinLabelDrawJustifiedRight;
	wi.customParam = 0;
	wi.customStyle = 0;
	ghlabel1 = gwinLabelCreate(0, &wi);

	gwinSetColor(ghlabel1, White);
	gwinSetBgColor(ghlabel1, Black);
	gwinLabelSetBorder(ghlabel1, FALSE);
#endif


#if 0
	// create button widget: ghImagebox1
	wi.g.show = TRUE;
	wi.g.x = 50;
	wi.g.y = 0;
	wi.g.width = 90;
	wi.g.height = 90;
	wi.g.parent = 0;
	ghImagebox1 = gwinImageCreate(0, &wi.g);
	gwinImageOpenFile(ghImagebox1, "rsc/chibios.gif");

	// Image
	wi.g.show = TRUE;
	wi.g.x = 140;
	wi.g.y = 0;
	wi.g.width = 180;
	wi.g.height = 50;
	wi.g.parent = 0;
	ghImagebox1 = gwinImageCreate(0, &wi.g);
	gwinImageOpenFile(ghImagebox1, "ugfx.bmp");
#endif

}



static void createShell(void) {

	gdispClear(Black);

#if 1

									/*|123456789012345678901234567890123456789012345|*/
									/*|....'....1....'....2....|....3....'....4....'|*/
	vt100_puts("\e[1;1H"); vt100_puts("STM32-HAL FreeRTOS(TM) Terminal CMSIS v0.1.1");
	vt100_puts("\e[2;1H"); vt100_puts("ARM Cortex M4 64kB");

	vt100_puts("\e[4;1H"); /* vt100_puts(">"); */
#endif
}


























#ifdef UGFXSIMULATOR
void myguiEventLoop(void) {
#else
void guiEventLoop(void) {
#endif


    //static char showtime[16] = {0};
    //static char showdate[16] = {0};

    //RTC_CalendarShow(showtime, showdate);

    //gdispFillStringBox(320/2, 0, 320/2,  20, showtime, dejavu_sans_16, White, Black, justifyLeft);
    //gdispFillString(5, 0, showtime, dejavu_sans_10, White, Black);
    
    //gdispFillString(5, 20, showdate, dejavu_sans_10, White, Black);

    //osDelay(1000);

#if 0
	RTC_TimeTypeDef stimestructureget;
	//RTC_DateTypeDef sdatestructureget;

	/* Get the RTC current Time */
	HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
	/* Get the RTC current Date */
	//HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);
	/* Display time Format : hh:mm:ss */
	sprintf(showtime,"%02d:%02d:%02d", stimestructureget.Hours, stimestructureget.Minutes, stimestructureget.Seconds);
	/* Display date Format : mm-dd-yy */
	//sprintf((char*)showdate,"%02d-%02d-%02d", sdatestructureget.Month, sdatestructureget.Date, 2000 + sdatestructureget.Year);
#endif

#if 1
	GEvent *pe;

	// WAITS for an Event - is blocking !!!
	pe = geventEventWait(&glistener, TIME_INFINITE);

	GEventGWinButton  *peb = (GEventGWinButton *)pe;

	switch(pe->type) {

		case GEVENT_GWIN_BUTTON:

			gwinPrintf(ghConsole, "Button %s\n", gwinGetText(((GEventGWinButton *)pe)->gwin));
			

			/* CONFIG */
			if (peb->gwin == ghButton1) {
				fprintf(stderr, "CONFIG:\n");
			} 

			/* CLEAN */
			else if (peb->gwin == ghButton2) {
				fprintf(stderr, "CLEAN:\n");
			}



			/* NEXT */
			else if (peb->gwin == ghButton8) {
				fprintf(stderr, "Next page..\n");
			}




			break;



		default:
			gwinPrintf(ghConsole, "Unknown %d\n", pe->type);
			break;
	}

#endif

}



void guiOutput(char *str) {
	/* Function to print the answers from the vt100 term, like reports and queries */
	/* Is called like: term->send_response("\e[?1;0c"); */

	(void) str;
	//vt100_puts(str);
}


#ifdef UGFXSIMULATOR
int main(void) {

	// Initialize the display (for simulator)
	gfxInit();

#else
void guiCreate(void) {
#endif	

#if 1

	//vt100_init(guiOutput);

	// Prepare fonts
	//dejavu_sans_10 = gdispOpenFont("DejaVuSans10");
	dejavu_sans_16 = gdispOpenFont("DejaVuSans16");
	fixed_7x14     = gdispOpenFont("Fixed7x14");

	gdispClear(Black);

	//createShell();

	// Show SplashScreen
	//zen_splash();




#if 1
	// Prepare images
	gdispImageOpenFile(&ic_home,           "rsc/ic_home.gif");
	gdispImageOpenFile(&ic_settings,       "rsc/ic_settings.gif");
	gdispImageOpenFile(&ic_forward,        "rsc/ic_forward.gif");
	gdispImageOpenFile(&ic_local_drink,    "rsc/ic_local_drink.gif");
	gdispImageOpenFile(&ic_alarm,          "rsc/ic_alarm.gif");
	gdispImageOpenFile(&ic_public,         "rsc/ic_public.gif");
	gdispImageOpenFile(&ic_thumbs_up_down, "rsc/ic_thumbs_up_down.gif");
	gdispImageOpenFile(&ic_search,         "rsc/ic_search.gif");
	gdispImageOpenFile(&ic_live_help,      "rsc/ic_live_help.gif");

	gdispImageOpenFile(&chibios,     "rsc/chibios.gif");
	gdispImageOpenFile(&testbild,    "rsc/testbild.gif");

	// Set the widget defaults
	gwinSetDefaultFont(dejavu_sans_16);
	gwinSetDefaultStyle(&BlackWidgetStyle, FALSE);

	
	// Create the gwin windows/widgets
	createWidgets();
	

	// Make the console visible
	//gwinShow(ghConsole);
	//gwinClear(ghConsole);

    // We want to listen for widget events
	geventListenerInit(&glistener);
	gwinAttachListener(&glistener);

    // Assign toggles and dials to specific buttons & sliders etc.
	#if GINPUT_NEED_TOGGLE
		gwinAttachToggle(ghButton1, 0, 0);
		gwinAttachToggle(ghButton2, 0, 1);
		gwinAttachToggle(ghButton3, 0, 2);
		gwinAttachToggle(ghButton4, 0, 3);
		gwinAttachToggle(ghButton5, 0, 4);
		gwinAttachToggle(ghButton6, 0, 5);
		gwinAttachToggle(ghButton7, 0, 6);
		gwinAttachToggle(ghButton8, 0, 7);
	#endif

	gwinShow(ghContainerPage1);
#endif

#else

	static gdispImage myImage;
	static gdispImage myImage2;

	gdispImageOpenFile(&myImage, "rsc/chibios.gif");
	gdispImageOpenFile(&myImage2, "rsc/testbild.gif");

	gdispImageDraw(&myImage, 0, 0, 320, 240, 0, 0);
	gdispImageDraw(&myImage2, 100, 0, 320, 240, 0, 0);

	//LOG_UART("used: %ld", myImage.memused);
	//LOG_UART("max:  %ld", myImage.maxmemused);

	//LOG_UART("used: %ld", myImage2.memused);
	//LOG_UART("max:  %ld", myImage2.maxmemused);

	gdispImageClose(&myImage);
	gdispImageClose(&myImage2);

#endif



#ifdef UGFXSIMULATOR

	#if 1
	uint8_t c;
	while(1) {
		myguiEventLoop();

		//c = getchar();
		//vt100_putc(c);
	    //fflush(stdout);
	}
	#endif
	   
	#if 0
	uint8_t c;
	while((c=getchar()) != EOF)      
	{
	    //putchar(c);
		//vt100_test(c);
		vt100_putc(c);
	}
	#endif

	return 0;

#endif

}















#else  /* NOT USING GFX_USE_GWIN - BARE TESTING ROUTINE */

/* Dummy functions for testing and debugging ... */
void guiEventLoop(void) { }


#ifdef UGFXSIMULATOR
int main(void) {

  /* Start µGFX */
  gfxInit();

  gdispSetBacklight(100);
  gdispSetContrast(100);

  //geventListenerInit(&glistener);
  //gwinAttachListener(&glistener);

  //guiCreate();

  static gdispImage myImage;

  gdispImageOpenFile(&myImage, "rsc/chibios.gif");
  //gdispImageOpenFile(&myImage, "rsc/testbild.gif");

  gdispImageDraw(&myImage, 0, 0, 320, 240, 0, 0);

  printf("used: %u\n", myImage.memused);
  printf("max:  %u\n", myImage.maxmemused);

  gdispImageClose(&myImage);

  gdispImageDraw(&myImage, 0, 0, myImage.width, myImage.height, 0, 0);

  while(1) {};
}
#endif



#endif

