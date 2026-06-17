#pragma once

#include "json.hpp"
#include <string>
#include <vector>
#include <cstdint>

// One player's transform + shape state at a single recorded moment in time.
struct GhostPlayerFrame {
	float x = 0.f;
	float y = 0.f;
	float rotation = 0.f;
	float scaleX = 1.f;
	float scaleY = 1.f;
	uint8_t mode = 0;     // bitmask, see GhostMode below
	bool visible = true;  // false while this player slot isn't active (e.g. before dual mode starts)
};

// Bitmask values for GhostPlayerFrame::mode
namespace GhostMode {
	constexpr uint8_t Ship = 1 << 0;
	constexpr uint8_t Ball = 1 << 1;
	constexpr uint8_t Bird = 1 << 2;
	constexpr uint8_t Dart = 1 << 3;
	constexpr uint8_t Robot = 1 << 4;
	constexpr uint8_t Spider = 1 << 5;
	constexpr uint8_t Swing = 1 << 6;
}

// A single recorded simulation step. Holds both players' frames (player 2 is
// only meaningfully "visible" while dual mode is active during that step).
struct GhostFrame {
	float dt = 0.f;
	GhostPlayerFrame p1;
	GhostPlayerFrame p2;
};

// A full recorded attempt, plus the bookkeeping we persist alongside it.
struct GhostRun {
	std::string levelName;
	float bestPercent = 0.f;
	bool enabledForLevel = true; // per-level on/off toggle
	std::vector<GhostFrame> frames;
};

inline nlohmann::json playerFrameToJson(const GhostPlayerFrame& f) {
	return nlohmann::json::array({ f.x, f.y, f.rotation, f.scaleX, f.scaleY, (int)f.mode, f.visible ? 1 : 0 });
}

inline GhostPlayerFrame playerFrameFromJson(const nlohmann::json& j) {
	GhostPlayerFrame f;
	if (j.is_array() && j.size() >= 7) {
		f.x = j[0].get<float>();
		f.y = j[1].get<float>();
		f.rotation = j[2].get<float>();
		f.scaleX = j[3].get<float>();
		f.scaleY = j[4].get<float>();
		f.mode = (uint8_t)j[5].get<int>();
		f.visible = j[6].get<int>() != 0;
	}
	return f;
}

inline nlohmann::json ghostRunToJson(const GhostRun& run) {
	nlohmann::json j;
	j["levelName"] = run.levelName;
	j["bestPercent"] = run.bestPercent;
	j["enabledForLevel"] = run.enabledForLevel;

	auto framesArr = nlohmann::json::array();
	for (auto& f : run.frames) {
		framesArr.push_back(nlohmann::json::array({ f.dt, playerFrameToJson(f.p1), playerFrameToJson(f.p2) }));
	}
	j["frames"] = framesArr;

	return j;
}

inline GhostRun ghostRunFromJson(const nlohmann::json& j) {
	GhostRun run;
	if (j.contains("levelName")) run.levelName = j["levelName"].get<std::string>();
	if (j.contains("bestPercent")) run.bestPercent = j["bestPercent"].get<float>();
	if (j.contains("enabledForLevel")) run.enabledForLevel = j["enabledForLevel"].get<bool>();

	if (j.contains("frames") && j["frames"].is_array()) {
		for (auto& fj : j["frames"]) {
			if (!fj.is_array() || fj.size() < 3) continue;
			GhostFrame f;
			f.dt = fj[0].get<float>();
			f.p1 = playerFrameFromJson(fj[1]);
			f.p2 = playerFrameFromJson(fj[2]);
			run.frames.push_back(f);
		}
	}

	return run;
}
