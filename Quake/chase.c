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
// chase.c -- chase camera code

#include "quakedef.h"

cvar_t	chase_back = {"chase_back", "100", CVAR_NONE};
cvar_t	chase_up = {"chase_up", "16", CVAR_NONE};
cvar_t	chase_right = {"chase_right", "0", CVAR_NONE};
cvar_t	chase_active = {"chase_active", "0", CVAR_NONE};

/*
==============
Chase_Init
==============
*/
void Chase_Init (void)
{
	Cvar_RegisterVariable (&chase_back);
	Cvar_RegisterVariable (&chase_up);
	Cvar_RegisterVariable (&chase_right);
	Cvar_RegisterVariable (&chase_active);
}

extern int SV_HullPointContents(hull_t* hull, int num, vec3_t p); // woods(Qrack) #betterchase

qboolean SV_RecursiveHullCheck2(hull_t* hull, int num, float p1f, float p2f, vec3_t p1, vec3_t p2, trace_t* trace) // woods (Qrack) #betterchase
{
	mclipnode_t* node; //johnfitz -- was dclipnode_t
	mplane_t* plane;
	float		t1, t2;
	float		frac;
	int			i;
	vec3_t		mid;
	int			side;
	float		midf;

	// LordHavoc: a goto!  everyone flee in terror... :)
loc0:
	// check for empty
	if (num < 0)
	{
		if (num != CONTENTS_SOLID)
		{
			trace->allsolid = false;
			if (num == CONTENTS_EMPTY)
				trace->inopen = true;
			else
				trace->inwater = true;
		}
		else
			trace->startsolid = true;
		return true;		// empty
	}

	// LordHavoc: this can be eliminated by validating in the loader...  but Mercury told me not to bother
	if (num < hull->firstclipnode || num > hull->lastclipnode)
		Sys_Error("SV_RecursiveHullCheck: bad node number");

	// find the point distances
	node = hull->clipnodes + num;
	plane = hull->planes + node->planenum;

	if (plane->type < 3)
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct(plane->normal, p1) - plane->dist;
		t2 = DotProduct(plane->normal, p2) - plane->dist;
	}

	// LordHavoc: recursion optimization
	if (t1 >= 0 && t2 >= 0)
	{
		num = node->children[0];
		goto loc0;
	}
	if (t1 < 0 && t2 < 0)
	{
		num = node->children[1];
		goto loc0;
	}

	// put the crosspoint DIST_EPSILON pixels on the near side
	side = (t1 < 0);
	if (side)
		frac = bound(0, (t1 + DIST_EPSILON) / (t1 - t2), 1);
	else
		frac = bound(0, (t1 - DIST_EPSILON) / (t1 - t2), 1);

	midf = p1f + (p2f - p1f) * frac;
	for (i = 0; i < 3; i++)
		mid[i] = p1[i] + frac * (p2[i] - p1[i]);

	// move up to the node
	if (!SV_RecursiveHullCheck2(hull, node->children[side], p1f, midf, p1, mid, trace))
		return false;
	/*
#ifdef PARANOID
	if (SV_HullPointContents(pm_hullmodel, mid, node->children[side]) == CONTENTS_SOLID)
	{
		Con_Printf("mid PointInHullSolid\n");
		return false;
	}
#endif
	*/

	// LordHavoc: warning to the clumsy, this recursion can not be optimized because mid would need to be duplicated on a stack
	if (SV_HullPointContents(hull, node->children[side ^ 1], mid) != CONTENTS_SOLID)
		// go past the node
		return SV_RecursiveHullCheck2(hull, node->children[side ^ 1], midf, p2f, mid, p2, trace);

	if (trace->allsolid)
		return false;		// never got out of the solid area

