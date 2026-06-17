#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "GhostStorage.hpp"
#include "GhostLive.hpp"

using namespace geode::prelude;

class GhostSettingsPopup : public geode::Popup {
protected:
	GJGameLevel* m_targetLevel = nullptr;
	CCLabelBMFont* m_statusLabel = nullptr;

	bool init(GJGameLevel* level) {
		if (!Popup::init(220.f, 160.f)) return false;

		m_targetLevel = level;
		this->setTitle("Best Run Ghost");

		auto run = loadGhostRun(level);

		auto statusText = run.frames.empty()
			? std::string("No best run recorded yet.")
			: std::string(CCString::createWithFormat("Best progress: %.0f%%", run.bestPercent)->getCString());

		m_statusLabel = CCLabelBMFont::create(statusText.c_str(), "chatFont.fnt");
		m_statusLabel->setScale(0.6f);
		m_mainLayer->addChildAtPosition(m_statusLabel, Anchor::Top, ccp(0, -38));

		auto toggleLabel = CCLabelBMFont::create("Show ghost on this level", "chatFont.fnt");
		toggleLabel->setScale(0.5f);
		m_mainLayer->addChildAtPosition(toggleLabel, Anchor::Center, ccp(-18, 8));

		auto menu = CCMenu::create();
		menu->setPosition({0.f, 0.f});

		auto toggle = CCMenuItemToggler::createWithStandardSprites(
			this, menu_selector(GhostSettingsPopup::onToggleEnabled), 0.7f
		);
		toggle->toggle(run.enabledForLevel);
		menu->addChild(toggle);
		toggle->setPosition({0.f, 0.f});
		m_mainLayer->addChildAtPosition(menu, Anchor::Center, ccp(70, 8));

		auto resetMenu = CCMenu::create();
		resetMenu->setPosition({0.f, 0.f});
		auto resetBtn = CCMenuItemSpriteExtra::create(
			ButtonSprite::create("Reset Ghost Data", "goldFont.fnt", "GJ_button_06.png", 0.8f),
			this,
			menu_selector(GhostSettingsPopup::onReset)
		);
		resetMenu->addChild(resetBtn);
		resetBtn->setPosition({0.f, 0.f});
		m_mainLayer->addChildAtPosition(resetMenu, Anchor::Bottom, ccp(0, 30));

		return true;
	}

	void onToggleEnabled(CCObject* sender) {
		auto toggler = static_cast<CCMenuItemToggler*>(sender);
		bool enabled = toggler->isToggled();
		setGhostEnabledForLevel(m_targetLevel, enabled);
		refreshLiveGhostState();
	}

	void onReset(CCObject*) {
		createQuickPopup(
			"Reset Ghost?",
			"This deletes your saved best run for this level. Are you sure?",
			"Cancel", "Reset",
			[this](auto, bool confirmed) {
				if (!confirmed) return;
				deleteGhostRun(m_targetLevel);
				refreshLiveGhostState();
				if (m_statusLabel) {
					m_statusLabel->setString("No best run recorded yet.");
				}
			}
		);
	}

public:
	static GhostSettingsPopup* create(GJGameLevel* level) {
		auto ret = new GhostSettingsPopup();
		if (ret->init(level)) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}
};
