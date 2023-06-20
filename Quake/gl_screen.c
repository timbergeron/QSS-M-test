/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
Copyright (C) 2007-2008 Kristian Duske
Copyright (C) 2010-2014 QuakeSpasm developers

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// screen.c -- master for refresh, status bar, console, chat, notify, etc

#include "time.h"
#include "quakedef.h"

/*

background clear
rendering
turtle/net/ram icons
sbar
centerprint / slow centerprint
notify lines
intermission / finale overlay
loading plaque
console
menu

required background clears
required update regions


syncronous draw mode or async
One off screen buffer, with updates either copied or xblited
Need to double buffer?


async draw will require the refresh area to be cleared, because it will be
xblited, but sync draw can just ignore it.

sync
draw

CenterPrint ()
SlowPrint ()
Screen_Update ();
Con_Printf ();

net
turn off messages option

the refresh is allways rendered, unless the console is full screen


console is:
	notify lines
	half
	full

*/


int			glx, gly, glwidth, glheight;

int ct; // woods connected map time #maptime
extern int	maptime; // woods connected map time #maptime
extern qboolean	sb_showscores; // woods
extern int	fragsort[MAX_SCOREBOARD]; // woods #scrping
extern int	scoreboardlines; // woods #scrping
char mute[2]; // woods for mute to memory #usermute
qboolean pausedprint; // woods #qssmhints
qboolean timerstarted; // woods #qssmhints

float		scr_con_current;
float		scr_conlines;		// lines of console to display

void Sbar_SortFrags(void); // woods #scrping
void Sbar_SortTeamFrags(void); // woods #matchhud
int	Sbar_ColorForMap(int m); // woods #matchhud
void Sbar_DrawCharacter(int x, int y, int num); // woods #matchhud
void Sbar_SortFrags_Obs(void); // woods #observerhud
void Sound_Toggle_Mute_On_f(void); // woods #usermute -- adapted from Fitzquake Mark V

Uint32 HintTimer_Callback(Uint32 interval, void* param); // woods #qssmhints
void Print_Hints_f(void); // woods #qssmhints

//johnfitz -- new cvars
cvar_t		scr_menuscale = {"scr_menuscale", "1", CVAR_ARCHIVE};
cvar_t		scr_sbarscale = {"scr_sbarscale", "1", CVAR_ARCHIVE};
cvar_t		scr_sbaralpha = {"scr_sbaralpha", "0.75", CVAR_ARCHIVE}; // woods #sbarstyles
cvar_t		scr_sbaralphaqwammo = {"scr_sbaralphaqwammo", "1", CVAR_ARCHIVE};
cvar_t		scr_sbar = {"scr_sbar", "1", CVAR_ARCHIVE}; // woods #sbarstyles
cvar_t		scr_conwidth = {"scr_conwidth", "0", CVAR_ARCHIVE};
cvar_t		scr_conscale = {"scr_conscale", "1", CVAR_ARCHIVE};
cvar_t		scr_consize = {"scr_consize", ".5", CVAR_ARCHIVE}; // woods #consize (joequake)
cvar_t		scr_crosshairscale = {"scr_crosshairscale", "1", CVAR_ARCHIVE};
cvar_t		scr_crosshaircolor = {"scr_crosshaircolor", "0", CVAR_ARCHIVE}; // woods #crosshair
cvar_t		scr_showfps = {"scr_showfps", "0", CVAR_ARCHIVE};
cvar_t		scr_clock = {"scr_clock", "0", CVAR_ARCHIVE};
cvar_t		scr_ping = {"scr_ping", "1", CVAR_ARCHIVE};  // woods #scrping
cvar_t		scr_match_hud = {"scr_match_hud", "1", CVAR_ARCHIVE};  // woods #matchhud
cvar_t		scr_showspeed = {"scr_showspeed", "0",CVAR_ARCHIVE}; // woods #speed
cvar_t		scr_matchclock = {"scr_matchclock", "0",CVAR_ARCHIVE}; // woods #varmatchclock
cvar_t		scr_matchclock_y = {"scr_matchclock_y", "0",CVAR_ARCHIVE}; // woods #varmatchclock
cvar_t		scr_matchclock_x = {"scr_matchclock_x", "0",CVAR_ARCHIVE}; // woods #varmatchclock
cvar_t		scr_matchclockscale = {"scr_matchclockscale", "1",CVAR_ARCHIVE}; // woods #varmatchclock
cvar_t		scr_showscores = {"scr_showscores", "0",CVAR_ARCHIVE}; // woods #observerhud
cvar_t		scr_shownet = {"scr_shownet", "0",CVAR_ARCHIVE}; // woods #shownet scr_obscenterprint
cvar_t		scr_obscenterprint = {"scr_obscenterprint", "0",CVAR_ARCHIVE}; // woods
cvar_t		scr_hints = {"scr_hints", "1",CVAR_ARCHIVE}; // woods #qssmhints
//johnfitz
cvar_t		scr_usekfont = {"scr_usekfont", "0", CVAR_NONE}; // 2021 re-release
cvar_t		cl_predict = { "cl_predict", "0", CVAR_NONE }; // 2021 re-release

cvar_t		scr_viewsize = {"viewsize","100", CVAR_ARCHIVE};
cvar_t		scr_fov = {"fov","90",CVAR_ARCHIVE};	// 10 - 170
cvar_t		scr_fov_adapt = {"fov_adapt","1",CVAR_ARCHIVE};
cvar_t		scr_zoomfov = {"zoom_fov","30",CVAR_ARCHIVE};	// 10 - 170 // woods #zoom (ironwail)
cvar_t		scr_zoomspeed = {"zoom_speed","8",CVAR_ARCHIVE}; // woods #zoom (ironwail)
cvar_t		scr_conspeed = {"scr_conspeed","500",CVAR_ARCHIVE};
cvar_t		scr_centertime = {"scr_centertime","2",CVAR_NONE};
cvar_t		scr_showturtle = {"showturtle","0",CVAR_NONE};
cvar_t		scr_showpause = {"showpause","1",CVAR_NONE};
cvar_t		scr_printspeed = {"scr_printspeed","8",CVAR_NONE};
cvar_t		gl_triplebuffer = {"gl_triplebuffer", "1", CVAR_ARCHIVE};

cvar_t		cl_gun_fovscale = {"cl_gun_fovscale","1",CVAR_ARCHIVE}; // Qrack

extern	cvar_t	crosshair;

qboolean	scr_initialized;		// ready to draw

qpic_t		*scr_net;
qpic_t		*scr_turtle;

void Sbar_DrawPicAlpha(int x, int y, qpic_t* pic, float alpha); // woods for loading #flagstatus alpha

int			clearconsole;
int			clearnotify;

vrect_t		scr_vrect;

qboolean	scr_disabled_for_loading;
qboolean	scr_drawloading;
float		scr_disabled_time;

int	scr_tileclear_updates = 0; //johnfitz

void SCR_ScreenShot_f (void);

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;
int			scr_erase_lines;
int			scr_erase_center;
#define CPRINT_TYPEWRITER	(1u<<0)
#define CPRINT_PERSIST		(1u<<1)
#define CPRINT_TALIGN		(1u<<2)
unsigned int scr_centerprint_flags;

int paused = 0; // woods #showpaused
qboolean	countdown; // #clearcrxcountdown
qboolean	cameras; // woods #crxcamera
qboolean	qeintermission; // woods #qeintermission
qboolean draw; // woods #crxcamera #qeintermission

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (const char *str) //update centerprint data
{
	unsigned int flags = 0;

	countdown = false; // woods #clearcrxcountdown
	cameras = false; // woods #crxcamera
	qeintermission = false; // woods #qeintermission
	pausedprint = false; // woods #qssmhints

	if (strstr(str, "eyecam") || strstr(str, "chasecam")) // woods #crxcamera
		cameras = true;

	
	if (cl.modtype == 4) // woods #qeintermission
	{ 
		if (strstr(str, "tttt") || strstr(str, "ttt") || (strstr(str, "ttttt") && strstr(str, "ttttttt"))) // woods #qeintermission (Vote For, TDM Stats, Match Summary)  
			qeintermission = true;
		else
			qeintermission = false;
	}

	if (strstr(str, "ttttttttt")) // woods #clearcrxcountdown (countdown)
		countdown = true;
	else
		countdown = false;

	if ((strstr(str, "tttttt")) || (strstr(str, "PAUSED"))) // #showpaused
	{
		pausedprint = true; // woods #qssmhints
		return;
	}

// ===============================
// woods for center print filter  -> this is #flagstatus
// ===============================

 // begin woods for flagstatus parsing --  t = blue abandoned, t = red abandoned, r = taken, b = taken

	const char* blueflag;
	char buf[10];
	blueflag = Info_GetKey(cl.serverinfo, "blue flag", buf, sizeof(buf));

	if (blueflag[0] == '\0') // we only use this if the server does NOT have a infokey for flag status
	{ 
		strncpy(cl.flagstatus, "n", sizeof(cl.flagstatus)); // null flag, reset all flag ... flags :)

		if (!strpbrk(str, "tttt")) // cdmod MOD print
		{
			// RED

			if (strstr(str, "rt") && !strstr(str, "bt") && !strstr(str, "t")) // red taken
				strncpy(cl.flagstatus, "r", sizeof(cl.flagstatus));

			if (strstr(str, "t") && !strstr(str, "bt") && !strstr(str, "t")) // red abandoned
				strncpy(cl.flagstatus, "x", sizeof(cl.flagstatus));

		// BLUE

			if (strstr(str, "bt") && !strstr(str, "rt") && !strstr(str, "t")) // blue taken
				strncpy(cl.flagstatus, "b", sizeof(cl.flagstatus));

			if (strstr(str, "t") && !strstr(str, "rt") && !strstr(str, "t")) // blue abandoned
				strncpy(cl.flagstatus, "y", sizeof(cl.flagstatus));

		// RED & BLUE

			if ((strstr(str, "bt")) && (strstr(str, "rt"))) //  blue & red taken
				strncpy(cl.flagstatus, "p", sizeof(cl.flagstatus));

			if ((strstr(str, "t")) && (strstr(str, "t"))) // blue & red abandoned
				strncpy(cl.flagstatus, "z", sizeof(cl.flagstatus));

			if ((strstr(str, "t")) && (strstr(str, "rt"))) // blue abandoned, red taken
				strncpy(cl.flagstatus, "j", sizeof(cl.flagstatus));

			if ((strstr(str, "bt")) && (strstr(str, "t"))) // red abandoned, blue taken
				strncpy(cl.flagstatus, "k", sizeof(cl.flagstatus));
		}
	}

	// end woods for flagstatus parsing

	if (!strcmp(str, "You found a secret area!") && cl.gametype == GAME_DEATHMATCH)
		return;

	if (!strcmp(str, "Your team captured the flag!\n") ||
		!strcmp(str, "Your flag was captured!\n") ||
		!strcmp(str, "Enemy tttt has been returned to base!") ||
		!strcmp(str, "Your tttt has been taken!") ||
		!strcmp(str, "Your team has the enemy tttt!") ||
		!strcmp(str, "Your tttt has been returned to base!"))
		return;

	if (*str != '/' && cl.intermission)
		flags |= CPRINT_TYPEWRITER | CPRINT_PERSIST | CPRINT_TALIGN;

	//check for centerprint prefixes/flags
	while (*str == '/')
	{
		if (str[1] == '.')
		{	//no more
			str+=2;
			break;
		}
		else if (str[1] == 'P')
			flags |= CPRINT_PERSIST;
		else if (str[1] == 'W')	//typewriter
			flags ^= CPRINT_TYPEWRITER;
		else if (str[1] == 'S')	//typewriter
			flags ^= CPRINT_PERSIST;
		else if (str[1] == 'M')	//masked background
			;
		else if (str[1] == 'O')	//obituary print (lower half)
			;
		else if (str[1] == 'B')	//bottom-align
			;
		else if (str[1] == 'B')	//top-align
			;
		else if (str[1] == 'L')	//left-align
			;
		else if (str[1] == 'R')	//right-align
			;
		else if (str[1] == 'F')	//alternative 'finale' control
		{
			str+=2;
			if (!cl.intermission)
				cl.completed_time = cl.time;
			switch(*str++)
			{
			case 0:
				str--;
				break;
			case 'R':	//remove intermission (no other method to do this)
				cl.intermission = 0;
				break;
			case 'I':	//regular intermission
			case 'S':	//show scoreboard
				cl.intermission = 1;
				break;
			case 'F':	//like svc_finale
				cl.intermission = 2;
				break;
			default:
				break;	//any other flag you want
			}
			vid.recalc_refdef = true;
			continue;
		}
		else if (str[1] == 'I')	//title image
		{
			const char *e;
			str+=2;
			e = strchr(str, ':');
			if (!e)
				e = strchr(str, ' ');	//probably an error
			if (!e)
				e = str+strlen(str)-1;	//error
			str = e+1;
			continue;
		}
		else
			break;
		str+=2;
	}

	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = (flags&CPRINT_PERSIST)?999999:scr_centertime.value;
	scr_centertime_start = cl.time;

	if (*scr_centerstring && !(flags&CPRINT_PERSIST))
		Con_LogCenterPrint (scr_centerstring);

// count the number of lines for centering
	scr_center_lines = 1;
	str = scr_centerstring;
	while (*str)
	{
		if (*str == '\n')
			scr_center_lines++;
		str++;
	}
}