//==================
// the other side of the node is solid, this is the impact point
//==================
	if (!side)
	{
		VectorCopy(plane->normal, trace->plane.normal);
		trace->plane.dist = plane->dist;
	}
	else
	{
		// LordHavoc: vec3_origin is evil; the compiler can not rely on it being '0 0 0'
		// VectorSubtract (vec3_origin, plane->normal, trace->plane.normal);
		trace->plane.normal[0] = -plane->normal[0];
		trace->plane.normal[1] = -plane->normal[1];
		trace->plane.normal[2] = -plane->normal[2];
		trace->plane.dist = -plane->dist;
	}

	while (SV_HullPointContents(hull, hull->firstclipnode, mid) == CONTENTS_SOLID)
	{ // shouldn't really happen, but does occasionally
		frac -= 0.1;
		if (frac < 0)
		{
			trace->fraction = midf;
			VectorCopy(mid, trace->endpos);
			return false;
		}
		midf = p1f + (p2f - p1f) * frac;
		for (i = 0; i < 3; i++)
			mid[i] = p1[i] + frac * (p2[i] - p1[i]);
	}

	trace->fraction = midf;
	VectorCopy(mid, trace->endpos);

	return false;
}

void TraceLine2(vec3_t start, vec3_t end, vec3_t impact) // woods (Qrack) #betterchase
{
	trace_t	trace;
	qboolean result;

	memset(&trace, 0, sizeof(trace));
	trace.fraction = 1;

	//result is true if end is empty...
	result = SV_RecursiveHullCheck2(cl.worldmodel->hulls, 0, 0, 1, start, end, &trace);

	if (!result)//hit something
	{
		VectorCopy(trace.endpos, impact);
	}
	else
		VectorCopy(end, impact);
}

void LerpVector(const vec3_t from, const vec3_t to, float frac, vec3_t out) // woods (Qrack) #betterchase
{
	out[0] = from[0] + frac * (to[0] - from[0]);
	out[1] = from[1] + frac * (to[1] - from[1]);
	out[2] = from[2] + frac * (to[2] - from[2]);
}

/*
==============
TraceLine

TODO: impact on bmodels, monsters
==============
*/
void TraceLine (vec3_t start, vec3_t end, vec3_t impact)
{
	trace_t	trace;

	memset (&trace, 0, sizeof(trace));
	SV_RecursiveHullCheck (cl.worldmodel->hulls, start, end, &trace, CONTENTMASK_ANYSOLID);

	VectorCopy (trace.endpos, impact);
}

/*
==============
Chase_UpdateForClient -- johnfitz -- orient client based on camera. called after input
==============
*/
void Chase_UpdateForClient (void)
{
	//place camera

	//assign client angles to camera

	//see where camera points

	//adjust client angles to point at the same place
}

/*
==============
Chase_UpdateForDrawing -- johnfitz -- orient camera based on client. called before drawing

TODO: stay at least 8 units away from all walls in this leaf
==============
*/
void Chase_UpdateForDrawing (void)
{
	int		i;
	vec3_t	forward, up, right;
	vec3_t	ideal, crosshair, temp;

	AngleVectors (cl.lerpangles, forward, right, up); // woods added lerpangles for #smoothcam

	// calc ideal camera location before checking for walls
	for (i=0 ; i<3 ; i++)
		ideal[i] = cl.viewent.origin[i]
		- forward[i]*chase_back.value
		+ right[i]*chase_right.value;
		//+ up[i]*chase_up.value;
	ideal[2] = cl.viewent.origin[2] + chase_up.value;

	// make sure camera is not in or behind a wall
	TraceLine2(r_refdef.vieworg, ideal, temp); // woods (Qrack) #betterchase - change to 2
	if (VectorLength(temp) != 0)
	{
		VectorCopy(temp, ideal);
		LerpVector(r_refdef.vieworg, temp, 0.666f, ideal); // woods --> R00k, this prevents the camera from poking into the wall by capping the traceline.
	}

	// place camera
	VectorCopy (ideal, r_refdef.vieworg);

	// find the spot the player is looking at
	VectorMA (cl.viewent.origin, 4096, forward, temp);
	TraceLine2 (cl.viewent.origin, temp, crosshair); // woods (Qrack) #betterchase

	// calculate camera angles to look at the same spot
	VectorSubtract (crosshair, r_refdef.vieworg, temp);
	VectorAngles (temp, NULL, r_refdef.viewangles);
	if (r_refdef.viewangles[PITCH] == 90 || r_refdef.viewangles[PITCH] == -90)
		r_refdef.viewangles[YAW] = cl.viewangles[YAW];
}

