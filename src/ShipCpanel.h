// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SHIPCPANEL_H
#define _SHIPCPANEL_H

#include "libs.h"
#include "gui/Gui.h"
#include "ShipCpanelMultiFuncDisplays.h"
#include "Ship.h"
#include "Serializer.h"
#include "Game.h"
#include "WorldView.h"

class Body;
class SpaceStation;
namespace Graphics { class Renderer; }

class ShipCpanel: public Gui::Fixed {
public:
	ShipCpanel(Graphics::Renderer *r);
    ShipCpanel(Serializer::Reader &rd, Graphics::Renderer *r);
	virtual ~ShipCpanel();
	virtual void Draw();
	void Update();
	MsgLogWidget *MsgLog() { return m_msglog; }
	MsgLogWidget *InfLog() { return m_inflog; }
	void SetAlertState(Ship::AlertState as);

	void TimeStepUpdate(float step);

	void Save(Serializer::Writer &wr);

	enum OverlayTextPos {
		OVERLAY_TOP_LEFT,
		OVERLAY_TOP_RIGHT,
		OVERLAY_BOTTOM_LEFT,
		OVERLAY_BOTTOM_RIGHT,
		OVERLAY_HUD2_LEFT,
		OVERLAY_HUD2_LEFT_2,
		OVERLAY_HUD2_LEFT_3,
		OVERLAY_HUD2_LEFT_4,
	};
	void SetOverlayText(OverlayTextPos pos, const std::string &text);
	void SetOverlayToolTip(OverlayTextPos pos, const std::string &text);
	void ClearOverlay();
	void ChangeCamButtonState(WorldView::CamType cam_type);

private:
	void InitObject();

	enum MapView { MAP_SECTOR, MAP_SYSTEM, MAP_INFO, MAP_GALACTIC };

	void OnChangeCamView(Gui::MultiStateImageButton *b);
	void OnChangeToMapView(Gui::MultiStateImageButton *b);
	void OnChangeMapView(enum MapView);
	void OnChangeInfoView(Gui::MultiStateImageButton *b);
	void OnClickComms(Gui::MultiStateImageButton *b);
	void OnDockingClearanceExpired(const SpaceStation *);

	void OnUserChangeMultiFunctionDisplay(multifuncfunc_t f);
	void ChangeMultiFunctionDisplay(multifuncfunc_t selected);
	void OnMultiFuncGrabFocus(multifuncfunc_t);
	void OnMultiFuncUngrabFocus(multifuncfunc_t);
	void HideMapviewButtons();

	enum MapView m_currentMapView;
	multifuncfunc_t m_userSelectedMfuncWidget;
	Gui::Label *m_clock;

	sigc::connection m_connOnDockingClearanceExpired;

	ScannerWidget *m_scanner;
	MsgLogWidget *m_msglog;
	MsgLogWidget *m_inflog;
	UseEquipWidget *m_useEquipWidget;
	Gui::MultiStateImageButton *m_camButton;
	Gui::RadioGroup *m_leftButtonGroup, *m_rightButtonGroup;
	Gui::Widget *m_mapViewButtons[4];
	Gui::Image *m_alertLights[3];

	Gui::Label *m_overlay[8];
};

#endif /* _SHIP_CPANEL_H */