void SCR_DrawCenterString (void) //actually do the drawing
{
	char	*start;
	int		l;
	int		j;
	int		x, y;
	int		remaining;

	char buf[15];
	const char* realobs;
	realobs = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "observer", buf, sizeof(buf));

	if (!scr_obscenterprint.value && !cameras && !countdown && !qeintermission && ((cl.modtype == 1 || cl.modtype == 4) && (!strcmp(realobs, "eyecam") || (!strcmp(realobs, "chase")))))
		return;

	if (!strcmp(cl.observer, "y") && (cl.modtype >= 2)) // woods #observer
		GL_SetCanvas(CANVAS_OBSERVER); //johnfitz //  center print moved down near weapon
	else
		GL_SetCanvas(CANVAS_MOD); //johnfitz // woods messages scale with console font size instead

// the finale prints the characters one at a time
	if (cl.intermission)
		remaining = scr_printspeed.value * (cl.time - scr_centertime_start);
	else
		remaining = 9999;

	scr_erase_center = 0;
	start = scr_centerstring;

	if (scr_center_lines <= 4)
		y = 200*0.35;	//johnfitz -- 320x200 coordinate system
	else
		y = 48;
	if (crosshair.value)
		y -= 8;

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2;	//johnfitz -- 320x200 coordinate system
		for (j=0 ; j<l ; j++, x+=8)
		{
			Draw_Character (x, y, start[j]);	//johnfitz -- stretch overlays
			if (!remaining--)
				return;
		}

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

void SCR_CheckDrawCenterString (void)
{
	draw = false; // woods #crxcamera #qeintermission
	
	if (scr_center_lines > scr_erase_lines)
		scr_erase_lines = scr_center_lines;

	scr_centertime_off -= host_frametime;

	if (scr_centertime_off <= 0 && !cl.intermission)
		return;
	if (key_dest != key_game)
		return;
	if (cl.paused) //johnfitz -- don't show centerprint during a pause
		return;

	if (cl.paused) //johnfitz -- don't show centerprint during a pause
		return;

	if (sb_showscores == true && (cl.gametype == GAME_DEATHMATCH)) // woods don't overlap centerprints with scoreboard
		return;

	draw = true; // woods #crxcamera #qeintermission

	SCR_DrawCenterString ();
}

// woods #zoom (ironwail) SCR_ToggleZoom_f, SCR_ZoomDown_f, SCR_ZoomUp_f, SCR_UpdateZoom

/*
====================
SCR_ToggleZoom_f
====================
*/
static void SCR_ToggleZoom_f(void)
{
	if (cl.zoomdir)
		cl.zoomdir = -cl.zoomdir;
	else
		cl.zoomdir = cl.zoom > 0.5f ? -1.f : 1.f;
}

/*
====================
SCR_ZoomDown_f
====================
*/
static void SCR_ZoomDown_f(void)
{
	cl.zoomdir = 1.f;
}

/*
====================
SCR_ZoomUp_f
====================
*/
static void SCR_ZoomUp_f(void)
{
	cl.zoomdir = -1.f;
}

/*
====================
SCR_UpdateZoom
====================
*/
void SCR_UpdateZoom(void)
{
	float delta = cl.zoomdir * scr_zoomspeed.value * (cl.time - cl.oldtime);
	if (!delta)
		return;
	cl.zoom += delta;
	if (cl.zoom >= 1.f)
	{
		cl.zoom = 1.f;
		cl.zoomdir = 0.f;
	}
	else if (cl.zoom <= 0.f)
	{
		cl.zoom = 0.f;
		cl.zoomdir = 0.f;
	}
	vid.recalc_refdef = 1;
}

//=============================================================================

/*
====================
AdaptFovx
Adapt a 4:3 horizontal FOV to the current screen size using the "Hor+" scaling:
2.0 * atan(width / height * 3.0 / 4.0 * tan(fov_x / 2.0))
====================
*/
float AdaptFovx (float fov_x, float width, float height)
{
	float	a, x;

	if (cl.statsf[STAT_VIEWZOOM])
		fov_x *= cl.statsf[STAT_VIEWZOOM]/255.0;
	if (fov_x < 1)
		fov_x = 1;
	if (fov_x > 179)
		fov_x = 179;

	if (!scr_fov_adapt.value)
		return fov_x;
	if ((x = height / width) == 0.75)
		return fov_x;
	a = atan(0.75 / x * tan(fov_x / 360 * M_PI));
	a = a * 360 / M_PI;
	return a;
}

/*
====================
CalcFovy
====================
*/
float CalcFovy (float fov_x, float width, float height)
{
	float	a, x;

	if (fov_x < 1 || fov_x > 179)
		Sys_Error ("Bad fov: %f", fov_x);

	x = width / tan(fov_x / 360 * M_PI);
	a = atan(height / x);
	a = a * 360 / M_PI;
	return a;
}

/*
=================
SCR_CalcRefdef

Must be called whenever vid changes
Internal use only
=================
*/
static void SCR_CalcRefdef (void)
{
	float		size, scale; //johnfitz -- scale
	float		zoom; // woods #zoom (ironwail)

// force the status bar to redraw
	Sbar_Changed ();

	scr_tileclear_updates = 0; //johnfitz

// bound viewsize
	if (scr_viewsize.value < 30)
		Cvar_SetQuick (&scr_viewsize, "30");
	if (scr_viewsize.value > 120)
		Cvar_SetQuick (&scr_viewsize, "120");

// bound fov
	if (scr_fov.value < 10)
		Cvar_SetQuick (&scr_fov, "10");
	if (scr_fov.value > 170)
		Cvar_SetQuick (&scr_fov, "170");
	if (scr_zoomfov.value < 10) // woods #zoom (ironwail)
		Cvar_SetQuick(&scr_zoomfov, "10");
	if (scr_zoomfov.value > 170)
		Cvar_SetQuick(&scr_zoomfov, "170"); // woods #zoom (ironwail)

	vid.recalc_refdef = 0;

	//johnfitz -- rewrote this section
	size = scr_viewsize.value;
	scale = CLAMP (1.0f, scr_sbarscale.value, (float)glwidth / 320.0f);

	if (size >= 120 || cl.intermission || (scr_sbaralpha.value < 1 || cl.qcvm.extfuncs.CSQC_DrawHud || cl.qcvm.extfuncs.CSQC_UpdateView)) //johnfitz -- scr_sbaralpha.value. Spike -- simple csqc assumes fullscreen video the same way.
		sb_lines = 0;
	else if (size >= 110)
		sb_lines = 24 * scale;
	else
		sb_lines = 48 * scale;

	size = q_min(scr_viewsize.value, 100.f) / 100;
	//johnfitz

	//johnfitz -- rewrote this section
	r_refdef.vrect.width = q_max(glwidth * size, 96.0f); //no smaller than 96, for icons
	r_refdef.vrect.height = q_min((int)(glheight * size), glheight - sb_lines); //make room for sbar
	r_refdef.vrect.x = (glwidth - r_refdef.vrect.width)/2;
	r_refdef.vrect.y = (glheight - sb_lines - r_refdef.vrect.height)/2;
	//johnfitz

	zoom = cl.zoom;
	zoom *= zoom * (3.f - 2.f * zoom); // smoothstep // woods #zoom (ironwail)
	r_refdef.basefov = LERP(scr_fov.value, scr_zoomfov.value, zoom); // woods #zoom (ironwail)
	r_refdef.fov_x = AdaptFovx(r_refdef.basefov, vid.width, vid.height); // woods #zoom (ironwail)
	r_refdef.fov_y = CalcFovy (r_refdef.fov_x, r_refdef.vrect.width, r_refdef.vrect.height); // woods #zoom (ironwail)

	scr_vrect = r_refdef.vrect;
}


