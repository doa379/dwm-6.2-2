/* See LICENSE file for copyright and license details. */

#include "XF86keysym.h"
#define BH_PADDING -1
#define STEXTDELIM "|"

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int showsystray        = 1;     /* 0 means no systray */
static const bool showbar           = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "Fixed:pixelsize=14" };
static const char dmenufont[]       = "Sans:pixelsize=20";
static const char col_gray10[]      = "#1a1a1a";
static const char col_gray20[]      = "#333333";
static const char col_gray30[]      = "#4d4d4d";
static const char col_gray40[]      = "#666666";
static const char col_gray50[]      = "#808080";
static const char col_gray60[]      = "#999999";
static const char col_gray70[]      = "#b3b3b3";
static const char col_gray80[]      = "#cccccc";
static const char col_gray90[]      = "#e6e6e6";
static const char col_cyan[]        = "#005577";
static const char col_cyan30[]      = "#006e99";
static const char col_cyan40[]      = "#0092cc";
static const char col_cyan50[]      = "#00b7ff";
static const char col_cyan60[]      = "#33c5ff";
static const char col_cyan70[]      = "#66d4ff";
static const char col_cyan80[]      = "#99e2ff";
static const char col_cyan90[]      = "#ccf1ff";
static const char col_yellow[]      = "#757500";
static const char col_yellow70[]    = "#ffff66";
static const char col_yellow80[]    = "#ffff99";
static const char col_yellow90[]    = "#ffffcc";
static const char col_magenta[]     = "#750075";
static const char col_magenta70[]   = "#ff66ff";
static const char col_magenta80[]   = "#ff99ff";
static const char col_magenta90[]   = "#ffccff";
static const char col_red[]         = "#ff0000";
static const char *colors[][3]      = {
	/*               fg         bg         border   */
	[SchemeNorm] = { col_gray80,    col_gray10,    col_cyan },
	[SchemeSel]  = { col_gray80,    col_cyan,      col_red  },
	[Scheme0]    = { col_cyan80,    col_gray10, "#000000"  },
	[Scheme1]    = { col_cyan80,    col_gray10, "#000000"  },
	[Scheme2]    = { col_yellow80,  col_gray10, "#000000"  },
	[Scheme3]    = { col_magenta80, col_gray10, "#000000"  },
	[Scheme4]    = { col_cyan80,    col_gray10, "#000000"  },
	[Scheme5]    = { col_yellow80,  col_gray10, "#000000"  },
	[Scheme6]    = { col_magenta80, col_gray10, "#000000"  },
	[Scheme7]    = { col_gray80,    col_gray10, "#000000"  },
	[Scheme8]    = { col_cyan80,    col_gray10, "#000000"  },
	[Scheme9]    = { col_red,       col_cyan,   "#000000"  },
	[Scheme10]   = { col_gray10,    col_gray80, "#000000"  },
	[Scheme11]   = { col_gray10,    col_gray80, "#000000"  },
	[Scheme12]   = { col_gray10,    col_gray80, "#000000"  },
};

/* tagging */
static const char *tags[] = { "#", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "#", "#" };

