#pragma once

#include "common.h"
#include <temple/dll.h>

const float tileToOffset = 12.0;
#define LOCATION_OFFSET_TOL 0.001 // when comparing offsets, differences smaller than this will be considered as ignorable computational error

struct LocationSys : temple::AddressTable
{
	int64_t * locTransX;
	int64_t*locTransY;
	float distBtwnLocAndOffs(LocAndOffsets, LocAndOffsets);
	void RegularizeLoc(LocAndOffsets* toLocTweaked); //  alters the location and offsets so that the offsets are within the tile
	void GetOverallOffset(LocAndOffsets loc, float* absX, float* absY);
	BOOL ShiftLocationByOneSubtile(LocAndOffsets* loc, ScreenDirections direction, LocAndOffsets* locOut);
	void (__cdecl*PointNodeInit)(LocAndOffsets* loc, PointNode* pntNode);
	int (__cdecl*GetLocFromScreenLocPrecise)(int64_t x, int64_t y, locXY* location, float* offX, float* offY);
	void(__cdecl * getLocAndOff)(objHndl objHnd, LocAndOffsets * locAndOff);
	void(__cdecl* SubtileToLocAndOff)(int64_t subtile, LocAndOffsets* locFromSubtile);
	int64_t(__cdecl * subtileFromLoc)(LocAndOffsets * loc);
	BOOL(__cdecl* ShiftSubtileOnceByDirection)(Subtile subtile, int direction, Subtile * shiftedTile);
	void(__cdecl * TOEEdistBtwnLocAndOffs)(LocAndOffsets, LocAndOffsets); // outputs to the FPU (st0);is basically  sqrt(dx^2+dy^2) where a tile is sqrt(800)xsqrt(800);  I think it's in Inches because some functions divide this result by 12 (inches->feet)
	float (__cdecl * Distance3d)(LocAndOffsets loc1, LocAndOffsets loc2); // is basically  sqrt(dx^2+dy^2)*INCH_PER_TILE  [inch]
	float intToFloat(int32_t x);

	// Distance between two objects in feet
	float (__cdecl *DistanceToObj)(objHndl from, objHndl to);

	// Distance between from and loc in inches (without the obj radius)
	float DistanceToLoc(objHndl from, LocAndOffsets loc);
	float DistanceToLocFeet(objHndl obj, LocAndOffsets *loc);

	float InchesToFeet(float inches);

	LocationSys();
};

/*
	Calculates the angle in radians between two points in the tile coordinate system.
	The angle can be used to make an object that is at fromPoint face the location at toPoint,
	given that rotation 0 means "look directory north".
*/
float AngleBetweenPoints(LocAndOffsets fromPoint, LocAndOffsets toPoint);

bool operator!=(const LocAndOffsets& to, const LocAndOffsets& rhs);
bool operator==(const LocAndOffsets& to, const LocAndOffsets& rhs);

extern LocationSys locSys;

