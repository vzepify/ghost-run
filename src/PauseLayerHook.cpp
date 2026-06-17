#include <Geode/Geode.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>

#include "GhostSettingsPopup.hpp"

using namespace geode::prelude;

class $modify(XPauseLayer, PauseLayer) {
	void customSetup() {
		PauseLayer::customSetup();

		auto menu = this->getChildByID("right-button-menu");
		if (!menu) return;

		auto spr = ButtonSprite::create("Ghost", "goldFont.fnt", "GJ_button_04.png", 0.8f);
		spr->setScale(0.65f);

		auto btn = CCMenuItemSpriteExtra::create(
			spr, this, menu_selector(XPauseLayer::onGhostSettings)
		);
		btn->setID("best-run-ghost-button"_spr);

		menu->addChild(btn);
		menu->updateLayout();
	}

	void onGhostSettings(CCObject*) {
		auto pl = PlayLayer::get();
		if (!pl || !pl->m_level) return;

		auto popup = GhostSettingsPopup::create(pl->m_level);
		if (popup) popup->show();
	}
};
