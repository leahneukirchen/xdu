/*
 * XDU - X Window System Interface.
 *
 * We hide all of the X hieroglyphics inside of this module.
 *
 * Phillip C. Dykstra
 * <phil@arl.mil>
 * 4 Sep 1991.
 * 
 * Copyright (c)	Phillip C. Dykstra	1991, 1993, 1994
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons who receive
 * copies from any such party to do so.  This license includes without
 * limitation a license to do the foregoing actions under any patents of
 * the party supplying this software to the X Consortium.
 */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/AsciiSrc.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>	/* for exit() */

/* IMPORTS: routines that this module vectors out to */
int press(int x, int y);
int reset(void);
int repaint(int width, int height);
int setorder(char *op);
int reorder(char *op);
int nodeinfo(void);
int helpinfo(void);
extern int ncols;

/* internal routines */
static void help_popup();
static void help_popdown();
void xrepaint(void);
void xrepaint_noclear(void);

static String fallback_resources[] = {
"*window.width:		600",
"*window.height:	480",
"*help.width:		500",
"*help.height:		330",
"*order:		first",
NULL
};

/* Application Resources */
typedef struct {
	Pixel	foreground;
	Pixel	background;
	XFontStruct *font;
	int	ncol;
	Boolean	showsize;
	Boolean	human;
	char	*order;
} res_data, *res_data_ptr;
static res_data res;

static XtResource application_resources[] = {
	{ XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
		XtOffset(res_data_ptr,foreground), XtRString, XtDefaultForeground},
	{ XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
		XtOffset(res_data_ptr,background), XtRString, XtDefaultBackground},
	{ XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
		XtOffset(res_data_ptr,font), XtRString, XtDefaultFont },
	{ "ncol", "Ncol", XtRInt, sizeof(int),
		XtOffset(res_data_ptr,ncol), XtRString, "5"},
	{ "showsize", "ShowSize", XtRBoolean, sizeof(Boolean),
		XtOffset(res_data_ptr,showsize), XtRString, "True"},
	{ "human", "Human", XtRBoolean, sizeof(Boolean),
		XtOffset(res_data_ptr,human), XtRString, "True"},
	{ "order", "Order", XtRString, sizeof(String),
		XtOffset(res_data_ptr,order), XtRString, "first"}
};

/* Command Line Options */
static XrmOptionDescRec options[] = {
	{"-c",		"*ncol",	XrmoptionSepArg,	NULL},
	{"+s",		"*showsize",	XrmoptionNoArg,		"True"},
	{"-s",		"*showsize",	XrmoptionNoArg,		"False"},
	{"+h",		"*human",	XrmoptionNoArg,		"True"},
	{"-h",		"*human",	XrmoptionNoArg,		"False"},
	{"-n",		"*order",	XrmoptionNoArg,		"size"},
	{"-rn",		"*order",	XrmoptionNoArg,		"rsize"},
	{"-a",		"*order",	XrmoptionNoArg,		"alpha"},
	{"-ra",		"*order",	XrmoptionNoArg,		"ralpha"}
};

/* action routines */
static void a_goto();
static void a_reset();
static void a_quit();
static void a_reorder();
static void a_size();
static void a_human();
static void a_ncol();
static void a_info();
static void a_help();
static void a_removehelp();

static XtActionsRec actionsTable[] = {
	{ "reset",	a_reset },
	{ "goto",	a_goto },
	{ "quit",	a_quit },
	{ "reorder",	a_reorder },
	{ "size",	a_size },
	{ "human",	a_human },
	{ "ncol",	a_ncol },
	{ "info",	a_info },
	{ "help",	a_help },
	{ "RemoveHelp",	a_removehelp }
};

static char defaultTranslations[] = "\
<Key>Q:	quit()\n\
<Key>Escape: quit()\n\
:<Key>/: reset()\n\
<Key>S:	size()\n\
<Key>H:	human()\n\
<Key>I:	info()\n\
<Key>Help: help()\n\
:<Key>?: help()\n\
<Key>A:	reorder(alpha)\n\
<Key>N:	reorder(size)\n\
<Key>F:	reorder(first)\n\
<Key>L:	reorder(last)\n\
<Key>R:	reorder(reverse)\n\
<Key>1:	ncol(1)\n\
<Key>2:	ncol(2)\n\
<Key>3:	ncol(3)\n\
<Key>4:	ncol(4)\n\
<Key>5:	ncol(5)\n\
<Key>6:	ncol(6)\n\
<Key>7:	ncol(7)\n\
<Key>8:	ncol(8)\n\
<Key>9:	ncol(9)\n\
<Key>0:	ncol(10)\n\
<Btn1Down>: goto()\n\
<Btn2Down>: reset()\n\
<Btn3Down>: quit()\n\
";