/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
void SCR_SizeUp_f (void)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value+10);
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
void SCR_SizeDown_f (void)
{
	Cvar_SetValueQuick (&scr_viewsize, scr_viewsize.value-10);
}

static void SCR_Callback_refdef (cvar_t *var)
{
	vid.recalc_refdef = 1;
	if (key_dest != key_console && host_initialized && scr_viewsize.value != 130 && scr_viewsize.value != 20) // woods
		Con_Printf("screen size: ^m%i\n", (int)scr_viewsize.value);
}

/*
==================
SCR_Conwidth_f -- johnfitz -- called when scr_conwidth or scr_conscale changes
==================
*/
void SCR_Conwidth_f (cvar_t *var)
{
	vid.recalc_refdef = 1;
	vid.conwidth = (scr_conwidth.value > 0) ? (int)scr_conwidth.value : (scr_conscale.value > 0) ? (int)(vid.width/scr_conscale.value) : vid.width;
	vid.conwidth = CLAMP (320, vid.conwidth, vid.width);
	vid.conwidth &= 0xFFFFFFF8;
	vid.conheight = vid.conwidth * vid.height / vid.width;
}

//============================================================================

/*
==================
SCR_LoadPics -- johnfitz
==================
*/
void SCR_LoadPics (void)
{
	scr_net = Draw_PicFromWad ("net");
	scr_turtle = Draw_PicFromWad ("turtle");
}

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	//johnfitz -- new cvars
	Cvar_RegisterVariable (&scr_menuscale);
	Cvar_RegisterVariable (&scr_sbarscale);
	Cvar_SetCallback (&scr_sbaralpha, SCR_Callback_refdef);
	Cvar_RegisterVariable (&scr_sbaralpha);
	Cvar_RegisterVariable (&scr_sbaralphaqwammo); // woods #sbarstyles
	Cvar_RegisterVariable (&scr_sbar); // woods #sbarstyles
	Cvar_SetCallback (&scr_conwidth, &SCR_Conwidth_f);
	Cvar_SetCallback (&scr_conscale, &SCR_Conwidth_f);
	Cvar_RegisterVariable (&scr_conwidth);
	Cvar_RegisterVariable (&scr_conscale);
	Cvar_RegisterVariable (&scr_consize); // woods #consize (joequake)
	Cvar_RegisterVariable (&scr_crosshairscale);
	Cvar_RegisterVariable (&scr_crosshaircolor); // woods #crosshair
	Cvar_RegisterVariable (&scr_showfps);
	Cvar_RegisterVariable (&scr_clock);
	Cvar_RegisterVariable (&scr_ping); // woods #scrping
	Cvar_RegisterVariable(&scr_match_hud); // woods #matchhud
	Cvar_RegisterVariable (&scr_showspeed); // woods #speed
	Cvar_RegisterVariable (&scr_matchclock); // woods #varmatchclock
	Cvar_RegisterVariable (&scr_matchclock_y); // woods #varmatchclock
	Cvar_RegisterVariable (&scr_matchclock_x); // woods #varmatchclock
	Cvar_RegisterVariable (&scr_matchclockscale); // woods #varmatchclock
	Cvar_RegisterVariable (&scr_showscores); // woods #observerhud
	Cvar_RegisterVariable (&scr_shownet); // woods #shownet
	Cvar_RegisterVariable (&scr_obscenterprint); // woods
	Cvar_RegisterVariable (&scr_hints); // woods #qssmhints
	//johnfitz
	Cvar_RegisterVariable (&scr_usekfont); // 2021 re-release
	Cvar_RegisterVariable (&cl_predict); // 2021 re-release
	Cvar_SetCallback (&scr_fov, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_fov_adapt, SCR_Callback_refdef); // woods #zoom (ironwail)
	Cvar_SetCallback (&scr_zoomfov, SCR_Callback_refdef);
	Cvar_SetCallback (&scr_viewsize, SCR_Callback_refdef);
	Cvar_RegisterVariable (&scr_fov);
	Cvar_RegisterVariable (&scr_fov_adapt); // woods #zoom (ironwail)
	Cvar_RegisterVariable(&scr_zoomfov); // woods #zoom (ironwail)
	Cvar_RegisterVariable(&scr_zoomspeed);
	Cvar_RegisterVariable (&scr_viewsize);
	Cvar_RegisterVariable (&scr_conspeed);
	Cvar_RegisterVariable (&scr_showturtle);
	Cvar_RegisterVariable (&scr_showpause);
	Cvar_RegisterVariable (&scr_centertime);
	Cvar_RegisterVariable (&scr_printspeed);
	Cvar_RegisterVariable (&gl_triplebuffer);
	Cvar_RegisterVariable (&cl_gun_fovscale);

	Cmd_AddCommand ("screenshot",SCR_ScreenShot_f);
	Cmd_AddCommand ("sizeup",SCR_SizeUp_f);
	Cmd_AddCommand ("sizedown",SCR_SizeDown_f);

	Cmd_AddCommand("togglezoom", SCR_ToggleZoom_f); // woods #zoom (ironwail)
	Cmd_AddCommand("+zoom", SCR_ZoomDown_f); // woods #zoom (ironwail)
	Cmd_AddCommand("-zoom", SCR_ZoomUp_f); // woods #zoom (ironwail)
	Cmd_AddCommand("hints", Print_Hints_f); // woods #hints

	SCR_LoadPics (); //johnfitz

	scr_initialized = true;
}

//============================================================================

/*
==============
SCR_DrawFPS -- johnfitz
==============
*/
void SCR_DrawFPS (void)
{
	static double	oldtime = 0;
	static double	lastfps = 0;
	static int	oldframecount = 0;
	double	elapsed_time;
	int	frames;

	elapsed_time = realtime - oldtime;
	frames = r_framecount - oldframecount;

	if (elapsed_time < 0 || frames < 0)
	{
		oldtime = realtime;
		oldframecount = r_framecount;
		return;
	}
	// update value every 3/4 second
	if (elapsed_time > 0.75)
	{
		lastfps = frames / elapsed_time;
		oldtime = realtime;
		oldframecount = r_framecount;
	}

	if (scr_showfps.value)
	{
		char	st[16];
		char	st2[16]; // woods #f_config
		int	x, y;
		sprintf (st, "%4.0f fps", lastfps);
		sprintf (st2, "%4.0f", lastfps); // woods #f_config
		cl.fps = atoi(st2); // woods #f_config
		x = 312 - (strlen(st)<<3); // woods added padding
		if (scr_sbar.value == 3 && scr_viewsize.value <= 110) // woods #qehud
		{
			GL_SetCanvas(CANVAS_BOTTOMRIGHTQESMALL);
			x = 301;
			if ((cl.items & IT_KEY1) || (cl.items & IT_KEY2) || (cl.items & IT_SIGIL1) || (cl.items & IT_SIGIL2) || (cl.items & IT_SIGIL3) || (cl.items & IT_SIGIL4))
			{
				y = 120;
				if (scr_viewsize.value >= 110)
					y += 20;
			}
			else
				y = 142;
		}
		else
		{
			GL_SetCanvas(CANVAS_BOTTOMRIGHT);
			x = 312;
			y = 186;
		}
		if (scr_clock.value) y -= 12; //make room for clock // woods added padding
			Draw_String (x - (strlen(st) << 3), y, st);

		scr_tileclear_updates = 0;
	}
}

/*
==============
SCR_DrawClock -- johnfitz
==============
*/
void SCR_DrawClock (void)
{
	char	str[12];
	int x,y;

	if (scr_clock.value == 1)
	{
		int minutes, seconds;

		minutes = cl.time / 60;
		seconds = ((int)cl.time)%60;

		sprintf (str,"%i:%i%i", minutes, seconds/10, seconds%10);
	}

	else if (scr_clock.value == 2)
	{
		time_t systime = time(0);
		struct tm loct =*localtime(&systime);

		strftime(str, 12, "%I:%M %p", &loct);
	}

	else if (scr_clock.value == 3)
	{
		time_t systime = time(0);
		struct tm loct =*localtime(&systime);

		strftime(str, 12, "%X", &loct);
	}
	else if (scr_clock.value == 4)
	{
		time_t systime = time(0);
		struct tm loct = *localtime(&systime);

		strftime(str, 12, "%m/%d/%Y", &loct);
	}
	else
		return;

	//draw it

	if (scr_sbar.value == 3 && scr_viewsize.value <= 110) // woods #qehud
	{
		GL_SetCanvas(CANVAS_BOTTOMRIGHTQESMALL);
		x = 301;
		if ((cl.items & IT_KEY1) || (cl.items & IT_KEY2) || (cl.items & IT_SIGIL1) || (cl.items & IT_SIGIL2) || (cl.items & IT_SIGIL3) || (cl.items & IT_SIGIL4))
		{
			y = 120;
			if (scr_viewsize.value >= 110)
				y += 20;
		}
		else
			y = 142;
	}
	else
	{ 
		GL_SetCanvas(CANVAS_BOTTOMRIGHT);
		x = 312;
		y = 186;
	}

	Draw_String(x - (strlen(str) << 3), y, str); // woods added padding
	scr_tileclear_updates = 0;
}

