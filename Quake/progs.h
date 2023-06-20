/*
Copyright (C) 1996-2001 Id Software, Inc.
Copyright (C) 2002-2009 John Fitzgibbons and others
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

#ifndef QUAKE_PROGS_H
#define QUAKE_PROGS_H

#include "pr_comp.h"	/* defs shared with qcc */
#include "progdefs.h"	/* generated by program cdefs */

typedef union eval_s
{
	string_t	string;
	float		_float;
	float		vector[3];
	func_t		function;
	int		_int;
	int		edict;
} eval_t;

#define	MAX_ENT_LEAFS	32
typedef struct edict_s
{
	qboolean	free;
	link_t		area;			/* linked to a division node or leaf */

	unsigned int		num_leafs;
	int		leafnums[MAX_ENT_LEAFS];

	entity_state_t	baseline;
	unsigned char	alpha;			/* johnfitz -- hack to support alpha since it's not part of entvars_t */
	qboolean	sendinterval;		/* johnfitz -- send time until nextthink to client for better lerp timing */
	qboolean	onladder;			/* spike -- content_ladder stuff */

	float		freetime;		/* sv.time when the object was freed */
	entvars_t	v;			/* C exported fields from progs */

	/* other fields from progs come immediately after */
} edict_t;

#define	EDICT_FROM_AREA(l)	STRUCT_FROM_LINK(l,edict_t,area)

//============================================================================

typedef void (*builtin_t) (void);
typedef struct qcvm_s qcvm_t;

void PR_Init (void);

void PR_ExecuteProgram (func_t fnum);
void PR_ClearProgs(qcvm_t *vm);
qboolean PR_LoadProgs (const char *filename, qboolean fatal, unsigned int needcrc, const builtin_t *builtins, size_t numbuiltins);

//from pr_ext.c
void PR_InitExtensions(void);
void PR_EnableExtensions(ddef_t *pr_globaldefs);	//adds in the extra builtins etc
void PR_AutoCvarChanged(cvar_t *var);				//updates the autocvar_ globals when their cvar is changed
void PR_ShutdownExtensions(void);					//nooooes!
void PR_ReloadPics(qboolean purge);					//for gamedir or video changes
func_t PR_FindExtFunction(const char *entryname);
void PR_DumpPlatform_f(void);						//console command: writes out a qsextensions.qc file
//special hacks...
int PF_SV_ForceParticlePrecache(const char *s);
int SV_Precache_Model(const char *s);
int SV_Precache_Sound(const char *s);
void PR_spawnfunc_misc_model(edict_t *self);

//from pr_edict, for pr_ext. reflection is messy.
qboolean	ED_ParseEpair (void *base, ddef_t *key, const char *s, qboolean zoned);
const char *PR_UglyValueString (int type, eval_t *val);
ddef_t *ED_FindField (const char *name);
ddef_t *ED_FindGlobal (const char *name);
dfunction_t *ED_FindFunction (const char *fn_name);

const char *PR_GetString (int num);
int PR_SetEngineString (const char *s);
int PR_AllocString (int bufferlength, char **ptr);
void PR_ClearEngineString(int num);

void PR_Profile_f (void);

edict_t *ED_Alloc (void);
void ED_Free (edict_t *ed);

void ED_Print (edict_t *ed);
void ED_Write (FILE *f, edict_t *ed);
const char *ED_ParseEdict (const char *data, edict_t *ent);

void ED_WriteGlobals (FILE *f);
const char *ED_ParseGlobals (const char *data);

void ED_LoadFromFile (const char *data);

/*
#define EDICT_NUM(n)		((edict_t *)(sv.edicts+ (n)*pr_edict_size))
#define NUM_FOR_EDICT(e)	(((byte *)(e) - sv.edicts) / pr_edict_size)
*/
edict_t *EDICT_NUM(int);
int NUM_FOR_EDICT(edict_t*);

#define	NEXT_EDICT(e)		((edict_t *)( (byte *)e + qcvm->edict_size))

#define	EDICT_TO_PROG(e)	((byte *)e - (byte *)qcvm->edicts)
#define PROG_TO_EDICT(e)	((edict_t *)((byte *)qcvm->edicts + e))