/*  action routines  */

static void
a_quit(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	XtDestroyApplicationContext(XtWidgetToApplicationContext(w));
	exit(0);
}

static void
a_goto(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	press(event->xbutton.x, event->xbutton.y);
}

static void
a_reset(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	reset();
}

static void
a_reorder(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	if (*num_params != 1) {
		fprintf(stderr, "xdu: bad number of params to reorder action\n");
	} else {
		reorder(*params);
	}
}

static void
a_size(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	if (res.showsize)
		res.showsize = 0;
	else
		res.showsize = 1;
	xrepaint();
}

static void
a_human(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	if (res.human)
		res.human = 0;
	else
		res.human = 1;
	xrepaint();
}

static void
a_ncol(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	int	n;

	if (*num_params != 1) {
		fprintf(stderr, "xdu: bad number of params to ncol action\n");
		return;
	}
	n = atoi(*params);
	if (n < 1 || n > 1000) {
		fprintf(stderr, "xdu: bad value to ncol action\n");
		return;
	}
	ncols = res.ncol = n;
	xrepaint();
}

static void
a_info(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	nodeinfo();
}

static void
a_help(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	/*helpinfo();*/
	help_popup();
}

static void
a_removehelp(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
	help_popdown();
}

/* callback routines */

static void
c_resize(Widget w, XtPointer data, XEvent *event, Boolean *continue_to_dispatch)
{
	/*printf("Resize\n");*/
	xrepaint();
}

static void
c_repaint(Widget w, XtPointer data, XEvent *event, Boolean *continue_to_dispatch)
{
	/*printf("Expose\n");*/
	xrepaint_noclear();
}

/* X Window related variables */
static Display *dpy;
static int screen;
static Visual *vis;
static Window win;
static GC gc;
static XtAppContext app_con;

Widget toplevel;

/*  External Functions  */

void
xsetup(int *argcp, char **argv)
{
	XtTranslations trans_table;
	Widget w;
	XGCValues gcv;
	int n;
	Arg args[5];

	/* Create the top level Widget */
	n = 0;
	XtSetArg(args[n], XtNtitle, "XDU Disk Usage Display ('h' for help)\n"); n++;
	toplevel = XtAppInitialize(&app_con, "XDu",
			options, XtNumber(options),
			argcp, argv,
			fallback_resources, args, n);

	XtGetApplicationResources(toplevel, (XtPointer)&res,
		application_resources, XtNumber(application_resources),
		NULL, 0 );

	XtAppAddActions(app_con, actionsTable, XtNumber(actionsTable));
	trans_table = XtParseTranslationTable(defaultTranslations);

	/* Create a simple Label class widget to draw in */
	n = 0;
	XtSetArg(args[n], XtNlabel, ""); n++;
	w = XtCreateManagedWidget("window", labelWidgetClass, toplevel,
		args, n);

	/* events */
	XtAddEventHandler(w, ExposureMask, False, c_repaint, NULL);
	XtAddEventHandler(w, StructureNotifyMask, False, c_resize, NULL);
	XtAugmentTranslations(w, trans_table);

	XtRealizeWidget(toplevel);

	/* We need these for the raw Xlib calls */
	win = XtWindow(w);
	dpy = XtDisplay(w);
	screen = DefaultScreen(dpy);
	vis = DefaultVisual(dpy,screen);

	gcv.foreground = res.foreground;
	gcv.background = res.background;
	gcv.font = res.font->fid;
	gc = XCreateGC(dpy, win, (GCFont|GCForeground|GCBackground), &gcv);

	setorder(res.order);
	ncols = res.ncol;
}

int
xmainloop(void)
{
	XtAppMainLoop(app_con);
	return(0);
}

void
xclear(void)
{
	XClearWindow(dpy, win);
}

void
xrepaint(void)
{
	XWindowAttributes xwa;

	XClearWindow(dpy, win);
	XGetWindowAttributes(dpy, win, &xwa);
	repaint(xwa.width, xwa.height);
}