/*
==================
SCR_Show_Ping -- added by woods #scrping
==================
*/
void SCR_ShowPing(void)
{
	int	i, k, l;
	int	x, y;
	char	num[12];
	scoreboard_t* s;

	ct = (SDL_GetTicks() - maptime) / 1000; // woods connected map time #maptime

	if (cl.gametype == GAME_DEATHMATCH && cls.state == ca_connected) {

		if (scr_ping.value) {

			GL_SetCanvas (CANVAS_BOTTOMLEFT2); //johnfitz woods 9/2/2021

			Sbar_SortFrags ();

			// draw the text
			l = scoreboardlines;

			x = 46; //johnfitz -- simplified becuase some positioning is handled elsewhere
			y = 20;

			if (scr_sbar.value == 3 && scr_viewsize.value <= 110) // #qehud
			{
				GL_SetCanvas(CANVAS_BOTTOMLEFTQESMALL);
				if (cl.stats[STAT_ARMOR] < 1)
					y = 140;
				else
					y = 114;
				x = 61;				
			}
			else
			{
				x = 46;
				y = 86;
			}

			for (i = 0; i < l; i++)
			{
				k = fragsort[i];
				s = &cl.scores[k];
				if (!s->name[0])
					continue;

				if (fragsort[i] == cl.realviewentity - 1) {

					sprintf (num, "%-4i", s->ping);

					if (ct > 5 && !scr_con_current) // dont update when console down or report ping 0
						M_PrintWhite (x - 8 * 5, y, num); //johnfitz -- was Draw_String, changed for stretched overlays 
				}
			}

			if (key_dest != key_console && (cls.signon >= SIGNONS)) // dont update when console down or not fully connected

				if (!cls.message.cursize && cl.expectingpingtimes < realtime)
				{
					cl.expectingpingtimes = realtime + 5;   // update frequency
					MSG_WriteByte(&cls.message, clc_stringcmd);
					MSG_WriteString(&cls.message, "ping");
				}
		}
	}

}

/*
==================
SCR_ShowPL -- added by woods #scrpl
==================
*/
void SCR_ShowPL(void)
{
	int pl;
	char			num[12];

	ct = (SDL_GetTicks() - maptime) / 1000; // woods connected map time #maptime

	if (cl.gametype == GAME_DEATHMATCH && cls.state == ca_connected) {

		pl = atoi(cl.scrpacketloss); // convert string to integer

		int	x, y;

		if (scr_sbar.value == 3) // #qehud
		{
			GL_SetCanvas(CANVAS_BOTTOMLEFTQESMALL);
			x = 20;
			if (cl.stats[STAT_ARMOR] < 1)
				y = 129;
			else
				y = 103;
			if (!scr_ping.value)
				y += 10;
		}
		else
		{
			GL_SetCanvas(CANVAS_BOTTOMLEFT2);
			x = 6;
			y = 77;
			if (!scr_ping.value)
				y += 10;
		}

		if (cl.expectingpltimes < realtime)
		{
			cl.expectingpltimes = realtime + 5;   // update frequency
			Cmd_ExecuteString("pl\n", src_command);

		}

		if (key_dest != key_console && ((ct != (int)cl.time) && (ct > 6)))
		{
			if (pl > 0) // color red
			{
				sprintf(num, "%-4i", pl);
				M_Print(x, y, num);
			}
		}
	}
}

/*====================
SCR_DrawMatchClock    woods (Adapted from Sbar_DrawFrags from r00k) draw match clock upper right corner #matchhud
====================
*/
void SCR_DrawMatchClock(void)
{
	char			num[22] = "empty";
	int				teamscores, minutes, seconds;
	int				match_time, tl;

	match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.time - cl.last_match_time));
	minutes = match_time / 60;
	seconds = match_time - 60 * minutes;
	teamscores = cl.teamgame;

	GL_SetCanvas(CANVAS_TOPRIGHT2);

	if ((teamscores) && !(cl.minutes != 255)) // display 0.00 for pre match mode in DM
	{
		sprintf(num, "%3d:%02d", 0, 0);
		Draw_String(((314 - (strlen(num) << 3)) + 1), 195 - 8, num);
	}

	if ((cl.minutes != 255))
	{
		if (!strcmp(cl.ffa, "y")) // display count up to timelimit in normal/ffa mode
		{
			
			minutes = cl.time / 60;
			seconds = cl.time - 60 * minutes;
			minutes = minutes & 511;
			sprintf(num, "%3d:%02d", minutes, seconds);
		}

		if (teamscores) // display timelimit if we can get it if there is a team
		{
			if (cl.modtype == 1) // nq crx server check, if so parse serverinfo for timelimit
			{
				char buf[10];
				const char* simt;
				simt = Info_GetKey(cl.serverinfo, "matchtime", buf, sizeof(buf));
				tl = atoi(simt);
			}
			else if (cl.modtype == 4) // qecrx server check, if so parse userinfo for timelimit
			{
				char buf[10];
				const char* uimt;
				uimt = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "matchtime", buf, sizeof(buf)); // userinfo (qecrx)
				tl = atoi(uimt);
			}
			else
				tl = 0; // if no timelimit available, set clock to 0:00

			sprintf(num, "%3d:%02d", tl, 0);
		}

		if (cl.minutes || cl.seconds) // counter is rolling
		{
			if (cl.match_pause_time)
				match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.match_pause_time - cl.last_match_time));
			else
				match_time = ceil(60.0 * cl.minutes + cl.seconds - (cl.time - cl.last_match_time));
			minutes = match_time / 60;
			seconds = match_time - 60 * minutes;
			sprintf(num, "%3d:%02d", minutes, seconds);
		}

		if (cl.seconds >= 128) // DM CRMOD 6.6 countdown, second count inaccurate in countdown, fix it
			sprintf(num, " 0:%02d", cl.seconds - 128);

		// now lets draw the clocks

		if (!strcmp(num, "empty"))
			return;

		if (scr_match_hud.value)
		{
			if ((((minutes <= 0) && (seconds < 15) && (seconds > 0)) && !(!strcmp(cl.ffa, "y"))) || cl.seconds >= 128) // color last 15 seconds to draw attention cl.seconds >= 128 is for CRMOD
				M_Print(((314 - (strlen(num) << 3)) + 1), 195 - 8, num); // M_Print is colored text
			else
				Draw_String(((314 - (strlen(num) << 3)) + 1), 195 - 8, num);
		}

		if (key_dest == key_menu) // woods #menuclear
			return;

		if (countdown && draw) // woods #clearcrxcountdown
			return;

		if (qeintermission && draw) // woods #qeintermission
			return;

		if (scr_matchclock.value) // woods #varmatchclock draw variable clock where players wants based on their x, y cvar
		{
			if (sb_showscores == false && (cl.gametype == GAME_DEATHMATCH && cls.state == ca_connected)) // woods don't overlap crosshair with scoreboard
			{
				GL_SetCanvas(CANVAS_MATCHCLOCK);
				if ((((minutes <= 0) && (seconds < 15) && (seconds > 0)) && !(!strcmp(cl.ffa, "y"))) || cl.seconds >= 128) // color last 15 seconds to draw attention cl.seconds >= 128 is for CRMOD
					M_Print(scr_matchclock_x.value, scr_matchclock_y.value, num); // M_Print is colored text
				else
					Draw_String(scr_matchclock_x.value, scr_matchclock_y.value, num);
			}
		}
	}
}

/*====================
SCR_DrawMatchScores   -- woods  (Adapted from Sbar_DrawFrags from r00k) -- draw match scores upper right corner #matchhud
====================++
*/
void SCR_DrawMatchScores(void)
{
	int				i, k, l;
	int				top, bottom;
	int				x, y, f;
	char			num[12];
	int				teamscores, colors;// JPG - added these
	int				ts1, ts2, tc1, tc2, diff, l2; // woods #hud_diff
	char			tcolor[12]; // woods #hud_diff
	scoreboard_t* s; // woods #hud_diff

	// JPG - check to see if we should sort teamscores instead
	teamscores = /*pq_teamscores.value && */cl.teamgame;

	if (teamscores)    // display frags if it's a teamgame match
		Sbar_SortTeamFrags();
	else
		return;

	// draw the text
	l = scoreboardlines <= 4 ? scoreboardlines : 4;

	x = 0;
	y = 0; // woods to position vertical

	if (cl.gametype == GAME_DEATHMATCH)
	{
		GL_SetCanvas(CANVAS_TOPRIGHT3);

		char buf[10];

		const char* uiplaymode;
		uiplaymode = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "mode", buf, sizeof(buf)); // userinfo (qecrx)

		if (!q_strcasecmp(uiplaymode, "ffa"))
			return;
		
		if (scr_match_hud.value)   // woods for console var off and on
		{
			if (cl.minutes != 255)
				Draw_Fill(11, 1, 32, 18, 0, 0.3);  // rectangle for missing team

			for (i = 0; i < l; i++)
			{
				k = fragsort[i];

				// JPG - check for teamscores
				if (teamscores)
				{
					colors = cl.teamscores[k].colors;
					f = cl.teamscores[k].frags;
					cl.teamscore[i] = f;
					cl.teamcolor[i] = colors;
				}
				else
					return;

				// draw background
				if (teamscores)
				{
					top = (colors & 15) << 4;
					bottom = (colors & 15) << 4;
				}
				else
				{
					top = colors & 0xf0;
					bottom = (colors & 15) << 4;
				}
				top = Sbar_ColorForMap(top);
				bottom = Sbar_ColorForMap(bottom);

				GL_SetCanvas(CANVAS_TOPRIGHT3);

				Draw_Fill((((x + 1) * 8) + 3), y + 1, 32, 6, top, .6);
				Draw_Fill((((x + 1) * 8) + 3), y + 7, 32, 3.5, bottom, .6);

				// draw number
				sprintf(num, "%3i", f);

				Sbar_DrawCharacter(((x + 1) * 8) + 7, y - 23, num[0]);
				Sbar_DrawCharacter(((x + 2) * 8) + 7, y - 23, num[1]);
				Sbar_DrawCharacter(((x + 3) * 8) + 7, y - 23, num[2]);

				x += 0;
				y += 9;  // woods to position vertical
			}

			// woods #hud_diff display point differential

			if (!cl.teamcolor[2]) // only for two colors
			{
				for (i = 0; i < l; i++)
				{
					k = fragsort[i];

					ts1 = cl.teamscore[0]; // high score
					ts2 = cl.teamscore[1]; // low score
					diff = abs(ts1 - ts2); // +/= differential

					tc1 = cl.teamcolor[0]; // top score [color]
					tc2 = cl.teamcolor[1]; // bottom score [color]

					// lets get YOUR team color from scoreboard

					Sbar_SortFrags();
					l2 = scoreboardlines;

					for (i = 0; i < l2; i++)
					{
						k = fragsort[i];
						s = &cl.scores[k];
						if (!s->name[0])
							continue;

						if (fragsort[i] == cl.viewentity - 1) {
							sprintf(tcolor, "%u", s->pants.basic);
						}
					}

					GL_SetCanvas(CANVAS_TOPRIGHT4); // lets do some printing

					if ((ts1 == ts2) || (l < 2)) // don't show ties, l = # of teams
						continue;

					else if ((atoi(tcolor) == tc1) || atoi(tcolor) == (tc1/17))// top score [color] is the same as your color
					{
						sprintf(num, "+%-i", diff);
						M_Print(40 - (strlen(num) << 3), y, num);
					}

					else if ((atoi(tcolor) == tc2) || atoi(tcolor) == (tc2 / 17)) // bottom score [color] is the same as your color
					{
						sprintf(num, "-%-i", diff);
						M_Print(40 - (strlen(num) << 3), y + 20, num);
					}				
				}
			}
		}
	}
	else
		return;
}