#define	G_FLOAT(o)		(qcvm->globals[o])
#define	G_INT(o)		(*(int *)&qcvm->globals[o])
#define	G_EDICT(o)		((edict_t *)((byte *)qcvm->edicts+ *(int *)&qcvm->globals[o]))
#define G_EDICTNUM(o)		NUM_FOR_EDICT(G_EDICT(o))
#define	G_VECTOR(o)		(&qcvm->globals[o])
#define	G_STRING(o)		(PR_GetString(*(string_t *)&qcvm->globals[o]))
#define	G_FUNCTION(o)		(*(func_t *)&qcvm->globals[o])

#define G_VECTORSET(r,x,y,z) do{G_FLOAT((r)+0) = x; G_FLOAT((r)+1) = y;G_FLOAT((r)+2) = z;}while(0)

#define	E_FLOAT(e,o)		(((float*)&e->v)[o])
#define	E_INT(e,o)		(*(int *)&((float*)&e->v)[o])
#define	E_VECTOR(e,o)		(&((float*)&e->v)[o])
#define	E_STRING(e,o)		(PR_GetString(*(string_t *)&((float*)&e->v)[o]))

extern	int		type_size[8];

FUNC_NORETURN void PR_RunError (const char *error, ...) FUNC_PRINTF(1,2);
void PR_RunWarning (const char *error, ...) FUNC_PRINTF(1,2);
#ifdef __WATCOMC__
#pragma aux PR_RunError aborts;
#endif

void ED_PrintEdicts (void);
void ED_PrintNum (int ent);

eval_t *GetEdictFieldValue(edict_t *ed, int fldofs);	//handles invalid offsets with a null
int ED_FindFieldOffset (const char *name);

#define GetEdictFieldValid(fld) (qcvm->extfields.fld>=0)
#define GetEdictFieldEval(ed,fld) ((eval_t *)((char *)&ed->v + qcvm->extfields.fld*4)) //caller must validate the field first

//from pr_cmds, no longer static so that pr_ext can use them.
sizebuf_t *WriteDest (void);
char *PR_GetTempString (void);
int PR_MakeTempString (const char *val);
char *PF_VarString (int	first);
#define	STRINGTEMP_BUFFERS		1024
#define	STRINGTEMP_LENGTH		1024
void PF_Fixme(void);	//the 'unimplemented' builtin. woot.