static const Rule rules[] = {
  /* xprop(1):
   *	WM_CLASS(STRING) = instance, class
   *	WM_NAME(STRING) = title
   */
  /* class      instance    title       tags mask     isfloating   monitor */
  { "Gimp",                   NULL,       NULL,       0,              True,        -1 },
  { "tabbed",                 NULL,       NULL,       1 << 5,         False,       -1 },
  { "libreoffice",            NULL,       NULL,       1 << 12,        False,       -1 },
  { NULL, NULL, "fxTrade Practice",                   1 << 4,         False,       -1 },
  { "com-oanda-launcher-Main", NULL,      "/",        1 << 4,         False,       -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const Layout layouts[] = { { NULL }, { tile }, { tcl }, { monocle } };
static const char *kb_layouts[] = { "gb", "us", "ru" };
/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = { "dmenu_run", "-m", dmenumon, "-fn", dmenufont, "-nb", col_gray10, "-nf", col_gray90, "-sb", col_cyan, "-sf", col_gray90, 
NULL };
/*
static const char *termcmd[]  = { "tabbed", "-c", "xterm", "-into", NULL };
static const char *urxvtcmd[] = { "tabbed", "-c", "urxvt", "-embed", NULL };
*/
static const char *termcmd[]  = { "urxvt", NULL };
static const char *termcmd1[] = { "urxvt1", NULL };
static const char *termcmd2[] = { "urxvt2", NULL };
static const char *scrotcmd[] = { "scrot", "/tmp/%Y-%m-%d-%H-%M-%s_$wx$h_scrot.png", "-q", "100", NULL };
static const char *lockscmd[] = { "lockall", NULL };
static const char *brightnessupcmd[] = { "sudo", "/usr/local/bin/backlight", "+", NULL };
static const char *brightnessdncmd[] = { "sudo", "/usr/local/bin/backlight", "-", NULL };
static const char *audioupcmd[] = { "vol", "+", NULL };
static const char *audiodncmd[] = { "vol", "-", NULL };
static const char *audiomutecmd[] = { "vol", "0", NULL };
static const char *displaycmd[] = { "switchdisplay", NULL };
static const char *suspendcmd[] = { "/usr/local/bin/zzz", "M", NULL };
static const char *hibernatecmd[] = { "/usr/local/bin/zzz", "D", NULL };


static Key keys[] = {
  /* modifier                     key        function        argument */
  { MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
  { MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
  { MODKEY,                       XK_b,      togglebar,      {0} },
  { MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
  { MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
  { MODKEY,                       XK_i,      inc_nmaster,    {.i = +1 } },
  { MODKEY,                       XK_d,      inc_nmaster,    {.i = -1 } },
  { MODKEY,                       XK_h,      set_mfact,      {.f = -0.02} },
  { MODKEY,                       XK_l,      set_mfact,      {.f = +0.02} },
  { MODKEY,                       XK_Return, zoomfloat,      {0} },
  { MODKEY,                       XK_z,      zoomcycle,      {0} },
  { MODKEY,                       XK_Escape, view,           {0} },
  { MODKEY,                       XK_Tab,    view_nonempty,  {.i = +1} },
  { MODKEY|ShiftMask,             XK_Tab,    view_nonempty,  {.i = -1} },
  { MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
  { MODKEY,                       XK_f,      setlayout,      {.v = &layouts[0]} },
  { MODKEY,                       XK_t,      setlayout_float,{.v = &layouts[1]} },
  { MODKEY,                       XK_u,      setlayout_float,{.v = &layouts[2]} },
  { MODKEY,                       XK_m,      setlayout_float,{.v = &layouts[3]} },
  { MODKEY,                       XK_space,  setlayout0,     {0} },
  { MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
  { MODKEY,                       XK_BackSpace,      view,   {.ui = ~0 } },
  { MODKEY|ShiftMask,             XK_BackSpace,      tag,    {.ui = ~0 } },
  { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
  { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
  { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
  { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
  TAGKEYS(                        XK_quoteleft,              0)
  TAGKEYS(                        XK_1,                      1)
  TAGKEYS(                        XK_2,                      2)
  TAGKEYS(                        XK_3,                      3)
  TAGKEYS(                        XK_4,                      4)
  TAGKEYS(                        XK_5,                      5)
  TAGKEYS(                        XK_6,                      6)
  TAGKEYS(                        XK_7,                      7)
  TAGKEYS(                        XK_8,                      8)
  TAGKEYS(                        XK_9,                      9)
  TAGKEYS(                        XK_0,                      10)
  TAGKEYS(                        XK_minus,                  11)
  TAGKEYS(                        XK_equal,                  12)
  { MODKEY|ShiftMask,             XK_q,      quit,           {0} },
  { MODKEY|ShiftMask,             XK_k,      setkblayout,    {0} },
  { MODKEY,                       XK_Left,   shiftview,      {.i = -1 } },
  { MODKEY,                       XK_Right,  shiftview,      {.i = +1 } },
  { MODKEY,                       XK_Down,   shiftview,      {.i = -1 } },
  { MODKEY,                       XK_Up,     shiftview,	     {.i = +1 } },
  { MODKEY|ShiftMask,             XK_l,                      spawn, {.v = lockscmd}},
  { MODKEY|ShiftMask,             XK_s,                      spawn, {.v = scrotcmd}},
  { MODKEY|ShiftMask,             XK_r,                      spawn, {.v = termcmd1}},
  { MODKEY|ShiftMask,             XK_t,                      spawn, {.v = termcmd2}},
  { 0,                            XF86XK_AudioRaiseVolume,   spawn, {.v = audioupcmd}},
  { 0,                            XF86XK_AudioLowerVolume,   spawn, {.v = audiodncmd}},
  { 0,                            XF86XK_AudioMute,          spawn, {.v = audiomutecmd}},
  { 0,                            XF86XK_MonBrightnessUp,    spawn, {.v = brightnessupcmd}},
  { 0,                            XF86XK_MonBrightnessDown,  spawn, {.v = brightnessdncmd}},
  { 0,                            XF86XK_Display,            spawn, {.v = displaycmd}},
  { 0,                            XF86XK_Sleep,              spawn, {.v = suspendcmd}},
  { 0,                            XF86XK_Standby,            spawn, {.v = suspendcmd}},
  { 0,                            XF86XK_PowerOff,           spawn, {.v = hibernatecmd}},
  { 0,                            XF86XK_Hibernate,          spawn, {.v = hibernatecmd}},
};

/* button definitions */
/* click can be ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                event mask      button          function        argument */
  { ClkWinTitle,          0,              Button2,        zoom,           {0} },
  { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
  { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
  { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
  { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
  { ClkTagBar,            0,              Button1,        view,           {0} },
  { ClkTagBar,            0,              Button3,        toggleview,     {0} },
  { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
  { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
  { ClkKbLayout,          0,              Button1,        setkblayout,    {0} },
};