/*
=======================
SCR_ShowObsFrags -- added by woods #observerhud
=======================
*/

void SCR_ShowObsFrags(void)
{
	int	i, k, x, y, f;
	char	num[12];
	scoreboard_t* s;
	char	shortname[16]; // woods for dynamic scoreboard during match, don't show ready
	char buf[15];
	const char* obs;

	if (cl.intermission)
		return;

	if (qeintermission)
		return;

	if (scr_viewsize.value == 120)
		return;

	if ((cl.gametype == GAME_DEATHMATCH) && (cls.state == ca_connected))
	{
		obs = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "observer", buf, sizeof(buf));

		if ((!strcmp(cl.observer, "y") && (cl.modtype >= 2)) || scr_showscores.value || !strcmp(obs, "eyecam") || !strcmp(obs, "chase") || !strcmp(obs, "fly") || !strcmp(obs, "walk"))
		{
			Sbar_SortFrags_Obs ();

			if (scr_sbar.value == 3)
			{
				GL_SetCanvas(CANVAS_BOTTOMLEFTQESCORES);
				x = 24;
				y = 170;
			}
			else
			{ 
				GL_SetCanvas(CANVAS_SCORES);
				x = 10;
				y = 160;
			}
			for (i = 0; i < scoreboardlines; i++, y += -8) //johnfitz -- change y init, test, inc woods (reverse drawing order from bottom to top)
			{
				k = fragsort[i];
				s = &cl.scores[k];
				if (!s->name[0])
					continue;

				// colors
				Draw_FillPlayer(x, y + 1, 40, 4, s->shirt, 1);
				Draw_FillPlayer(x, y + 5, 40, 3, s->pants, 1);

				// number
				f = s->frags;
				sprintf(num, "%3i", f);
				Draw_Character(x + 8, y, num[0]);
				Draw_Character(x + 16, y, num[1]);
				Draw_Character(x + 24, y, num[2]);

				// name
				sprintf(shortname, "%.15s", s->name); // woods only show name, not 'ready' or 'afk' -- 15 characters
				M_PrintWhite(x + 50, y, shortname);
			}
		}
	}
}

/*
=======================
SCR_ShowFlagStatus -- added by woods #flagstatus
Grab the impulse 70-80 CRCTF flag and print to top right screen. Abadondoned flags have reduced transparency.
=======================
*/
void SCR_ShowFlagStatus(void)
{
	float z;
	int x, y, xx, yy;

	// woods lets get some info from server infokeys

	const char* redflag;
	char buf[10];
	redflag = Info_GetKey(cl.serverinfo, "red flag", buf, sizeof(buf));

	const char* blueflag;
	char buf2[10];
	blueflag = Info_GetKey(cl.serverinfo, "blue flag", buf2, sizeof(buf2));

	if (blueflag[0] != '\0') // is there a key on the server (newer version of crx)
	{
		strncpy(cl.flagstatus, "n", sizeof(cl.flagstatus)); // null flag, reset all flag ... flags :)

		// RED

		if (!strcmp(redflag, "carried"))
			strncpy(cl.flagstatus, "r", sizeof(cl.flagstatus)); // red taken

		if (!strcmp(redflag, "dropped"))
			strncpy(cl.flagstatus, "x", sizeof(cl.flagstatus)); // red abandoned

		// BLUE

		if (!strcmp(blueflag, "carried"))
			strncpy(cl.flagstatus, "b", sizeof(cl.flagstatus)); // blue abandoned

		if (!strcmp(blueflag, "dropped"))
			strncpy(cl.flagstatus, "y", sizeof(cl.flagstatus)); // blue abandoned

		// RED & BLUE

		if (!strcmp(blueflag, "carried") && !strcmp(redflag, "carried")) // blue & red taken
			strncpy(cl.flagstatus, "p", sizeof(cl.flagstatus));

		if (!strcmp(blueflag, "dropped") && !strcmp(redflag, "dropped")) // blue & red abandoned
			strncpy(cl.flagstatus, "z", sizeof(cl.flagstatus));

		if (!strcmp(blueflag, "dropped") && !strcmp(redflag, "carried")) // blue abandoned, red taken
			strncpy(cl.flagstatus, "j", sizeof(cl.flagstatus));

		if (!strcmp(blueflag, "carried") && !strcmp(redflag, "dropped")) // red abandoned, blue taken
			strncpy(cl.flagstatus, "k", sizeof(cl.flagstatus));
	}

	GL_SetCanvas(CANVAS_TOPRIGHT3);

	z = 0.20; // abandoned not at base flag (alpha)
	x = 0; xx = 0; 	y = 0; 	yy = 0; // initiate

	if (!strcmp(cl.ffa, "y")) // change position in ffa mode below the clock
	{  // xx and yy needed because drawalpha uses diff positioning
		x = 26;
		xx = 12;
		y = -1;
		yy = -25;

	}

	else // xx and yy needed because drawalpha uses diff positioning
	{
		x = 26;
		xx = 12;
		y = 19;
		yy = -5;
	}

	if (scr_match_hud.value == 1)

		if (cl.gametype == GAME_DEATHMATCH && cls.state == ca_connected)
		{
			if (!strcmp(cl.flagstatus, "r")) // red taken
				Draw_Pic (x, y, Draw_PicFromWad ("sb_key2"));

			if (!strcmp(cl.flagstatus, "x")) // red abandoned
			{
				glDisable (GL_ALPHA_TEST);
				glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				Sbar_DrawPicAlpha (x, yy, Draw_PicFromWad2 ("sb_key2", TEXPREF_PAD | TEXPREF_NOPICMIP), z); // doesnt work
			}

			if (!strcmp(cl.flagstatus, "b")) // blue taken
				Draw_Pic (x, y, Draw_PicFromWad ("sb_key1"));

			if (!strcmp(cl.flagstatus, "y")) // blue abandoned
			{
				glDisable (GL_ALPHA_TEST);
				glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				Sbar_DrawPicAlpha (x, yy, Draw_PicFromWad2 ("sb_key1", TEXPREF_PAD | TEXPREF_NOPICMIP), z);
			}

			if (!strcmp(cl.flagstatus, "p")) //  blue & red taken
			{
				Draw_Pic (x, y, Draw_PicFromWad ("sb_key1")); // blue
				Draw_Pic (xx, y, Draw_PicFromWad ("sb_key2")); // red
			}

			if (!strcmp(cl.flagstatus, "z")) // blue & red abandoned
			{
				glDisable (GL_ALPHA_TEST);
				glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				Sbar_DrawPicAlpha (xx, yy, Draw_PicFromWad2 ("sb_key2", TEXPREF_PAD | TEXPREF_NOPICMIP), z);
				Sbar_DrawPicAlpha (x, yy, Draw_PicFromWad2 ("sb_key1", TEXPREF_PAD | TEXPREF_NOPICMIP), z);
			}

			if (!strcmp (cl.flagstatus, "j"))  // blue abandoned, red taken
			{
				Draw_Pic (xx, y, Draw_PicFromWad ("sb_key2")); // red

				glDisable (GL_ALPHA_TEST);
				glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				Sbar_DrawPicAlpha (x, yy, Draw_PicFromWad2 ("sb_key1", TEXPREF_PAD | TEXPREF_NOPICMIP), z);
			}

			if (!strcmp(cl.flagstatus, "k")) // red abandoned, blue taken
			{
				glDisable (GL_ALPHA_TEST);
				glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

				Sbar_DrawPicAlpha (xx, yy, Draw_PicFromWad2("sb_key2", TEXPREF_PAD | TEXPREF_NOPICMIP), z); // red

				Draw_Pic (x, y, Draw_PicFromWad ("sb_key1")); // blue
			}
		}
}

/*
==============
SCR_DrawSpeed -- woods #speed
==============
*/
void SCR_DrawSpeed (void)
{
	char			st[64];
	int				x, y;

	if (scr_viewsize.value > 110)
		return;

	if (scr_sbar.value == 3)
	{
		GL_SetCanvas(CANVAS_BOTTOMLEFTQE);
		y = 175;
		x = 134;
	}
	else
	{
		GL_SetCanvas(CANVAS_SBAR2);
		x = 0;
		y = 0;

		if (scr_viewsize.value <= 100)
			y = 18;
		else if (scr_viewsize.value == 110)
			y = 43;
		if (scr_sbar.value == 2)
			y = 43;
	}

	if (scr_showspeed.value && !cl.intermission) {
		vec3_t	vel = { cl.velocity[0], cl.velocity[1], 0 };
		float	speed = VectorLength(vel);

	sprintf(st, "%-3.0f", speed);
	if (scr_viewsize.value <= 110)
		{
			if (speed > 400 && !(speed > 600)) // red
				M_Print(x, y, st);
			else if (speed > 600)
					M_Print2(x, y, st); // yellow/gold
			else
					M_PrintWhite(x, y, st);  // white
		}
	}
}

/*
==============
SCR_DrawMute -- woods #usermute
==============
*/
void SCR_Mute(void)
{
	int				x, y;

	if (cl.intermission)
		return;
	if (scr_sbar.value > 3)
		return;

	if (scr_viewsize.value > 110)
		return;

	if (!strcmp(mute, "y"))
	{

		if (scr_sbar.value == 3) // #qehud
		{
			y = 173;
			x = 184;
			GL_SetCanvas(CANVAS_BOTTOMRIGHTQESMALL);
			

			if (cl.stats[STAT_AMMO] > 9) // two digits
				x -= 20;
			if (cl.stats[STAT_AMMO] > 99) // three digits
				x -= 32;

			if (cls.demoplayback)
				x -= 34;

			M_PrintWhite(x, y, "mute");
		}
		else
		{ 
			GL_SetCanvas(CANVAS_SBAR2);

			x = 288;
			y = 0;

			if (scr_viewsize.value <= 100)
				y = 18;
			else if (scr_viewsize.value == 110)
				y = 43;
			else
				return;
			if (scr_sbar.value == 2)
			{ 
				y = 43;

				if (!scr_showspeed.value || !cls.demoplayback)
					x = 0;
				if (scr_showspeed.value)
					x = 40;
			}

			M_PrintWhite(x, y, "mute");
		}
	}
}