struct pr_extfuncs_s
{
/*all vms*/
#define QCEXTFUNCS_COMMON \
	QCEXTFUNC(GameCommand,				"void(string cmdtext)")												/*obsoleted by m_consolecommand, included for dp compat.*/	\
/*csqc+ssqc*/
#define QCEXTFUNCS_GAME \
	QCEXTFUNC(EndFrame,					"void()")				\
/*ssqc*/
#define QCEXTFUNCS_SV \
	QCEXTFUNC(SV_ParseClientCommand,		"void(string cmd)")		\
	QCEXTFUNC(SV_RunClientCommand,		"void()")		\
/*csqc*/
#define QCEXTFUNCS_CS \
	QCEXTFUNC(CSQC_Init,				"void(float apilevel, string enginename, float engineversion)")	\
	QCEXTFUNC(CSQC_Shutdown,			"void()")	\
	QCEXTFUNC(CSQC_DrawHud,				"void(vector virtsize, float showscores)")							/*simple: for the simple(+limited) hud-only csqc interface.*/	\
	QCEXTFUNC(CSQC_DrawScores,			"void(vector virtsize, float showscores)")							/*simple: (optional) for the simple hud-only csqc interface.*/		\
	QCEXTFUNC(CSQC_InputEvent,			"float(float evtype, float scanx, float chary, float devid)")		\
	QCEXTFUNC(CSQC_ConsoleCommand,		"float(string cmdstr)")												\
	QCEXTFUNC(CSQC_Parse_Event,			"void()")															\
	QCEXTFUNC(CSQC_Parse_Damage,		"float(float save, float take, vector dir)")						\
	QCEXTFUNC(CSQC_UpdateView,			"void(float vwidth, float vheight, float notmenu)")					/*full only: for the full csqc-draws-entire-screen interface*/	\
	QCEXTFUNC(CSQC_Input_Frame,			"void()")															/*full only: input angles stuff.*/	\
	QCEXTFUNC(CSQC_Parse_CenterPrint,	"float(string msg)")												\
	QCEXTFUNC(CSQC_Parse_Print,			"void(string printmsg, float printlvl)")							\
	QCEXTFUNC(CSQC_Ent_Update,			"void(float isnew)")												/*full: lots of reads needed to interpret the bytestream*/	\
	QCEXTFUNC(CSQC_Ent_Remove,			"void()")															/*full: basically just remove(self)*/\
	QCEXTFUNC(CSQC_Event_Sound,			"void(float entnum, float channel, string soundname, float vol, float attenuation, vector pos, float pitchmod, float flags)") /*full: basically just remove(self)*/\
	QCEXTFUNC(CSQC_Parse_TempEntity,	"float()")															/*full: evil... This is the bane of all protocol compatibility. Die.*/	\
	QCEXTFUNC(CSQC_Parse_StuffCmd,		"void(string msg)")													/*full only: not in simple. Too easy to make cheats by ignoring server messages.*/	\
/*menucsqc*/
#define QCEXTFUNCS_MENU \
	QCEXTFUNC(m_init,					"void()")															\
	QCEXTFUNC(m_toggle,					"void(float wantmode)")												/*-1: toggle, 0: clear, 1: force*/	\
	QCEXTFUNC(m_draw,					"void(vector screensize)")											\
	QCEXTFUNC(m_keydown,				"void(float scan, float chr)")										/*obsoleted by Menu_InputEvent, included for dp compat.*/	\
	QCEXTFUNC(m_keyup,					"void(float scan, float chr)")										/*obsoleted by Menu_InputEvent, included for dp compat.*/	\
	QCEXTFUNC(m_consolecommand,			"float(string cmd)")												\
	QCEXTFUNC(Menu_InputEvent,			"float(float evtype, float scanx, float chary, float devid)")		\

#define QCEXTFUNC(n,t) func_t n;
	QCEXTFUNCS_COMMON
	QCEXTFUNCS_GAME
	QCEXTFUNCS_SV
	QCEXTFUNCS_CS
	QCEXTFUNCS_MENU
#undef QCEXTFUNC
};
extern	cvar_t	pr_checkextension;	//if 0, extensions are disabled (unless they'd be fatal, but they're still spammy)

struct pr_extglobals_s
{
#define QCEXTGLOBALS_COMMON \
	QCEXTGLOBAL_FLOAT(time)\
	QCEXTGLOBAL_FLOAT(frametime)\
	//end
#define QCEXTGLOBALS_GAME \
	QCEXTGLOBAL_FLOAT(input_timelength)\
	QCEXTGLOBAL_VECTOR(input_movevalues)\
	QCEXTGLOBAL_VECTOR(input_angles)\
	QCEXTGLOBAL_FLOAT(input_buttons)\
	QCEXTGLOBAL_FLOAT(input_impulse)\
	QCEXTGLOBAL_INT(input_weapon)\
	QCEXTGLOBAL_VECTOR(input_cursor_screen)\
	QCEXTGLOBAL_VECTOR(input_cursor_trace_start)\
	QCEXTGLOBAL_VECTOR(input_cursor_trace_endpos)\
	QCEXTGLOBAL_FLOAT(input_cursor_entitynumber)\
	QCEXTGLOBAL_FLOAT(physics_mode)\
	//end
#define QCEXTGLOBALS_CSQC \
	QCEXTGLOBAL_FLOAT(cltime)\
	QCEXTGLOBAL_FLOAT(clframetime)\
	QCEXTGLOBAL_FLOAT(maxclients)\
	QCEXTGLOBAL_FLOAT(intermission)\
	QCEXTGLOBAL_FLOAT(intermission_time)\
	QCEXTGLOBAL_FLOAT(player_localnum)\
	QCEXTGLOBAL_FLOAT(player_localentnum)\
	QCEXTGLOBAL_VECTOR(view_angles)\
	QCEXTGLOBAL_FLOAT(clientcommandframe)\
	QCEXTGLOBAL_FLOAT(servercommandframe)\
	//end
#define QCEXTGLOBAL_FLOAT(n) float *n;
#define QCEXTGLOBAL_INT(n) int *n;
#define QCEXTGLOBAL_VECTOR(n) float *n;
	QCEXTGLOBALS_COMMON
	QCEXTGLOBALS_GAME
	QCEXTGLOBALS_CSQC
#undef QCEXTGLOBAL_FLOAT
#undef QCEXTGLOBAL_INT
#undef QCEXTGLOBAL_VECTOR
};

