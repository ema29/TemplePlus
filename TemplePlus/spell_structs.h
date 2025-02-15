#pragma once

const uint32_t SPELL_ENUM_MAX = 802;


enum SpellStoreType : uint8_t
{
	spellStoreKnown = 1,
	spellStoreMemorized = 2,
	spellStoreCast = 3
};

struct MetaMagicData
{
	unsigned char metaMagicFlags : 4; // 1 - Maximize Spell ; 2 - Quicken Spell ; 4 - Silent Spell;  8 - Still Spell
	unsigned char metaMagicEmpowerSpellCount : 4;
	unsigned char metaMagicEnlargeSpellCount : 4;
	unsigned char metaMagicExtendSpellCount : 4;
	unsigned char metaMagicHeightenSpellCount : 4;
	unsigned char metaMagicWidenSpellCount : 4;
	operator  uint32_t()
	{
		return (*(uint32_t*)this) & 0xFFFFFF;
	}

	MetaMagicData()
	{
		metaMagicFlags = 0;
		metaMagicEmpowerSpellCount = 0;
		metaMagicEnlargeSpellCount = 0;
		metaMagicExtendSpellCount = 0;
		metaMagicHeightenSpellCount = 0;
		metaMagicWidenSpellCount = 0;

	}

	MetaMagicData(unsigned int raw)
	{
		metaMagicFlags = raw & 0xF;
		metaMagicEmpowerSpellCount =  (raw & 0xF0) >> 4;
		metaMagicEnlargeSpellCount =  (raw & 0xF00) >> 8;
		metaMagicExtendSpellCount =   (raw & 0xF000) >> 12;
		metaMagicHeightenSpellCount = (raw & 0xF0000) >> 16;
		metaMagicWidenSpellCount =    (raw & 0xF00000) >> 20;
	}

	
};
const uint32_t TestSizeOfMetaMagicData = sizeof(MetaMagicData);

struct SpellStoreState
{
	SpellStoreType spellStoreType;
	uint8_t usedUp; // relevant only for spellStoreMemorized
};

enum SpontCastType : unsigned char {
	spontCastGoodCleric = 2,
	spontCastEvilCleric = 4,
	spontCastDruid = 8
};

struct D20SpellData
{
	uint16_t spellEnumOrg;
	MetaMagicData metaMagicData;
	uint8_t spellClassCode;
	uint8_t itemSpellData;
	SpontCastType spontCastType : 4;
	unsigned char spellSlotLevel : 4;
};

const uint32_t TestSizeOfD20SpellData = sizeof(D20SpellData);

#pragma pack(push,4)
struct SpellStoreData
{
	uint32_t spellEnum;
	uint32_t classCode;
	uint32_t spellLevel;
	SpellStoreState spellStoreState;
	MetaMagicData metaMagicData; // should be stored as 32bit value!
	char pad0;
	uint32_t pad1;
	uint32_t pad2;
	uint32_t pad3;
};
const auto TestSizeOfSpellStoreData = sizeof(SpellStoreData);
#pragma pack(pop)