/*
==============
SCR_Mute_Switch -- woods
==============
*/
void SCR_Mute_Switch(void)
{
	if ((!strcmp(mute, "y")) != true)
		strncpy(mute, "y", sizeof(mute));
	else
		strncpy(mute, "n", sizeof(mute));
}

/*
==============
SCR_Observing -- woods -- detect if client is observing and print for crx. eyecam pulls keys from other persons viewentity, chase doesnt
do not use for: fly or walk
==============
*/
void SCR_Observing(void)
{
	if ((cl.gametype == GAME_DEATHMATCH) && (cls.state == ca_connected))
	{
		char printtxt[25];
		char buf2[25];
		char buf3[25];
		const char* obs;
		const char* observing;
		int color;
		obs = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "observer", buf2, sizeof(buf2));
		observing = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "observing", buf3, sizeof(buf3));
		color = cl.scores[cl.viewentity - 1].pants.basic; // get color 0-13
		color = Sbar_ColorForMap((color & 15) << 4); // translate to proper drawfill color

		if (scr_viewsize.value > 110)
			return;
		
		if (cl.intermission)
			return;

		if (qeintermission)
			return;

		if (!strcmp(observing, "off"))
			return;

		GL_SetCanvas(CANVAS_SBAR2);

		if (cl.modtype == 1 || cl.modtype == 4) // crx case
		{
			if (!strcmp(obs, "chase")) // chase
			{
				sprintf(printtxt, "%s", observing); // print who you are observering
				M_PrintWhite(136 - strlen(observing), 0, printtxt);
			}
			else if (!strcmp(obs, "eyecam"))// eyecam
			{
				sprintf(printtxt, "%s", observing); // // print self (name), viewentity hack (eyecam thinks your are them)
				M_PrintWhite(136-strlen(observing), 0, printtxt);
				Draw_Fill(125 - strlen(observing), 0, 8, 8, color, 1); // show their color
			}		
		}
	}
}

/*
==============
SCR_DrawDevStats
==============
*/
void SCR_DrawDevStats (void)
{
	char	str[40];
	int		y = 25-9; //9=number of lines to print
	int		x = 0; //margin

	if (!devstats.value)
		return;

	GL_SetCanvas (CANVAS_BOTTOMLEFT);

	Draw_Fill (x, y*8, 19*8, 9*8, 0, 0.5); //dark rectangle

	sprintf (str, "devstats |Curr Peak");
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "---------+---------");
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Edicts   |%4i %4i", dev_stats.edicts, dev_peakstats.edicts);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Packet   |%4i %4i", dev_stats.packetsize, dev_peakstats.packetsize);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Visedicts|%4i %4i", dev_stats.visedicts, dev_peakstats.visedicts);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Efrags   |%4i %4i", dev_stats.efrags, dev_peakstats.efrags);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Dlights  |%4i %4i", dev_stats.dlights, dev_peakstats.dlights);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Beams    |%4i %4i", dev_stats.beams, dev_peakstats.beams);
	Draw_String (x, (y++)*8-x, str);

	sprintf (str, "Tempents |%4i %4i", dev_stats.tempents, dev_peakstats.tempents);
	Draw_String (x, (y++)*8-x, str);
}

/*
==============
SCR_DrawTurtle
==============
*/
void SCR_DrawTurtle (void)
{
	static int	count;

	if (!scr_showturtle.value)
		return;

	if (host_frametime < 0.1)
	{
		count = 0;
		return;
	}

	count++;
	if (count < 3)
		return;

	GL_SetCanvas (CANVAS_DEFAULT); //johnfitz

	Draw_Pic (scr_vrect.x, scr_vrect.y, scr_turtle);
}

/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (!scr_shownet.value)
		return;
	
	if (realtime - cl.last_received_message < 0.3)
		return;
	if (cls.demoplayback)
		return;

	GL_SetCanvas (CANVAS_DEFAULT2); // woods

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, scr_net);
}

