#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/binding/GameManager.hpp>
#include <algorithm>

#include "GhostData.hpp"
#include "GhostStorage.hpp"
#include "GhostLive.hpp"

using namespace geode::prelude;

class XPlayLayer;

static XPlayLayer* g_activeLayer = nullptr;

static uint8_t modeBitmaskOf(PlayerObject* p) {
	if (!p) return 0;
	if (p->m_isShip) return GhostMode::Ship;
	if (p->m_isBird) return GhostMode::Bird;
	if (p->m_isBall) return GhostMode::Ball;
	if (p->m_isDart) return GhostMode::Dart;
	if (p->m_isRobot) return GhostMode::Robot;
	if (p->m_isSpider) return GhostMode::Spider;
	if (p->m_isSwing) return GhostMode::Swing;
	return 0;
}

// Switches a ghost's shape to match a recorded mode bitmask, properly
// reverting out of whatever mode it was previously in first (mirrors how
// passing through a real mode portal works).
static void applyGhostMode(PlayerObject* ghost, uint8_t fromMode, uint8_t toMode) {
	if (!ghost || fromMode == toMode) return;

	switch (fromMode) {
		case GhostMode::Ship: ghost->toggleFlyMode(false, false); break;
		case GhostMode::Bird: ghost->toggleBirdMode(false, false); break;
		case GhostMode::Ball: ghost->toggleRollMode(false, false); break;
		case GhostMode::Dart: ghost->toggleDartMode(false, false); break;
		case GhostMode::Robot: ghost->toggleRobotMode(false, false); break;
		case GhostMode::Spider: ghost->toggleSpiderMode(false, false); break;
		case GhostMode::Swing: ghost->toggleSwingMode(false, false); break;
		default: break;
	}

	switch (toMode) {
		case GhostMode::Ship: ghost->toggleFlyMode(true, false); break;
		case GhostMode::Bird: ghost->toggleBirdMode(true, false); break;
		case GhostMode::Ball: ghost->toggleRollMode(true, false); break;
		case GhostMode::Dart: ghost->toggleDartMode(true, false); break;
		case GhostMode::Robot: ghost->toggleRobotMode(true, false); break;
		case GhostMode::Spider: ghost->toggleSpiderMode(true, false); break;
		case GhostMode::Swing: ghost->toggleSwingMode(true, false); break;
		default: break;
	}
}

static float lerpf(float a, float b, float t) {
	return a + (b - a) * t;
}

class $modify(XPlayLayer, PlayLayer) {
	struct Fields {
		// Recording (this attempt)
		std::vector<GhostFrame> m_recording;
		float m_attemptBestPercent = 0.f;
		bool m_started = false;

		// Playback (the saved best run we're racing against)
		GhostRun m_playbackRun;
		std::vector<float> m_playbackCumulative;
		float m_playbackElapsed = 0.f;
		size_t m_playbackIndex = 0;
		uint8_t m_ghost1Mode = 0;
		uint8_t m_ghost2Mode = 0;

		std::vector<PlayerObject*> m_ghosts; // [0] = ghost for player1, [1] = ghost for player2

		bool m_masterEnabled = true;
		bool m_levelEnabled = true;
		float m_opacity = 0.4f;
		bool m_tint = true;
		bool m_recordInPractice = false;
	};

	void recomputeCumulative() {
		m_fields->m_playbackCumulative.clear();
		m_fields->m_playbackCumulative.reserve(m_fields->m_playbackRun.frames.size());
		float acc = 0.f;
		for (auto& f : m_fields->m_playbackRun.frames) {
			acc += f.dt;
			m_fields->m_playbackCumulative.push_back(acc);
		}
		m_fields->m_playbackElapsed = 0.f;
		m_fields->m_playbackIndex = 0;
		m_fields->m_ghost1Mode = 0;
		m_fields->m_ghost2Mode = 0;
	}

