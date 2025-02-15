#include "stdafx.h"
#include "util/fixes.h"
#include "tio/tio_utils.h"
#include "gamesystems/gamesystems.h"
#include "ui/ui.h"
#include "timeevents.h"
#include "party.h"
#include "maps.h"
#include "critter.h"
#include "python/python_integration_obj.h"
#include <tio/tio.h>
#include "gamesystems/legacy.h"

struct GsiData {
	string filename;
	string displayName;
	string moduleName;
	int leaderPortrait;
	int leaderLevel;
	locXY leaderLoc;
	GameTime time;
	string leaderName;
	int mapNumber;
	int storyState;
};

static struct GameLibSaveAddresses : temple::AddressTable {

	int* isInSave;

	// Enables ironman mode
	int* isIronman;
	int* ironmanSaveNumber;
	char **ironmanSaveName;

	void(__cdecl *UiMmRelated)(int arg);
	void(__cdecl *UiMmRelated2)(int arg);

	bool (__cdecl *PackArchive)(const char *filename, const char *srcPath);

	bool (__cdecl *UiSaveGame)();
	
	GameLibSaveAddresses() {
		rebase(isInSave, 0x103072D0);
		rebase(UiMmRelated, 0x10111A00);
		rebase(UiMmRelated2, 0x10111A40);
		rebase(PackArchive, 0x101E5F80);
		rebase(isIronman, 0x103072B8);
		rebase(ironmanSaveNumber, 0x10306F44);
		rebase(ironmanSaveName, 0x103072C0);
		rebase(UiSaveGame, 0x101152F0);
	}
} addresses;

struct InSaveGame {
	InSaveGame() {
		*addresses.isInSave = true;
	}
	~InSaveGame() {
		*addresses.isInSave = false;
	}
};

static GsiData GatherGsiData(const string &filename, const string &displayName) {
	GsiData result;
	result.filename = filename;
	result.displayName = displayName;
	result.moduleName = "ToEE";
	
	auto leader = party.GetLeader();
	result.leaderName = objects.GetDisplayName(leader, leader);
	result.mapNumber = maps.GetCurrentMapId();
	result.leaderPortrait = critterSys.GetPortraitId(leader);
	result.leaderLevel = critterSys.GetLevel(leader);
	result.leaderLoc = objects.GetLocation(leader);
	result.storyState = party.GetStoryState();

	return result;
}

static bool SaveGsiData(const GsiData &saveInfo) {

	// Remove existing GSI for the slots
	string globPattern = format("save\\{}*.gsi", saveInfo.filename);
	TioFileList list;
	tio_filelist_create(&list, globPattern.c_str());
	for (int i = 0; i < list.count; ++i) {
		auto filename = format("save\\{}", list.files[i].name);		
		tio_remove(filename.c_str());
	}
	tio_filelist_destroy(&list);

	auto filename = format("save\\{}{}.gsi", saveInfo.filename, saveInfo.displayName);
	auto file = tio_fopen(filename.c_str(), "wb");	
	if (!file) {
		logger->error("Unable to open {} for writing.", filename);
		return false;
	}

	int version = 0;
	if (tio_fwrite(&version, 4, 1, file) != 1) {
		logger->error("Unable to write version info for GSI.");
		tio_fclose(file);
		return false;
	}

	bool success = true;

	int stringLength = saveInfo.moduleName.length() + 1;
	success &= tio_fwrite(&stringLength, 4, 1, file) == 1;
	success &= tio_fwrite(saveInfo.moduleName.c_str(), stringLength, 1, file) == 1;

	stringLength = saveInfo.leaderName.length() + 1;
	success &= tio_fwrite(&stringLength, 4, 1, file) == 1;
	success &= tio_fwrite(saveInfo.leaderName.c_str(), stringLength, 1, file) == 1;

	success &= tio_fwrite(&saveInfo.mapNumber, 4, 1, file) == 1;
	success &= tio_fwrite(&saveInfo.time, 8, 1, file) == 1;
	success &= tio_fwrite(&saveInfo.leaderPortrait, 4, 1, file) == 1;
	success &= tio_fwrite(&saveInfo.leaderLevel, 4, 1, file) == 1;
	success &= tio_fwrite(&saveInfo.leaderLoc, 8, 1, file) == 1;
	success &= tio_fwrite(&saveInfo.storyState, 4, 1, file) == 1;

	stringLength = saveInfo.displayName.length() + 1;
	success &= tio_fwrite(&stringLength, 4, 1, file) == 1;
	success &= tio_fwrite(saveInfo.displayName.c_str(), stringLength, 1, file) == 1;

	tio_fclose(file);
	return success;
}