struct pr_extfields_s
{	//various fields that might be wanted by the engine. -1 == invalid

#define QCEXTFIELDS_ALL	\
	/*renderscene means we need a number of fields here*/	\
	QCEXTFIELD(alpha,					".float")				/*float*/	\
	QCEXTFIELD(scale,					".float")				/*float*/	\
	QCEXTFIELD(colormod,				".vector")			/*vector*/	\
	QCEXTFIELD(tag_entity,				".entity")			/*entity*/	\
	QCEXTFIELD(tag_index,				".float")			/*float*/	\
	QCEXTFIELD(modelflags,				".float")			/*float, the upper 8 bits of .effects*/	\
	QCEXTFIELD(origin,					".vector")				/*for menuqc's addentity builtin.*/	\
	QCEXTFIELD(angles,					".vector")				/*for menuqc's addentity builtin.*/	\
	QCEXTFIELD(frame,					".float")				/*for menuqc's addentity builtin.*/	\
	QCEXTFIELD(skin,					".float")				/*for menuqc's addentity builtin.*/	\
	/*end of list*/
#define QCEXTFIELDS_GAME	\
	/*stuff used by csqc+ssqc, but not menu*/	\
	QCEXTFIELD(customphysics,			".void()")/*function*/	\
	QCEXTFIELD(gravity,					".float")			/*float*/	\
	//end of list
#define QCEXTFIELDS_CL	\
	QCEXTFIELD(frame2,					".float")				/*for csqc's addentity builtin.*/	\
	QCEXTFIELD(lerpfrac,				".float")			/*for csqc's addentity builtin.*/	\
	QCEXTFIELD(frame1time,				".float")			/*for csqc's addentity builtin.*/	\
	QCEXTFIELD(frame2time,				".float")			/*for csqc's addentity builtin.*/	\
	QCEXTFIELD(renderflags,				".float")		/*for csqc's addentity builtin.*/	\
	//end of list
#define QCEXTFIELDS_CS	\
	/*csqc-only*/	\
	QCEXTFIELD(entnum,					".float")				/*for csqc.*/	\
	QCEXTFIELD(drawmask,				".float")			/*for csqc's addentities builtin.*/	\
	QCEXTFIELD(predraw,					".float()")			/*for csqc's addentities builtin.*/	\
	//end of list
#define QCEXTFIELDS_SS	\
	/*ssqc-only*/	\
	QCEXTFIELD(items2,					"//.float")				/*float*/	\
	QCEXTFIELD(movement,				".vector")			/*vector*/	\
	QCEXTFIELD(viewmodelforclient,		".entity")	/*entity*/	\
	QCEXTFIELD(exteriormodeltoclient,	".entity")	/*entity*/	\
	QCEXTFIELD(traileffectnum,			".float")		/*float*/	\
	QCEXTFIELD(emiteffectnum,			".float")		/*float*/	\
	QCEXTFIELD(button3,					".float")			/*float*/	\
	QCEXTFIELD(button4,					".float")			/*float*/	\
	QCEXTFIELD(button5,					".float")			/*float*/	\
	QCEXTFIELD(button6,					".float")			/*float*/	\
	QCEXTFIELD(button7,					".float")			/*float*/	\
	QCEXTFIELD(button8,					".float")			/*float*/	\
	QCEXTFIELD(viewzoom,				".float")			/*float*/	\
	QCEXTFIELD(SendEntity,				".float(entity to, float changedflags)")			/*function*/	\
	QCEXTFIELD(SendFlags,				".float")			/*float. :( */	\
	//end of list

#define QCEXTFIELD(n,t) int n;
	QCEXTFIELDS_ALL
	QCEXTFIELDS_GAME
	QCEXTFIELDS_CL
	QCEXTFIELDS_CS
	QCEXTFIELDS_SS
#undef QCEXTFIELD
};