/*
==============
DrawPause
==============
*/
void SCR_DrawPause (void)
{
	qpic_t	*pic;

	if (!cl.paused)
		return;

	if (!scr_showpause.value)		// turn off for screenshots
		return;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	pic = Draw_CachePic ("gfx/pause.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
QSS-M Hints -- woods #qssmhints
==============
*/

char* hints[] = {
	"typing anything into the console searches for commands",
	"typing help into the console opens qss-m webpage in browser",
	"alt-enter swtiches between windowed and fullscreen mode",
	"pressing tab in console will auto-complete commands",
	"dragging a demo into windowed mode plays the demo",
	"arrow keys, scroll wheel, and pgup/pgdn adjust demo speed",
	"ctrl-u clears the console line",
	"cl_say allows you to tyle chat into the console",
	"connect last or reconnect will connect to last server",
	"connect + tab will autocomplete server history",
	"send a direct message by typing tell + first letters of name",
	"alt+shift+scrollwheel adjust game volume",
	"ctrl-m or mute will mute volume",
	"clear command will clear all console history",
	"type exec + tab to exec cfgs, or restore cfg backup",
	"uparrow and downarrow in the console for history",
	"add nickname to con_notifylist to flash client attentiion",
	"ctrl-enter in console will use messagemode2 (team)",
	"uparrow and downarrow in the console for history",
	"ctrl-home and ctrl-end jump to top/bottom of console",
	"type identify to identify the last person connected",
	"type lastid to see your last recorded ghost code",
	"type hints to print all hints to the console",
	"typing 'maps chamber' will search maps for chamber",
	"anything in end.cfg will be executed at match end",
	"anything in ctf.cfg will be executed if mode is ctf",
	"anything in dm.cfg will be executed if mode is dm",
	"type namemaker to make custom names with quake chars",
	"chat f_config, f_system, f_version, f_random for player info",
	"cl_smartspawn 1 can help train spawning with spacebar",
	"say_rand0 will randomly chat a line from say_rand0.txt",
	"typing open id1 or open screenshots etc will open folder",
	"w_switch values > 0 will disable all auto weapon switches",
	"use cl_enemycolor and cl_teamcolor to force colors"
};

char* random_hint;
int num_hints = sizeof(hints) / sizeof(hints[0]);

Uint32 HintTimer_Callback (Uint32 interval, void* param)
{
	int index = rand() % num_hints;
	random_hint = hints[index];
	return interval;
}

/*
==============
Print_Hints_f -- woods #qssmhints
==============
*/

void Print_Hints_f (void)
{
	Con_Printf("\n");
	for (int i = 0; i < num_hints; i++) 
	{
		Con_Printf("%s\n", hints[i]);
	}
	Con_Printf("\n");
}

/*
==============
DrawPause2 -- woods #showpaused #qssmhints
==============
*/
void SCR_DrawPause2(void)
{
	qpic_t* pic;
	char hint[80];
	static SDL_TimerID hint_timer_id = 0;

	GL_SetCanvas(CANVAS_MENU2); //johnfitz

	pic = Draw_CachePic("gfx/pause.lmp");

	if (cl.match_pause_time > 0 || pausedprint)
		Draw_Pic((320 - pic->width) / 2, (240 - 48 - pic->height) / 2, pic); //johnfitz -- stretched menus

	if ((cl.match_pause_time > 0 || pausedprint) && scr_hints.value)
	{
		GL_SetCanvas(CANVAS_HINT);

		if (!timerstarted) // only start timer once
		{ 
			random_hint = hints[rand() % num_hints];
			hint_timer_id = SDL_AddTimer(6000, HintTimer_Callback, NULL);
			timerstarted = true;
		}

		snprintf(hint, sizeof(hint), "%s", random_hint);
		M_Print(340 - (strlen(hint) * 4), 300, "QSS-M Hint:");
		M_PrintWhite(346 - ((strlen(hint) - 24) * 4), 300, hint);
	}
	else // remove timer when not paused, if it was started
	{ 
		if (timerstarted)
		{
			SDL_RemoveTimer(hint_timer_id);
			hint_timer_id = 0;
		}
		timerstarted = false;
	}

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
SCR_DrawLoading
==============
*/
void SCR_DrawLoading (void)
{
	qpic_t	*pic;

	if (!scr_drawloading)
		return;

	GL_SetCanvas (CANVAS_MENU2); //johnfitz

	pic = Draw_CachePic ("gfx/loading.lmp");
	Draw_Pic ( (320 - pic->width)/2, (240 - 48 - pic->height)/2, pic); //johnfitz -- stretched menus

	scr_tileclear_updates = 0; //johnfitz
}

/*
==============
SCR_DrawCrosshair -- johnfitz -- woods major change #crosshair
==============
*/
void SCR_DrawCrosshair (void)
{
	int x,hue;

	hue = 0;

	if (key_dest == key_menu) // woods #menuclear
		return;

	if (countdown && draw) // woods #clearcrxcountdown
		return;

	if (qeintermission && draw) // woods #qeintermission
		return;

	/*if (sb_showscores == true && (cl.gametype == GAME_DEATHMATCH && cls.state == ca_connected)) // woods don't overlap crosshair with scoreboard
		return;

	if (!crosshair.value || (!strcmp(cl.observer, "y")))
		return;*/

	if (cl.time <= cl.faceanimtime && cl_damagehue.value == 2)
		hue = 1;

	GL_SetCanvas (CANVAS_CROSSHAIR);

	x = 0;

	if (scr_crosshaircolor.value == 0)
	{ 
		if (hue)
			x = 234; // orange
		else
			x = 254;
	}
	if (scr_crosshaircolor.value == 1)
	{
		if (hue)
			x = 234; // orange
		else
			x = 192;
	}
	if (scr_crosshaircolor.value == 2)
	{
		if (hue)
			x = 254; // white
		else
			x = 251;
	}
	if (scr_crosshaircolor.value == 3)
	{ 
		if (hue)
			x = 254; // white
		else
			x = 208;
	}
	if (crosshair.value == 1)
		Draw_Fill(-2, 1, 3, 3, x, 1); // simple dot
	if (crosshair.value == 2)
	{ 
		Draw_Fill(-1, 7, 1, 8, x, 1);//  SOUTH
		Draw_Fill(4, 2, 8, 1, x, 1); //  WEST
		Draw_Fill(-5, 2, -8, 1, x, 1); // EAST
		Draw_Fill(-1, -2, 1, -8, x, 1); // NORTH
	}
	if (crosshair.value == 3)
	{
		Draw_Fill(-1, -6, 1, 17, x, 1); // vertical
		Draw_Fill(-9, 2, 17, 1, x, 1); //  horizontal
	}
	if (crosshair.value == 4)
	{
		Draw_Fill(-2, -6, 3, 17, x, 1); // vertical (thicker)
		Draw_Fill(-9, 1, 17, 3, x, 1); //  horizontal (thicker)
	}
	if (crosshair.value == 5)
	{
		Draw_Fill(-3, 0, 5, 5, 0, 1); // simple dot (black bg)
		Draw_Fill(-2, 1, 3, 3, x, 1); // simple dot
	}
	if (crosshair.value == 6)
	{
		Draw_Fill(-3, -7, 5, 19, 0, 1); // vertical (black bg)
		Draw_Fill(-10, 0, 19, 5, 0, 1); //  horizontal (black bg)
		Draw_Fill(-2, -6, 3, 17, x, 1); // vertical (thicker)
		Draw_Fill(-9, 1, 17, 3, x, 1); //  horizontal (thicker)
	}
	if (crosshair.value == 7)
		Draw_Character(-4, -4, '+'); //0,0 is center of viewport
}

/*
================
LaserSight - port from quakespasm-shalrathy / qrack --  woods #laser
================
*/
void LaserSight (void)
{
	char buf[15];
	const char* obs;
	obs = Info_GetKey(cl.scores[cl.realviewentity - 1].userinfo, "observer", buf, sizeof(buf));

	if (!strcmp(obs, "fly") || cl.stats[STAT_HEALTH] <= 0 || noclip_anglehack) // not in server flyme, noclip, or dead
		return;
	
	vec3_t	start, forward, right, up, crosshair, wall;
	float point1[3];

	// copy origin to start, offset it correctly

	AngleVectors(r_refdef.viewangles, forward, right, up);
	VectorCopy(cl.viewent.origin, start);
	start[2] += 16;//QuakeC uses + '0 0 16' for gun aim.

	// find the spot the player is looking at
	VectorMA(start, 4096, forward, crosshair);
	TraceLine(cl.viewent.origin, crosshair, wall);

	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	GL_PolygonOffset(OFFSET_SHOWTRIS);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

	// set point 1 to players position
	point1[0] = cl.entities[cl.viewentity].origin[0];
	point1[1] = cl.entities[cl.viewentity].origin[1];
	point1[2] = cl.entities[cl.viewentity].origin[2];

	wall[2] += -8.5; // adjust for relative crosshair

	// draw green line
	glColor4f(0.0, 1.0, 0.0, gl_laserpoint_alpha.value);
	glBegin(GL_LINES);
	if (gl_laserpoint.value == 2)
		glVertex3f(crosshair[0], crosshair[1], crosshair[2]);
	else
		glVertex3f(wall[0], wall[1], wall[2]);
	glVertex3f(point1[0], point1[1], point1[2]);
	glEnd();

	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	GL_PolygonOffset(OFFSET_NONE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (gl_laserpoint.value == 1)
		PScript_RunParticleEffectTypeString(wall, NULL, 1, "laserpoint"); // particle cfg "r_part laserpoint" for dot on wall
}

//=============================================================================


/*
==================
SCR_SetUpToDrawConsole
==================
*/
void SCR_SetUpToDrawConsole (void)
{
	//johnfitz -- let's hack away the problem of slow console when host_timescale is <0
	extern float frame_timescale; // woods #demorewind (Baker Fitzquake Mark V)
	//extern cvar_t host_timescale;
	//float timescale;
	//johnfitz

	Con_CheckResize ();

	if (scr_drawloading)
		return;		// never a console with loading plaque

// decide on the height of the console
	con_forcedup = !cl.worldmodel || cls.signon != SIGNONS;

	if (con_forcedup)
	{
		scr_conlines = glheight; //full screen //johnfitz -- glheight instead of vid.height
		scr_con_current = scr_conlines;
	}
	else if (key_dest == key_console)
	{
		scr_conlines = glheight * scr_consize.value; //johnfitz -- glheight instead of vid.height // woods #consize (joequake)
		if (scr_conlines < 50)
			scr_conlines = 50;
		if (scr_conlines > glheight - 50)
			scr_conlines = glheight - 50;
	}
	else
		scr_conlines = 0; //none visible

	//timescale = (host_timescale.value > 0) ? host_timescale.value : 1; //johnfitz -- timescale

	if (scr_conlines < scr_con_current)
	{
		// ericw -- (glheight/600.0) factor makes conspeed resolution independent, using 800x600 as a baseline
		scr_con_current -= scr_conspeed.value * host_frametime / frame_timescale; //johnfitz -- timescale // woods #demorewind (Baker Fitzquake Mark V)
	//	scr_con_current -= scr_conspeed.value*(glheight/600.0)*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;
	}
	else if (scr_conlines > scr_con_current)
	{
		// ericw -- (glheight/600.0)
		scr_con_current += scr_conspeed.value * (glheight / 600.0) * host_frametime / frame_timescale; //johnfitz -- timescale // woods #demorewind (Baker Fitzquake Mark V)
		//scr_con_current += scr_conspeed.value*(glheight/600.0)*host_frametime/timescale; //johnfitz -- timescale
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

	if (clearconsole++ < vid.numpages)
		Sbar_Changed ();
	else
		con_notifylines_ = 0; // woods from proquake 493 #notifylines

	if (!con_forcedup && scr_con_current)
		scr_tileclear_updates = 0; //johnfitz
}

/*
==================
SCR_DrawConsole
==================
*/
void SCR_DrawConsole (void)
{
	if (scr_con_current)
	{
		Con_DrawConsole (scr_con_current, true);
		clearconsole = 0;
	}
	else
	{
		if (key_dest == key_game || key_dest == key_message)
			Con_DrawNotify ();	// only draw notify in game
	}
}


/*
==============================================================================

SCREEN SHOTS

==============================================================================
*/

//======================================================
// woods #screenshotcopy from fitzquake markvr9
//======================================================

#if defined(_WIN32)
static void FlipBuffer(byte* buffer, const int columns, const int rows, const int BytesPerPixel)	// Flip the image because of GL's up = positive-y
{
	int		bufsize = columns * BytesPerPixel; // bufsize=widthBytes;

	byte* tb1 = malloc(bufsize);
	byte* tb2 = malloc(bufsize);
	int		i, offset1, offset2;

	for (i = 0; i < (rows + 1) / 2;i++)
	{
		offset1 = i * bufsize;
		offset2 = ((rows - 1) - i) * bufsize;

		memcpy(tb1, buffer + offset1, bufsize);
		memcpy(tb2, buffer + offset2, bufsize);
		memcpy(buffer + offset1, tb2, bufsize);
		memcpy(buffer + offset2, tb1, bufsize);
	}

	free(tb1);
	free(tb2);
	return;
}

void SCR_ScreenShot_Clipboard_f(void)
{
	int		buffersize = glwidth * glheight * 4; // 4 bytes per pixel
	byte* buffer = malloc(buffersize);

	//get data
	glReadPixels(glx, gly, glwidth, glheight, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);

	// We are upside down flip it
	FlipBuffer(buffer, glwidth, glheight, 4 /* bytes per pixel */);

	// FIXME: No gamma correction of screenshots in Fitz?
	Sys_Image_BGRA_To_Clipboard(buffer, glwidth, glheight, buffersize);

	//Con_Printf("\nscreenshot copied to clipboard\n");

	free(buffer);
}
#endif

static void SCR_ScreenShot_Usage (void)
{
	Con_Printf ("usage: screenshot <format> <quality>\n");
	Con_Printf ("   format must be \"png\" or \"tga\" or \"jpg\"\n");
	Con_Printf ("   quality must be 1-100\n");
	return;
}

/*
==================
SCR_ScreenShot_f -- johnfitz -- rewritten to use Image_WriteTGA
==================
*/
void SCR_ScreenShot_f (void)
{
	byte	*buffer;
	char	ext[4];
	char	imagename[MAX_OSPATH];  //johnfitz -- was [80] // woods #screenshots was 16
	char	checkname[MAX_OSPATH];
	int	quality;
	qboolean	ok;

	// woods added time for demo output // woods #screenshots
	char str[24];
	time_t systime = time(0);
	struct tm loct = *localtime(&systime);

	strftime(str, 24, "%m-%d-%Y-%H%M%S", &loct); // time and date support

	q_snprintf(checkname, sizeof(checkname), "%s/screenshots", com_gamedir); // woods #screenshots
	Sys_mkdir(checkname); //  woods create screenshots if not there #screenshots
	
	Q_strncpy (ext, "png", sizeof(ext));

	if (Cmd_Argc () >= 2)
	{
		const char	*requested_ext = Cmd_Argv (1);

		if (!q_strcasecmp ("png", requested_ext)
		    || !q_strcasecmp ("tga", requested_ext)
		    || !q_strcasecmp ("jpg", requested_ext))
			Q_strncpy (ext, requested_ext, sizeof(ext));
		else
		{
			SCR_ScreenShot_Usage ();
			return;
		}
	}

// read quality as the 3rd param (only used for JPG)
	quality = 90;
	if (Cmd_Argc () >= 3)
		quality = Q_atoi (Cmd_Argv(2));
	if (quality < 1 || quality > 100)
	{
		SCR_ScreenShot_Usage ();
		return;
	}
	
	/*
// find a file name to save it to
	for (i=0; i<10000; i++)
	{
		q_snprintf (imagename, sizeof(imagename), "screenshots/qssm%04i.%s", i, ext);	// "fitz%04i.tga" // woods #screenshots
		q_snprintf (checkname, sizeof(checkname), "%s/%s", com_gamedir, imagename);
		if (Sys_FileTime(checkname) == -1)
			break;	// file doesn't exist
	}
	if (i == 10000)
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't find an unused filename\n");
		return;
	}*/

	q_snprintf(imagename, sizeof(imagename), "screenshots/%s_%s.%s", cl.mapname, str, ext);	// woods #screenshots time and date support
	q_snprintf(checkname, sizeof(checkname), "%s/%s", com_gamedir, imagename);

//get data
	if (!(buffer = (byte *) malloc(glwidth*glheight*3)))
	{
		Con_Printf ("SCR_ScreenShot_f: Couldn't allocate memory\n");
		return;
	}

	glPixelStorei (GL_PACK_ALIGNMENT, 1);/* for widths that aren't a multiple of 4 */
	glReadPixels (glx, gly, glwidth, glheight, GL_RGB, GL_UNSIGNED_BYTE, buffer);

// now write the file
	if (!q_strncasecmp (ext, "png", sizeof(ext)))
		ok = Image_WritePNG (imagename, buffer, glwidth, glheight, 24, false);
	else if (!q_strncasecmp (ext, "tga", sizeof(ext)))
		ok = Image_WriteTGA (imagename, buffer, glwidth, glheight, 24, false);
	else if (!q_strncasecmp (ext, "jpg", sizeof(ext)))
		ok = Image_WriteJPG (imagename, buffer, glwidth, glheight, 24, quality, false);
	else
		ok = false;

	if (ok)
	{ 
		Con_Printf ("Wrote %s\n", imagename);
		S_LocalSound("player/tornoff2.wav"); // woods add sound to screenshot
	}
	else
		Con_Printf ("SCR_ScreenShot_f: Couldn't create %s\n", imagename);

#if defined(_WIN32)
	SCR_ScreenShot_Clipboard_f();	// woods #screenshotcopy
#endif

	free (buffer);
}


//=============================================================================


/*
===============
SCR_BeginLoadingPlaque

================
*/
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds (true);

	if (cls.state != ca_connected)
		return;
	if (cls.signon != SIGNONS)
		return;

// redraw with no console and the loading plaque
	Con_ClearNotify ();
	scr_centertime_off = 0;
	scr_con_current = 0;

	scr_drawloading = true;
	Sbar_Changed ();
	SCR_UpdateScreen ();
	scr_drawloading = false;

	scr_disabled_for_loading = true;
	scr_disabled_time = realtime;
}

/*
===============
SCR_EndLoadingPlaque

================
*/
void SCR_EndLoadingPlaque (void)
{
	scr_disabled_for_loading = false;
	Con_ClearNotify ();
}

//=============================================================================

const char	*scr_notifystring;
qboolean	scr_drawdialog;

void SCR_DrawNotifyString (void)
{
	const char	*start;
	int		l;
	int		j;
	int		x, y;

	GL_SetCanvas (CANVAS_MENU); //johnfitz

	start = scr_notifystring;

	y = 200 * 0.35; //johnfitz -- stretched overlays

	do
	{
	// scan the width of the line
		for (l=0 ; l<40 ; l++)
			if (start[l] == '\n' || !start[l])
				break;
		x = (320 - l*8)/2; //johnfitz -- stretched overlays
		for (j=0 ; j<l ; j++, x+=8)
			Draw_Character (x, y, start[j]);

		y += 8;

		while (*start && *start != '\n')
			start++;

		if (!*start)
			break;
		start++;		// skip the \n
	} while (1);
}

/*
==================
SCR_ModalMessage

Displays a text string in the center of the screen and waits for a Y or N
keypress.
==================
*/
int SCR_ModalMessage (const char *text, float timeout) //johnfitz -- timeout
{
	double time1, time2; //johnfitz -- timeout
	int lastkey, lastchar;

	if (cls.state == ca_dedicated)
		return true;

	scr_notifystring = text;

// draw a fresh screen
	scr_drawdialog = true;
	SCR_UpdateScreen ();
	scr_drawdialog = false;

	S_ClearBuffer ();		// so dma doesn't loop current sound

	time1 = Sys_DoubleTime () + timeout; //johnfitz -- timeout
	time2 = 0.0f; //johnfitz -- timeout

	Key_BeginInputGrab ();
	do
	{
		Sys_SendKeyEvents ();
		Key_GetGrabbedInput (&lastkey, &lastchar);
		Sys_Sleep (16);
		if (timeout) time2 = Sys_DoubleTime (); //johnfitz -- zero timeout means wait forever.
	} while (lastchar != 'y' && lastchar != 'Y' &&
		 lastchar != 'n' && lastchar != 'N' &&
		 lastkey != K_ESCAPE &&
		 lastkey != K_ABUTTON &&
		 lastkey != K_BBUTTON &&
		 time2 <= time1);
	Key_EndInputGrab ();

//	SCR_UpdateScreen (); //johnfitz -- commented out

	//johnfitz -- timeout
	if (time2 > time1)
		return false;
	//johnfitz

	return (lastchar == 'y' || lastchar == 'Y' || lastkey == K_ABUTTON);
}


//=============================================================================

//johnfitz -- deleted SCR_BringDownConsole


/*
==================
SCR_TileClear
johnfitz -- modified to use glwidth/glheight instead of vid.width/vid.height
	    also fixed the dimentions of right and top panels
	    also added scr_tileclear_updates
==================
*/
void SCR_TileClear (void)
{
	//ericw -- added check for glsl gamma. TODO: remove this ugly optimization?
	if (scr_tileclear_updates >= vid.numpages && !gl_clear.value && !(gl_glsl_gamma_able && vid_gamma.value != 1))
		return;
	scr_tileclear_updates++;

	if (r_refdef.vrect.x > 0)
	{
		// left
		Draw_TileClear (0,
						0,
						r_refdef.vrect.x,
						glheight - sb_lines);
		// right
		Draw_TileClear (r_refdef.vrect.x + r_refdef.vrect.width,
						0,
						glwidth - r_refdef.vrect.x - r_refdef.vrect.width,
						glheight - sb_lines);
	}

	if (r_refdef.vrect.y > 0)
	{
		// top
		Draw_TileClear (r_refdef.vrect.x,
						0,
						r_refdef.vrect.width,
						r_refdef.vrect.y);
		// bottom
		Draw_TileClear (r_refdef.vrect.x,
						r_refdef.vrect.y + r_refdef.vrect.height,
						r_refdef.vrect.width,
						glheight - r_refdef.vrect.y - r_refdef.vrect.height - sb_lines);
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.

WARNING: be very careful calling this from elsewhere, because the refresh
needs almost the entire 256k of stack space!
==================
*/
void SCR_UpdateScreen (void)
{
	vid.numpages = (gl_triplebuffer.value) ? 3 : 2;

	if (scr_disabled_for_loading)
	{
		if (realtime - scr_disabled_time > 60)
		{
			scr_disabled_for_loading = false;
			Con_Printf ("load failed.\n");
		}
		else
			return;
	}

	if (!scr_initialized || !con_initialized)
		return;				// not initialized yet


	GL_BeginRendering (&glx, &gly, &glwidth, &glheight);

	if (cl.worldmodel && cl.qcvm.worldmodel && cl.qcvm.extfuncs.CSQC_UpdateView)
	{
		float s = CLAMP (1.0, scr_sbarscale.value, (float)glwidth / 320.0);
		SCR_SetUpToDrawConsole ();
		GL_SetCanvas (CANVAS_CSQC);

		PR_SwitchQCVM(&cl.qcvm);

		if (qcvm->extglobals.cltime)
			*qcvm->extglobals.cltime = realtime;
		if (qcvm->extglobals.clframetime)
			*qcvm->extglobals.clframetime = host_frametime;
		if (qcvm->extglobals.player_localentnum)
			*qcvm->extglobals.player_localentnum = cl.viewentity;
		if (qcvm->extglobals.intermission)
			*qcvm->extglobals.intermission = cl.intermission;
		if (qcvm->extglobals.intermission_time)
			*qcvm->extglobals.intermission_time = cl.completed_time;
		if (qcvm->extglobals.view_angles)
			VectorCopy(cl.viewangles, qcvm->extglobals.view_angles);
		if (qcvm->extglobals.clientcommandframe)
			*qcvm->extglobals.clientcommandframe = cl.movemessages;
		if (qcvm->extglobals.servercommandframe)
			*qcvm->extglobals.servercommandframe = cl.ackedmovemessages;
//		Sbar_SortFrags ();

		pr_global_struct->time = qcvm->time;
		pr_global_struct->frametime = qcvm->frametime;
		G_FLOAT(OFS_PARM0) = glwidth/s;
		G_FLOAT(OFS_PARM1) = glheight/s;
		G_FLOAT(OFS_PARM2) = true;
		PR_ExecuteProgram(cl.qcvm.extfuncs.CSQC_UpdateView);
		PR_SwitchQCVM(NULL);

		GL_Set2D ();
	}
	else
	{
		//
		// determine size of refresh window
		//
		r_refdef.drawworld = true;
		if (vid.recalc_refdef)
			SCR_CalcRefdef ();

//
// do 3D refresh drawing, and then update the screen
//
		SCR_SetUpToDrawConsole ();

		V_RenderView ();

		GL_Set2D ();

		//FIXME: only call this when needed
		SCR_TileClear ();

		if (!cl.intermission)
		{
			Sbar_Draw ();
			if (!scr_drawloading && !con_forcedup)
				SCR_DrawCrosshair (); //johnfitz
		}
	}

	if (scr_drawdialog) //new game confirm
	{
		if (con_forcedup)
			Draw_ConsoleBackground ();
		Draw_FadeScreen ();
		SCR_DrawNotifyString ();
	}
	else if (scr_drawloading) //loading
	{
		SCR_DrawLoading ();
	}
	else if (cl.intermission == 1 && key_dest == key_game) //end of level
	{
		Sbar_IntermissionOverlay ();
	}
	else if (cl.intermission == 2 && key_dest == key_game) //end of episode
	{
		Sbar_FinaleOverlay ();
		SCR_CheckDrawCenterString ();
	}
	else
	{
		SCR_DrawNet ();
		SCR_DrawTurtle ();
		SCR_DrawPause ();
		SCR_DrawPause2 (); // woods #showpaused
		SCR_CheckDrawCenterString ();
		SCR_DrawDevStats (); //johnfitz
		SCR_DrawFPS (); //johnfitz
		SCR_DrawClock (); //johnfitz
		SCR_ShowPing (); // woods #scrping
		SCR_ShowPL (); // woods #scrpl
		SCR_DrawMatchClock (); // woods #matchhud
		SCR_DrawMatchScores (); // woods #matchhud
		SCR_ShowFlagStatus (); // woods #matchhud #flagstatus
		SCR_ShowObsFrags (); // woods #observerhud
		SCR_DrawSpeed (); // woods #speed
		SCR_Mute (); // woods #usermute
		SCR_Observing (); // woods
		SCR_DrawConsole ();
		M_Draw ();
	}

	V_UpdateBlend (); //johnfitz -- V_UpdatePalette cleaned up and renamed

	GLSLGamma_GammaCorrect ();

	GL_EndRendering ();
}

