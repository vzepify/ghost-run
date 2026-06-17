#pragma once

#include <Geode/Geode.hpp>
#include "GhostData.hpp"
#include <filesystem>
#include <fstream>

using namespace geode::prelude;

// Figures out a stable, unique key for a level so we can save one ghost file
// per level. Published levels have a real m_levelID; local/unsaved levels in
// the editor report 0, so we fall back to name + revision + song as a
// reasonably unique fingerprint for those.
inline std::string ghostLevelKey(GJGameLevel* level) {
	if (!level) return "unknown";

	int id = level->m_levelID.value();
	if (id != 0) {
		return std::to_string(id);
	}

	std::string name = level->m_levelName;
	for (auto& c : name) {
		if (!isalnum((unsigned char)c)) c = '_';
	}

	return "local_" + name + "_" + std::to_string(level->m_levelRev) + "_" + std::to_string(level->m_songID);
}

inline std::filesystem::path ghostFilePath(GJGameLevel* level) {
	auto dir = Mod::get()->getSaveDir() / "ghosts";
	std::filesystem::create_directories(dir);
	return dir / ("ghost_" + ghostLevelKey(level) + ".json");
}

inline bool ghostRunExists(GJGameLevel* level) {
	return std::filesystem::exists(ghostFilePath(level));
}

inline GhostRun loadGhostRun(GJGameLevel* level) {
	auto path = ghostFilePath(level);

	if (!std::filesystem::exists(path)) {
		GhostRun empty;
		if (level) empty.levelName = level->m_levelName;
		return empty;
	}

	try {
		std::ifstream in(path);
		nlohmann::json j;
		in >> j;
		return ghostRunFromJson(j);
	} catch (...) {
		log::warn("Best Run Ghost: failed to parse saved ghost for level, starting fresh.");
		GhostRun empty;
		if (level) empty.levelName = level->m_levelName;
		return empty;
	}
}

inline void saveGhostRun(GJGameLevel* level, const GhostRun& run) {
	auto path = ghostFilePath(level);

	try {
		std::ofstream out(path);
		out << ghostRunToJson(run).dump();
	} catch (...) {
		log::warn("Best Run Ghost: failed to save ghost data to disk.");
	}
}

// Only flips the enabled flag and rewrites the file; used by the popup so
// toggling on/off doesn't require touching the (potentially large) frame data
// in memory.
inline void setGhostEnabledForLevel(GJGameLevel* level, bool enabled) {
	auto run = loadGhostRun(level);
	run.enabledForLevel = enabled;
	if (level) run.levelName = level->m_levelName;
	saveGhostRun(level, run);
}

inline bool isGhostEnabledForLevel(GJGameLevel* level) {
	if (!ghostRunExists(level)) return true; // default on for levels with no data yet
	return loadGhostRun(level).enabledForLevel;
}

inline void deleteGhostRun(GJGameLevel* level) {
	auto path = ghostFilePath(level);
	std::error_code ec;
	std::filesystem::remove(path, ec);
}