void
xrepaint_noclear(void)
{
	XWindowAttributes xwa;

	XGetWindowAttributes(dpy, win, &xwa);
	repaint(xwa.width, xwa.height);
}

void
xdrawrect(char *name, off_t size, int x, int y, int width, int height)
{
	int	textx, texty;
	char	label[1024];
	XCharStruct overall;
	int	ascent, descent, direction;
	int	cheight;

	/*printf("draw(%d,%d,%d,%d)\n", x, y, width, height );*/
	XDrawRectangle(dpy, win, gc, x, y, width, height);

	if (res.showsize) {
		if (res.human) {
			double d = size;
			const char *u = "KMGTPEZY";

			while (d >= 1024) {
				u++;
				d /= 1024.0;
			}
			sprintf(label,"%s (%.1f%c)", name, d, *u);
		} else {
			sprintf(label,"%s (%jd)", name, (intmax_t)size);
		}

		name = label;
	}

	XTextExtents(res.font, name, strlen(name), &direction,
		&ascent, &descent, &overall);
	cheight = overall.ascent + overall.descent;
	if (height < (cheight + 2))
		return;

	/* print label */
	textx = x + 4;
	texty = y + height/2.0 + (overall.ascent - overall.descent)/2.0 + 1.5;
	XDrawString(dpy, win, gc, textx, texty, name, strlen(name));
}

static Widget popup;

static void
help_popup(void)
{
	Widget form, text, src;
	Arg args[15];
	int n;
	Atom wm_delete_window;
	XtTranslations trans_table;

	if (popup != NULL) {
		XtPopup(popup, XtGrabNone);
		return;
	}

	/* popup shell */
	n = 0;
	XtSetArg(args[n], XtNtitle, "XDU Help"); n++;
	popup = XtCreatePopupShell("helpPopup", transientShellWidgetClass,
		toplevel, args, n);

	/* form container */
	n = 0;
	XtSetArg(args[n], XtNborderWidth, 0); n++;
	XtSetArg(args[n], XtNdefaultDistance, 0); n++;
	form = XtCreateManagedWidget("form", formWidgetClass,
		popup, args, n);

	/* text widget in form */
	n = 0;
	XtSetArg(args[n], XtNborderWidth, 0); n++;
	XtSetArg(args[n], XtNresize, XawtextResizeBoth); n++;
	/* fallback resources weren't working here on the Sun */
	XtSetArg(args[n], XtNwidth, 500); n++;
	XtSetArg(args[n], XtNheight, 330); n++;
	text = XtCreateManagedWidget("help", asciiTextWidgetClass,
		form, args, n);

	/* create text source */
	n = 0;
	XtSetArg(args[n], XtNtype, XawAsciiString); n++;
	XtSetArg(args[n], XtNeditType, XawtextRead); n++;
	XtSetArg(args[n], XtNstring, "\
XDU Version 3.0 - Phil Dykstra <phil@arl.mil>\n\
\n\
Keyboard Commands\n\
  a  sort alphabetically\n\
  n  sort numerically (largest first)\n\
  f  sort first-in-first-out\n\
  l  sort last-in-first-out\n\
  r  reverse sort\n\
  s  toggle size display\n\
  h  toggle human readable sizes\n\
  /  goto the root\n\
  i  node info to standard out\n\
  ?  this help message\n\
  q  quit (also Escape)\n\
0-9  set number of columns (0=10)\n\
\n\
Mouse Commands\n\
  Left   Goto node (goto parent if leftmost box)\n\
  Middle Back to root\n\
  Right  Quit\n\
"); n++;
	src = XtCreateWidget("textSource", asciiSrcObjectClass,
		text, args, n);
	/* set text source */
	XawTextSetSource(text, src, 0);

	XtRealizeWidget(popup);
	XtPopup(popup, XtGrabNone);

	trans_table = XtParseTranslationTable("<Key>Q: RemoveHelp()");
	XtAugmentTranslations(form, trans_table);

	/* Set up ICCCM delete window */
	wm_delete_window = XInternAtom(XtDisplay(popup), "WM_DELETE_WINDOW", False);
	XtOverrideTranslations(popup, XtParseTranslationTable("<Message>WM_PROTOCOLS: RemoveHelp()"));
	XSetWMProtocols(XtDisplay(popup), XtWindow(popup), &wm_delete_window, 1);
}

static void
help_popdown(void)
{
	XtPopdown(popup);
}
