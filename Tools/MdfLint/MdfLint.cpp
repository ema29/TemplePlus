#include <iostream>
#include <fstream>
#include <sstream>

#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#include <infrastructure/mdfparser.h>
#include <infrastructure/stringutil.h>

struct LintStats {
	int processed = 0;
	int clipper = 0;
	int textured = 0;
	int general = 0;
};

static int LintFile(const std::wstring& filename, LintStats& stats);
static void LintFilesIn(const std::wstring& directory, LintStats& stats);

int wmain(int argc, wchar_t* argv[]) {

	std::ios_base::sync_with_stdio(false);
	LintStats stats;

	if (argc != 2) {
		std::cerr << "Usage: mdflint <filename.mdf|directory>" << std::endl;
		return -1;
	}

	auto input = argv[1];

	std::cout << "Processing " << ucs2_to_local(input) << std::endl;

	if (PathIsDirectory(input)) {
		// Traverse directory
		LintFilesIn(input, stats);
		std::cout << "Done. (Processed " << stats.processed << " files)" << std::endl;

		std::cout << "Textured: " << stats.textured
			<< ", General: " << stats.general
			<< ", Clipper: " << stats.clipper << std::endl;

	} else {
		// Lint a single file
		if (!LintFile(input, stats)) {
			return -1;
		}
	}

	return 0;
}

static int LintFile(const std::wstring& filename, LintStats& stats) {

	stats.processed++;

	// Read the file into memory
	std::ifstream in(filename);

	if (!in) {
		std::cerr << "Unable to open " << ucs2_to_local(filename)
			<< std::endl;
		return false;
	}

	// Slurp the file into the string buffer
	std::stringstream sstr;
	sstr << in.rdbuf();

	in.close();

	// Usually the files here come from the tio system 
	// which only supports ascii anyway
	auto localFilename(ucs2_to_utf8(filename));
	gfx::MdfParser parser(localFilename, sstr.str());
	parser.SetStrict(true);

	try {
		auto result = parser.Parse();
		if (!result) {
			return false;
		}
		switch (result->GetType()) {
		case gfx::MdfType::Textured:
			stats.textured++;
			break;
		case gfx::MdfType::General:
			stats.general++;
			break;
		case gfx::MdfType::Clipper:
			stats.clipper++;
			break;
		default:
			break;
		}
		return true;
	} catch (TempleException& e) {
		std::cerr << e.what() << std::endl;
		return false;
	}

}

static void IterateDirectory(const wchar_t* globPattern,
                             std::function<void(const WIN32_FIND_DATA&)> callback) {

	WIN32_FIND_DATA findData;
	auto handle = FindFirstFile(globPattern, &findData);
	if (handle != INVALID_HANDLE_VALUE) {
		do {
			if (!wcscmp(findData.cFileName, L".") || !wcscmp(findData.cFileName, L"..")) {
				continue;
			}

			callback(findData);
		} while (FindNextFile(handle, &findData));

		FindClose(handle);
	}

}

void LintFilesIn(const std::wstring& directory, LintStats& stats) {
	wchar_t searchPath[MAX_PATH];


	// Search for files to process
	PathCombine(searchPath, directory.c_str(), L"*.mdf");

	IterateDirectory(searchPath, [&](const auto& findData) {
		                 if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			                 wchar_t path[MAX_PATH];
			                 PathCombine(path, directory.c_str(), findData.cFileName);
			                 LintFile(path, stats);
		                 }
	                 });

	// Search for subdirectories to process now
	// Build <dir>\* in searchPath
	PathCombine(searchPath, directory.c_str(), L"*");
	IterateDirectory(searchPath, [&](const auto& findData) {
		                 if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			                 wchar_t subdir[MAX_PATH];
			                 PathCombine(subdir, directory.c_str(), findData.cFileName);
			                 LintFilesIn(subdir, stats);
		                 }
	                 });

}
