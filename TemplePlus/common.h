#pragma once

#include <d3dx9math.h>

#include "obj_structs.h"
#include "d20_defs.h"
#include "temple_enums.h"

#define macAsmProl {\
	__asm push ecx\
	__asm push esi\
	__asm push ebx\
	__asm push edi};

#define macAsmEpil {\
	__asm pop edi\
	__asm pop ebx\
	__asm pop esi\
	__asm pop ecx}\

#define macRebase(funName , b) rebase(funName, 0x##b); 
#define macReplaceFun(b, funName) replaceFunction(0x##b, funName); 
#define macAsmThis(funName) {\
	__asm mov ecx, this\
	__asm mov esi, [ecx]._##funName }

#define macTempleFix(sysName) public:\
	const char* name() override {\
			return #sysName "Function Replacements";}\
			void apply() override \

#define BonusListMax 40

// This is the number of pixels per tile on the x and y axis. Derived from sqrt(800)
#define INCH_PER_TILE 28.284271247461900976033774484194f
#define INCH_PER_HALFTILE (INCH_PER_TILE/2.0f)
#define INCH_PER_FEET 12

// this is the number of tiles per sector in each direction (so the total is this squared i.e. 4096 in toee)
#define SECTOR_SIDE_SIZE 64

# pragma region Standard Structs

#pragma pack(push, 1)

typedef D3DXVECTOR2 vector2f;
typedef D3DXVECTOR3 vector3f;

struct locXY {
	uint32_t locx;
	uint32_t locy;

	static locXY fromField(uint64_t location) {
		return *(locXY*)&location;
	}

	uint64_t ToField() {
		return *((uint64_t*)this);
	}

	operator uint64_t() const {
		return *(uint64_t*)this;
	}

	vector2f ToInches2D(float offsetX = 0, float offsetY = 0) {
		return{
			locx * INCH_PER_TILE + offsetX,
			locy * INCH_PER_TILE + offsetY
		};
	}

	vector3f ToInches3D(float offsetX = 0, float offsetY = 0, float offsetZ = 0) {
		return {
			locx * INCH_PER_TILE + offsetX,
			offsetZ,
			locy * INCH_PER_TILE + offsetY
		};
	}
};

struct SectorLoc
{
	uint64_t raw;

	uint64_t x()
	{
		return raw & 0x3ffFFFF;
	}

	uint64_t y()
	{
		return raw >> 26 ;
	}

	uint64_t ToField() {
		return this->raw;
	}

	operator uint64_t() const {
			return this->raw;
		}

	operator int64_t() const {
		return (int64_t)this->raw;
	}

	void GetFromLoc(locXY loc)
	{
		raw = loc.locx / SECTOR_SIDE_SIZE
			 + ( (loc.locy / SECTOR_SIDE_SIZE) << 26 );
	}

	SectorLoc()
	{
		raw = 0;
	}

	SectorLoc(locXY loc)
	{
		raw = loc.locx / SECTOR_SIDE_SIZE
			+ ((loc.locy / SECTOR_SIDE_SIZE) << 26);
	}

	locXY GetBaseTile()
	{
		locXY loc;
		loc.locx = (int) x() * SECTOR_SIDE_SIZE;
		loc.locy = (int) y() * SECTOR_SIDE_SIZE;
		return loc;
	}

	bool operator ==(SectorLoc secLoc) {
		return raw == secLoc.raw;
	}

};

struct Subtile // every tile is subdivided into 3x3 subtiles
{
	int32_t x;
	int32_t y;

	static Subtile fromField(int64_t location) {
		return *(Subtile*)&location;
	}

	int64_t ToField() {
		return *((int64_t*)this);
	}

	operator int64_t() const {
		return *(int64_t*)this;
	}

	operator Subtile() const
	{
		return *(Subtile*)this;
	}
};

struct TileRect
{
	int64_t x1;
	int64_t y1;
	int64_t x2;
	int64_t y2;
};

struct LocAndOffsets {
	locXY location;
	float off_x;
	float off_y;
	
	vector2f ToInches2D() {
		return location.ToInches2D(off_x, off_y);
	}

	vector3f ToInches3D(float offsetZ = 0) {
		return location.ToInches3D(off_x, off_y, offsetZ);
	}

	void FromAbsolute(double overallOffX, double overallOffY)
	{
		location.locx = static_cast<uint32_t>(overallOffX / INCH_PER_TILE);
		location.locy = static_cast<uint32_t>(overallOffY / INCH_PER_TILE);
		off_x = static_cast<float>(overallOffX - (static_cast<double>(location.locx) + 0.5) * INCH_PER_TILE);
		off_y = static_cast<float>(overallOffY - (static_cast<double>(location.locy) + 0.5) * INCH_PER_TILE);
	};
};

inline std::ostream& operator<<(std::ostream& os, const LocAndOffsets & loc) {

	return os
		<< std::to_string(loc.location.locx)
		+ "," + std::to_string(loc.location.locy)
		+ "," + std::to_string(loc.off_x)
		+ "," + std::to_string(loc.off_y);
}

struct LocFull {
	LocAndOffsets location;
	float off_z;
};

struct GroupArray {
	objHndl GroupMembers[32];
	uint32_t GroupSize;
	int (__cdecl*sortFunc)(void*, void*); // used for comparing two items (e.g. alphabetic sorting)
};

struct JumpPointPacket {
	int jmpPntId;
	char * pMapName;
	_mapNum mapNum;
	uint32_t field_C;
	locXY location;
};



#pragma pack(pop)


struct BonusEntry
{
	int32_t bonValue;
	uint32_t bonType; // gets comapred with 0, 8 and 21 in 100E6490; I think these types allow bonuses to stack
	char * bonusMesString; // parsable string for the help system e.g. "~Item~[TAG_ITEM]"
	char * bonusDescr; // e.g. "Magic Full Plate +1"
};

struct BonusCap
{
	int capValue;
	int bonType;
	char *bonCapperString;
	char * bonCapDescr;
};

struct BonusList
{
	BonusEntry bonusEntries[40];
	uint32_t bonCount;
	BonusCap bonCaps[10];
	uint32_t bonCapperCount;
	uint32_t zeroBonusReasonMesLine[10]; // a line from the bonus.mes that is auto assigned a 0 value (I think it will print ---). Probably for overrides like racial immunity and stuff.
	uint32_t zeroBonusCount;
	int32_t overallCapHigh; // init to largest  positive int; controlls what the sum of all the modifiers of various types cannot exceed
	uint32_t field358; // looks unused
	char * overallCapHighBonusMesString;
	char * overallCapHighDescr;
	int32_t overallCapLow; //init to most negative int
	uint32_t field368; // looks unused
	char * overallCapLowBonusMesString;
	char * overallCapLowDescr;
	uint32_t bonFlags; // init 0; 0x1 - overallCapHigh set; 0x2 - overallCapLow set; 0x4 - force cap override (otherwise it can only impose restrictions i.e. it will only change the cap if it's lower than the current one)

	BonusList()
	{
		this->bonCount = 0;
		this->bonCapperCount = 0;
		this->zeroBonusCount = 0;
		this->bonFlags = 0;
		this->overallCapHigh = 0x7fffFFFF;
		this->overallCapLow = 0x80000001;
	}
};

const int TestSizeOfBonusList = sizeof(BonusList); // should be 888 (0x378)


struct AttackPacket
{
	objHndl attacker;
	objHndl victim;
	D20ActionType d20ActnType;
	int dispKey;
	D20CAF flags;
	int field_1C;
	objHndl weaponUsed;
	objHndl ammoItem;
};

enum ScreenDirections : char {
	Top = 0,
	TopRight = 1,
	Right,
	BottomRight,
	Bottom,
	BottomLeft,
	Left,
	TopLeft,
	DirectionsNum // 8
};


inline std::ostream& operator<<(std::ostream& os, const ScreenDirections & direc) {
	switch(direc)
	{
	case ScreenDirections::Top:
		return os << "U";
	case ScreenDirections::TopRight:
		return os << "UR";
	case ScreenDirections::Right:
		return os << "R";
	case ScreenDirections::BottomRight:
		return os << "DR";
	case ScreenDirections::Bottom:
		return os << "D";
	case ScreenDirections::BottomLeft :
		return os << "DL";
	case ScreenDirections::Left:
		return os << "L";
	case ScreenDirections::TopLeft:
		return os << "UL";
	default:
		return os << "NA";
	}
}


struct PointNode
{
	float absX;
	int initTo0;
	float absY;
};
#pragma endregion