bool GameSystems::SaveGame(const string& filename, const string& displayName) {

	InSaveGame inSave;
	addresses.UiMmRelated(63);

	if (!TioDirExists("Save\\Current")) {
		return false;
	}

	auto fullPath = "Save\\Current\\data.sav"; // Full path
	auto file = tio_fopen(fullPath, "wb");

	if (!file) {
		logger->error("Error creating: Save\\Current\\data.sav");
		return false;
	}

	int saveVersion = 0;
	if (tio_fwrite(&saveVersion, 4, 1, file) != 1) {
		logger->error("Failed to write save version.");
		tio_fclose(file);
		tio_remove(fullPath);
		return false;
	}

	if (tio_fwrite(addresses.isIronman, 4, 1, file) != 1) {
		logger->error("Failed to write ironman mode flag.");
		tio_fclose(file);
		tio_remove(fullPath);
		return false;
	}

	if (*addresses.isIronman) {
		if (tio_fwrite(addresses.ironmanSaveNumber, 4, 1, file) != 1) {
			logger->error("Failed to write ironman save number");
			tio_fclose(file);
			tio_remove(fullPath);
			return false;
		}

		auto savenameLen = strlen(*addresses.ironmanSaveName) + 1;
		if (tio_fwrite(&savenameLen, 4, 1, file) != 1) {
			logger->error("Failed to write ironman save name length");
			tio_fclose(file);
			tio_remove(fullPath);
			return false;
		}
		
		if (tio_fwrite(*addresses.ironmanSaveName, 1, savenameLen, file) != savenameLen) {
			logger->error("Failed to write ironman save name");
			tio_fclose(file);
			tio_remove(fullPath);
			return false;
		}
	}

	uint64_t startOfData;
	tio_fgetpos(file, &startOfData);

	for (int i = 0; i < gameSystemCount; ++i) {
		addresses.UiMmRelated2(i + 1);
		auto &system = legacyGameSystems->systems[i];

		if (!system.save) {
			continue;
		}

		if (!system.save(file)) {
			logger->error("Save function for game system {} failed.", system.name);
			tio_fclose(file);
			tio_remove(fullPath);
			return false;
		}

		uint32_t sentinel = 0xBEEFCAFE;
		if (tio_fwrite(&sentinel, 4, 1, file) != 1) {
			tio_fclose(file);
			tio_remove(fullPath);
			return false;
		}

		uint64_t endOfData;
		tio_fgetpos(file, &endOfData);
		logger->info(" wrote to: {}, Total: ({})", endOfData, endOfData - startOfData);
		startOfData = endOfData;
	}

	tio_fclose(file);

	if (!addresses.UiSaveGame()) {
		tio_remove(fullPath);
		return false;
	}

	logger->info("UI savegame created");

	GsiData gsiData = GatherGsiData(filename, displayName);
	if (!SaveGsiData(gsiData)) {
		logger->error("Unable to save GSI file.");
		return false;
	}

	// Create savegame archive
	auto archiveName = format("save\\{}", filename);
	if (!addresses.PackArchive(archiveName.c_str(), "Save\\Current")) {
		logger->error("Unable to compress game save archive.");
		return false;
	}

	addresses.UiMmRelated2(63);

	logger->info("Game save created successfully.");

	// Rename screenshots
	auto screenDestName = format("save\\{}l.jpg", filename);
	if (tio_fileexists(screenDestName.c_str())) {
		tio_remove(screenDestName.c_str());
	}
	tio_rename("save\\templ.jpg", screenDestName.c_str());

	screenDestName = format("save\\{}s.jpg", filename);
	if (tio_fileexists(screenDestName.c_str())) {
		tio_remove(screenDestName.c_str());
	}
	tio_rename("save\\temps.jpg", screenDestName.c_str());

	// co8 savehook
	auto saveHookArgs = Py_BuildValue("(s)", filename.c_str());
	pythonObjIntegration.ExecuteScript("templeplus.savehook", "save", saveHookArgs);
	Py_DECREF(saveHookArgs);

	return true;
}

static class GameLibSaveReplacement : TempleFix {
public:
	const char* name() override {
		return "GameLib Save replacement.";
	}

	static bool SaveGame(const char *filename, const char *displayName) {
		return gameSystems->SaveGame(filename, displayName);
	}

	void apply() override {
		replaceFunction(0x100042C0, SaveGame);
	}
} fix;