typedef struct
{
	int		s;
	dfunction_t	*f;
} prstack_t;


typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	link_t	trigger_edicts;
	link_t	solid_edicts;
} areanode_t;
#define	AREA_DEPTH	4
#define	AREA_NODES	32

#define CSIE_KEYDOWN			0
#define CSIE_KEYUP				1
#define CSIE_MOUSEDELTA			2
#define CSIE_MOUSEABS			3
//#define CSIE_ACCELEROMETER	4
//#define CSIE_FOCUS			5
#define CSIE_JOYAXIS			6
//#define CSIE_GYROSCOPE		7

struct qcvm_s
{
	dprograms_t	*progs;
	dfunction_t	*functions;
	dstatement_t	*statements;
	float		*globals;	/* same as pr_global_struct */
	ddef_t		*fielddefs;	//yay reflection.

	int			edict_size;	/* in bytes */

	builtin_t	builtins[1024];
	int			numbuiltins;

	int			argc;

	qboolean	trace;
	dfunction_t	*xfunction;
	int			xstatement;

	unsigned short	progscrc;	//crc16 of the entire file
	unsigned int	progshash;	//folded file md4
	unsigned int	progssize;	//file size (bytes)

	struct pr_extglobals_s extglobals;
	struct pr_extfuncs_s extfuncs;
	struct pr_extfields_s extfields;

	qboolean cursorforced;
	void *cursorhandle;	//video code.
	qboolean nogameaccess;	//simplecsqc isn't allowed to poke properties of the actual game (to prevent cheats when there's no restrictions on what it can access)
	qboolean brokenbouncemissile;	//2021 rerelease redefined it, breaking any mod that depends on it.
	qboolean brokenpushrotate;		//2021 rerelease fucks over avelocity on movetype_push.
	qboolean brokeneffects;			//2021 rerelease redefined EF_RED and EF_BLUE.
	qboolean precacheanytime; //mod queried for support. this is used to spam warnings to anyone that doesn't bother checking for it first. this annoyance is to reduce compat issues.

	//was static inside pr_edict
	char		*strings;
	int			stringssize;
	const char	**knownstrings;
	int			maxknownstrings;
	int			numknownstrings;
	int			freeknownstrings;
	ddef_t		*globaldefs;

	unsigned char *knownzone;
	size_t knownzonesize;

	//originally defined in pr_exec, but moved into the switchable qcvm struct
#define	MAX_STACK_DEPTH		1024 /*was 64*/	/* was 32 */
	prstack_t	stack[MAX_STACK_DEPTH];
	int			depth;

#define	LOCALSTACK_SIZE		16384 /* was 2048*/
	int			localstack[LOCALSTACK_SIZE];
	int			localstack_used;

	//originally part of the sv_state_t struct
	//FIXME: put worldmodel in here too.
	double		time;
	double		frametime;
	int			num_edicts;
	int			reserved_edicts;
	int			max_edicts;
	edict_t		*edicts;			// can NOT be array indexed, because edict_t is variable sized, but can be used to reference the world ent
	struct qmodel_s	*worldmodel;
	struct qmodel_s	*(*GetModel)(int modelindex);	//returns the model for the given index, or null.

	//originally from world.c
	areanode_t	areanodes[AREA_NODES];
	int			numareanodes;
};
extern globalvars_t	*pr_global_struct;

#if 0
extern qcvm_t ssqcvm;
#define qcvm (&ssqcvm)
#define PR_SwitchQCVM(n)
#else
extern qcvm_t *qcvm;
void PR_SwitchQCVM(qcvm_t *nvm);
#endif

extern const builtin_t pr_ssqcbuiltins[];
extern const int pr_ssqcnumbuiltins;
extern const builtin_t pr_csqcbuiltins[];
extern const int pr_csqcnumbuiltins;
extern const builtin_t pr_menubuiltins[];
extern int const pr_menunumbuiltins;

#endif	/* QUAKE_PROGS_H */