	void loadSettingsAndState() {
		m_fields->m_masterEnabled = Mod::get()->getSettingValue<bool>("ghosts-enabled");
		m_fields->m_opacity = Mod::get()->getSettingValue<double>("ghost-opacity");
		m_fields->m_tint = Mod::get()->getSettingValue<bool>("tint-ghost");
		m_fields->m_recordInPractice = Mod::get()->getSettingValue<bool>("record-in-practice");
		m_fields->m_levelEnabled = isGhostEnabledForLevel(m_level);
	}

	void setupGhostPlayers() {
		if (!m_fields->m_ghosts.empty()) return;
		if (!m_objectLayer) return;

		auto gm = GameManager::sharedState();

		for (int i = 0; i < 2; i++) {
			auto ghost = PlayerObject::create(
				gm->getPlayerFrame(),
				gm->getPlayerShip(),
				typeinfo_cast<GJBaseGameLayer*>(this),
				typeinfo_cast<CCLayer*>(this),
				true // no physics: purely cosmetic, never collides or dies
			);
			if (!ghost) continue;

			ghost->setPosition(m_player1->getPosition());
			ghost->setOpacity(0);
			ghost->setID(("best-run-ghost-" + std::to_string(i)).c_str());

			if (m_fields->m_tint) {
				ghost->setColor({150, 200, 255});
				ghost->setSecondColor({200, 230, 255});
			}

			m_objectLayer->addChild(ghost, 50);
			m_fields->m_ghosts.push_back(ghost);
		}
	}

	void refreshGhostVisibility() {
		bool show = m_fields->m_masterEnabled && m_fields->m_levelEnabled
			&& !(m_isPracticeMode && !m_fields->m_recordInPractice)
			&& !m_fields->m_playbackRun.frames.empty();

		unsigned char op = show ? (unsigned char)(255.f * m_fields->m_opacity) : 0;
		for (auto* g : m_fields->m_ghosts) {
			if (g) g->setOpacity(op);
		}
	}

	bool shouldRecordNow() {
		if (!m_fields->m_masterEnabled) return false;
		if (m_isPracticeMode && !m_fields->m_recordInPractice) return false;
		return true;
	}

	void recordFrame(float dt) {
		GhostFrame f;
		f.dt = dt;

		f.p1.x = m_player1->getPositionX();
		f.p1.y = m_player1->getPositionY();
		f.p1.rotation = m_player1->getRotation();
		f.p1.scaleX = m_player1->getScaleX();
		f.p1.scaleY = m_player1->getScaleY();
		f.p1.mode = modeBitmaskOf(m_player1);
		f.p1.visible = m_player1->isVisible();

		if (m_player2) {
			f.p2.x = m_player2->getPositionX();
			f.p2.y = m_player2->getPositionY();
			f.p2.rotation = m_player2->getRotation();
			f.p2.scaleX = m_player2->getScaleX();
			f.p2.scaleY = m_player2->getScaleY();
			f.p2.mode = modeBitmaskOf(m_player2);
			f.p2.visible = m_player2->isVisible();
		} else {
			f.p2.visible = false;
		}

		m_fields->m_recording.push_back(f);
	}

	void applyFrameToGhost(PlayerObject* ghost, uint8_t& appliedMode, const GhostPlayerFrame& a, const GhostPlayerFrame& b, float alpha, bool show) {
		if (!ghost) return;

		bool visible = a.visible || b.visible;
		if (!show || !visible) {
			ghost->setVisible(false);
			return;
		}
		ghost->setVisible(true);

		ghost->setPositionX(lerpf(a.x, b.x, alpha));
		ghost->setPositionY(lerpf(a.y, b.y, alpha));
		ghost->setRotation(lerpf(a.rotation, b.rotation, alpha));
		ghost->setScaleX(lerpf(a.scaleX, b.scaleX, alpha));
		ghost->setScaleY(lerpf(a.scaleY, b.scaleY, alpha));

		applyGhostMode(ghost, appliedMode, a.mode);
		appliedMode = a.mode;
	}

	void playGhostStep(float dt) {
		auto& frames = m_fields->m_playbackRun.frames;
		if (frames.empty() || m_fields->m_ghosts.size() < 2) return;

		bool show = m_fields->m_masterEnabled && m_fields->m_levelEnabled
			&& !(m_isPracticeMode && !m_fields->m_recordInPractice);

		if (!show) {
			for (auto* g : m_fields->m_ghosts) if (g) g->setVisible(false);
			return;
		}

		m_fields->m_playbackElapsed += dt;

		auto& cum = m_fields->m_playbackCumulative;
		size_t lastIdx = frames.size() - 1;

		while (m_fields->m_playbackIndex < lastIdx && cum[m_fields->m_playbackIndex] <= m_fields->m_playbackElapsed) {
			m_fields->m_playbackIndex++;
		}

		size_t i0 = m_fields->m_playbackIndex;
		size_t i1 = std::min(i0 + 1, lastIdx);

		float t0 = (i0 == 0) ? 0.f : cum[i0 - 1];
		float t1 = cum[i0];
		float alpha = 0.f;
		if (i1 != i0 && t1 > t0) {
			alpha = std::clamp((m_fields->m_playbackElapsed - t0) / (t1 - t0), 0.f, 1.f);
		}

		applyFrameToGhost(m_fields->m_ghosts[0], m_fields->m_ghost1Mode, frames[i0].p1, frames[i1].p1, alpha, true);
		applyFrameToGhost(m_fields->m_ghosts[1], m_fields->m_ghost2Mode, frames[i0].p2, frames[i1].p2, alpha, true);
	}

	void finalizeAttempt() {
		float finalPercent = m_fields->m_attemptBestPercent;

		if (shouldRecordNow() && finalPercent >= m_fields->m_playbackRun.bestPercent) {
			GhostRun newRun;
			newRun.levelName = m_level ? std::string(m_level->m_levelName) : "";
			newRun.bestPercent = finalPercent;
			newRun.enabledForLevel = m_fields->m_levelEnabled;
			newRun.frames = m_fields->m_recording;

			saveGhostRun(m_level, newRun);

			m_fields->m_playbackRun = newRun;
			recomputeCumulative();
		}
	}

	bool init(GJGameLevel* level, bool p1, bool p2) {
		if (!PlayLayer::init(level, p1, p2)) return false;

		g_activeLayer = this;

		loadSettingsAndState();
		m_fields->m_playbackRun = loadGhostRun(level);
		recomputeCumulative();

		setupGhostPlayers();
		refreshGhostVisibility();

		return true;
	}

	void startGame() {
		PlayLayer::startGame();

		m_fields->m_started = true;
		m_fields->m_recording.clear();
		m_fields->m_attemptBestPercent = 0.f;
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		m_fields->m_recording.clear();
		m_fields->m_attemptBestPercent = 0.f;
		m_fields->m_playbackElapsed = 0.f;
		m_fields->m_playbackIndex = 0;
		m_fields->m_ghost1Mode = 0;
		m_fields->m_ghost2Mode = 0;
	}

	void updateVisibility(float dt) {
		PlayLayer::updateVisibility(dt);

		if (!m_fields->m_started || !m_player1) return;

		if (shouldRecordNow()) {
			recordFrame(dt);
			float pct = getCurrentPercent();
			if (pct > m_fields->m_attemptBestPercent) m_fields->m_attemptBestPercent = pct;
		}

		playGhostStep(dt);
		refreshGhostVisibility();
	}

	void levelComplete() {
		m_fields->m_attemptBestPercent = 100.f;
		finalizeAttempt();

		PlayLayer::levelComplete();
	}

	void onExit() {
		if (g_activeLayer == this) g_activeLayer = nullptr;
		PlayLayer::onExit();
	}
};

class $modify(PlayerObject) {
	void playerDestroyed(bool p0) {
		PlayLayer* pl = PlayLayer::get();
		bool isRealPlayer = pl && (pl->m_player1 == this || pl->m_player2 == this);

		PlayerObject::playerDestroyed(p0);

		if (isRealPlayer && g_activeLayer == pl) {
			g_activeLayer->finalizeAttempt();
		}
	}
};

void refreshLiveGhostState() {
	if (!g_activeLayer) return;
	g_activeLayer->loadSettingsAndState();
	g_activeLayer->refreshGhostVisibility();
}
