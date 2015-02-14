// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "libs.h"
#include "Pi.h"
#include "ShipCpanel.h"
#include "Player.h"
#include "WorldView.h"
#include "SpaceStation.h"
#include "ShipCpanelMultiFuncDisplays.h"
#include "SectorView.h"
#include "SystemView.h"
#include "SystemInfoView.h"
#include "GalacticView.h"
#include "UIView.h"
#include "Lang.h"
#include "Game.h"

// XXX duplicated in WorldView. should probably be a theme variable
static const Color s_hudTextColor(0,255,0,204);
static const vector2f s_centerCircleSize(148.0f, 192.0f);

ShipCpanel::ShipCpanel(Graphics::Renderer *r):
	Gui::Fixed(float(Gui::Screen::GetWidth()), float(Gui::Screen::GetHeight()))
{
	m_scanner = new ScannerWidget(r);

	InitObject();
}

ShipCpanel::ShipCpanel(Serializer::Reader &rd, Graphics::Renderer *r): 
	Gui::Fixed(float(Gui::Screen::GetWidth()), float(Gui::Screen::GetHeight()))
{
	m_scanner = new ScannerWidget(r, rd);

	InitObject();

	m_camButton->SetActiveState(rd.Int32());
}

void ShipCpanel::InitObject()
{
	SetTransparency(true);

	Gui::Image *img = new Gui::Image("icons/cpanel.png");
	img->SetRenderDimensions(800, 80);
	Add(img, 0, 520);

	m_currentMapView = MAP_SECTOR;
	m_useEquipWidget = new UseEquipWidget();
	//m_msglog = new MsgLogWidget();
	m_inflog = new MsgLogWidget();

	m_userSelectedMfuncWidget = MFUNC_SCANNER;

	m_scanner->onGrabFocus.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnMultiFuncGrabFocus), MFUNC_SCANNER));
	m_useEquipWidget->onGrabFocus.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnMultiFuncGrabFocus), MFUNC_EQUIPMENT));
	//m_msglog->onGrabFocus.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnMultiFuncGrabFocus), MFUNC_MSGLOG));

	m_scanner->onUngrabFocus.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnMultiFuncUngrabFocus), MFUNC_SCANNER));
	m_useEquipWidget->onUngrabFocus.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnMultiFuncUngrabFocus), MFUNC_EQUIPMENT));
	//m_msglog->onUngrabFocus.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnMultiFuncUngrabFocus), MFUNC_MSGLOG));

	ChangeMultiFunctionDisplay(MFUNC_SCANNER);

//	Gui::RadioGroup *g = new Gui::RadioGroup();
	float ui_width = static_cast<float>(Gui::Screen::GetWidth());
	const float pp_margin = 40.0f, clock_margin = 135.0f;

	Gui::ImageRadioButton *b;

	m_leftButtonGroup = new Gui::RadioGroup();
	m_camButton = new Gui::MultiStateImageButton();
	m_leftButtonGroup->Add(m_camButton);
	m_camButton->SetSelected(true);
	m_camButton->AddState(WorldView::CAM_INTERNAL, "icons/cam_internal.png", "icons/cam_internal_on.png", Lang::INTERNAL_VIEW);
	m_camButton->AddState(WorldView::CAM_EXTERNAL, "icons/cam_external.png", "icons/cam_external_on.png", Lang::EXTERNAL_VIEW);
	m_camButton->AddState(WorldView::CAM_SIDEREAL, "icons/cam_sidereal.png", "icons/cam_sidereal_on.png", Lang::SIDEREAL_VIEW);
	m_camButton->SetShortcut(SDLK_F1, KMOD_NONE);
	m_camButton->onClick.connect(sigc::mem_fun(this, &ShipCpanel::OnChangeCamView));
	m_camButton->SetRenderDimensions(30, 22);
	Add(m_camButton, 214, 56 + 520);

	Gui::MultiStateImageButton *map_button = new Gui::MultiStateImageButton();
	m_leftButtonGroup->Add(map_button);
	map_button->SetSelected(false);
	map_button->SetShortcut(SDLK_F2, KMOD_NONE);
	map_button->AddState(0, "icons/cpan_f2_map.png", "icons/cpan_f2_map_on.png", Lang::NAVIGATION_STAR_MAPS);
	map_button->onClick.connect(sigc::mem_fun(this, &ShipCpanel::OnChangeToMapView));
	map_button->SetRenderDimensions(30, 22);
	Add(map_button, 247, 56 + 520);

	Gui::MultiStateImageButton *info_button = new Gui::MultiStateImageButton();
	m_leftButtonGroup->Add(info_button);
	info_button->SetSelected(false);
	info_button->SetShortcut(SDLK_F3, KMOD_NONE);
	info_button->AddState(0, "icons/cpan_f3_shipinfo.png", "icons/cpan_f3_shipinfo_on.png", Lang::SHIP_INFORMATION);
	info_button->onClick.connect(sigc::mem_fun(this, &ShipCpanel::OnChangeInfoView));
	info_button->SetRenderDimensions(30, 22);
	Add(info_button, 279, 56 + 520);

	m_commsButton = new Gui::MultiStateImageButton();
	m_leftButtonGroup->Add(m_commsButton);
	m_commsButton->SetSelected(false);
	m_commsButton->SetShortcut(SDLK_F4, KMOD_NONE);
	m_commsButton->AddState(FLIGHT_BUTTON_UNAVAILABLE, "icons/comms_unavailable.png", Lang::COMMS);
	m_commsButton->AddState(FLIGHT_BUTTON_OFF, "icons/comms_off.png", Lang::COMMS);
	m_commsButton->AddState(FLIGHT_BUTTON_ON, "icons/comms_on.png", Lang::COMMS);
	m_commsButton->onClick.connect(sigc::mem_fun(this, &ShipCpanel::OnClickComms));
	m_commsButton->SetRenderDimensions(30, 22);
	m_commsButton->SetEnabled(false);
	Add(m_commsButton, 312, 56 + 520);

	Gui::Screen::PushFont("OverlayFont");
	m_clock = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	Add(m_clock, ui_width - clock_margin, 64 + 520);
	Gui::Screen::PopFont();

	m_rightButtonGroup = new Gui::RadioGroup();
	b = new Gui::ImageRadioButton(m_rightButtonGroup, "icons/map_sector_view.png", "icons/map_sector_view_on.png");
	m_rightButtonGroup->SetSelected(0);
	b->onSelect.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnChangeMapView), MAP_SECTOR));
	b->SetShortcut(SDLK_F5, KMOD_NONE);
	b->SetToolTip(Lang::GALAXY_SECTOR_VIEW);
	b->SetRenderDimensions(30, 22);
	Add(b, 459, 56 + 520);
	m_mapViewButtons[0] = b;
	b = new Gui::ImageRadioButton(m_rightButtonGroup, "icons/map_system_view.png", "icons/map_system_view_on.png");
	b->onSelect.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnChangeMapView), MAP_SYSTEM));
	b->SetShortcut(SDLK_F6, KMOD_NONE);
	b->SetToolTip(Lang::SYSTEM_ORBIT_VIEW);
	b->SetRenderDimensions(30, 22);
	Add(b, 491, 56 + 520);
	m_mapViewButtons[1] = b;
	b = new Gui::ImageRadioButton(m_rightButtonGroup, "icons/map_sysinfo_view.png", "icons/map_sysinfo_view_on.png");
	b->onSelect.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnChangeMapView), MAP_INFO));
	b->SetShortcut(SDLK_F7, KMOD_NONE);
	b->SetToolTip(Lang::STAR_SYSTEM_INFORMATION);
	b->SetRenderDimensions(30, 22);
	Add(b, 524, 56 + 520);
	m_mapViewButtons[2] = b;
	b = new Gui::ImageRadioButton(m_rightButtonGroup, "icons/map_galactic_view.png", "icons/map_galactic_view_on.png");
	b->onSelect.connect(sigc::bind(sigc::mem_fun(this, &ShipCpanel::OnChangeMapView), MAP_GALACTIC));
	b->SetShortcut(SDLK_F8, KMOD_NONE);
	b->SetToolTip(Lang::GALACTIC_VIEW);
	b->SetRenderDimensions(30, 22);
	Add(b, 556, 56 + 520);
	m_mapViewButtons[3] = b;

	img = new Gui::Image("icons/alert_green.png");
	img->SetToolTip(Lang::NO_ALERT);
	img->SetRenderDimensions(20, 13);
	Add(img, 388, 5 + 520);
	m_alertLights[0] = img;
	img = new Gui::Image("icons/alert_yellow.png");
	img->SetToolTip(Lang::SHIP_NEARBY);
	img->SetRenderDimensions(20, 13);
	Add(img, 388, 5 + 520);
	m_alertLights[1] = img;
	img = new Gui::Image("icons/alert_red.png");
	img->SetToolTip(Lang::LASER_FIRE_DETECTED);
	img->SetRenderDimensions(20, 13);
	Add(img, 388, 5 + 520);
	m_alertLights[2] = img;

	float cursor = 23.0f + 520, cursor_inc = 8.0f;
	const float hh = static_cast<float>(Gui::Screen::GetHeight() / 2) + 3;
	const float hw = static_cast<float>(Gui::Screen::GetWidth() / 2);
	const float top = hh - (s_centerCircleSize.y / 2.0f);
	const float bottom = top + s_centerCircleSize.y;
	const float left = hw - (s_centerCircleSize.x / 2.0f);
	const float right = left + s_centerCircleSize.x;

	Gui::Screen::PushFont("HudFont");
	m_overlay[OVERLAY_TOP_LEFT]     = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_TOP_RIGHT]    = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_BOTTOM_LEFT]  = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_BOTTOM_RIGHT] = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_HUD2_LEFT] = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_HUD2_LEFT_2] = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_HUD2_LEFT_3] = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_HUD2_LEFT_4] = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_HUD2_LEFT_5] = (new Gui::Label(""))->Color(s_hudTextColor);
	m_overlay[OVERLAY_DEV] = (new Gui::Label(""))->Color(Color::WHITE);
	Gui::Screen::PopFont();

	Gui::Screen::PushFont("SmallHudFont");
	m_overlay[OVERLAY_CIRCLE_TOP_1] = (new Gui::Label(""))->Color(Color::PARAGON_BLUE);
	m_overlay[OVERLAY_CIRCLE_TOP_2] = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_overlay[OVERLAY_CIRCLE_TOP_1]->SetAlignment(Gui::Alignment::ALIGN_CENTER);
	m_overlay[OVERLAY_CIRCLE_TOP_2]->SetAlignment(Gui::Alignment::ALIGN_CENTER);

	m_overlay[OVERLAY_CIRCLE_LEFT_1] = (new Gui::Label(""))->Color(Color::PARAGON_BLUE);
	m_overlay[OVERLAY_CIRCLE_LEFT_2] = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_overlay[OVERLAY_CIRCLE_LEFT_1]->SetAlignment(Gui::Alignment::ALIGN_RIGHT);
	m_overlay[OVERLAY_CIRCLE_LEFT_2]->SetAlignment(Gui::Alignment::ALIGN_RIGHT);

	m_overlay[OVERLAY_CIRCLE_RIGHT_1] = (new Gui::Label(""))->Color(Color::PARAGON_BLUE);
	m_overlay[OVERLAY_CIRCLE_RIGHT_2] = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_overlay[OVERLAY_CIRCLE_RIGHT_1]->SetAlignment(Gui::Alignment::ALIGN_LEFT);
	m_overlay[OVERLAY_CIRCLE_RIGHT_2]->SetAlignment(Gui::Alignment::ALIGN_LEFT);

	m_overlay[OVERLAY_CIRCLE_BOTTOM_1] = (new Gui::Label(""))->Color(Color::PARAGON_BLUE);
	m_overlay[OVERLAY_CIRCLE_BOTTOM_2] = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_overlay[OVERLAY_CIRCLE_BOTTOM_1]->SetAlignment(Gui::Alignment::ALIGN_CENTER);
	m_overlay[OVERLAY_CIRCLE_BOTTOM_2]->SetAlignment(Gui::Alignment::ALIGN_CENTER);


	Add(m_overlay[OVERLAY_TOP_LEFT], 214.0f, 22.0f + 520);
	Add(m_overlay[OVERLAY_TOP_RIGHT], 460.0f, 22.0f + 520);
	Add(m_overlay[OVERLAY_BOTTOM_LEFT], 214.0f, 34.0f + 520);
	Add(m_overlay[OVERLAY_BOTTOM_RIGHT], 460.0f, 34.0f + 520);
	
	//Add(m_overlay[OVERLAY_HUD2_LEFT], 2.0f, cursor); cursor += cursor_inc;
	//Add(m_overlay[OVERLAY_HUD2_LEFT_2], 2.0f, cursor); cursor += cursor_inc;
	//Add(m_overlay[OVERLAY_HUD2_LEFT_3], 2.0f, cursor); cursor += cursor_inc;
	//Add(m_overlay[OVERLAY_HUD2_LEFT_4], 2.0f, cursor); cursor += cursor_inc;
	//Add(m_overlay[OVERLAY_HUD2_LEFT_5], 2.0f, cursor); //cursor += cursor_inc;
	Add(m_overlay[OVERLAY_DEV],			2.0f, cursor); cursor += cursor_inc;

	Add(m_overlay[OVERLAY_CIRCLE_TOP_1], hw, top - cursor_inc);
	Add(m_overlay[OVERLAY_CIRCLE_TOP_2], hw, top - (cursor_inc * 2.0f));
	Add(m_overlay[OVERLAY_CIRCLE_LEFT_1], left - 100.0f, hh - cursor_inc);
	m_overlay[OVERLAY_CIRCLE_LEFT_1]->SetRightMargin(100.0f);
	Add(m_overlay[OVERLAY_CIRCLE_LEFT_2], left - 100.0f, hh);
	m_overlay[OVERLAY_CIRCLE_LEFT_2]->SetRightMargin(100.0f);
	Add(m_overlay[OVERLAY_CIRCLE_RIGHT_1], right, hh - cursor_inc);
	Add(m_overlay[OVERLAY_CIRCLE_RIGHT_2], right, hh);
	Add(m_overlay[OVERLAY_CIRCLE_BOTTOM_1], hw, bottom);
	Add(m_overlay[OVERLAY_CIRCLE_BOTTOM_2], hw, bottom + cursor_inc);

	Gui::Screen::PopFont();

	m_connOnDockingClearanceExpired =
		Pi::onDockingClearanceExpired.connect(sigc::mem_fun(this, &ShipCpanel::OnDockingClearanceExpired));
}

ShipCpanel::~ShipCpanel()
{
	delete m_leftButtonGroup;
	delete m_rightButtonGroup;
	Remove(m_scanner);
	Remove(m_useEquipWidget);
	//Remove(m_msglog);
	Remove(m_inflog);
	delete m_scanner;
	delete m_useEquipWidget;
	//delete m_msglog;
	delete m_inflog;
	m_connOnDockingClearanceExpired.disconnect();

	m_commsButton = nullptr; // Should've been deleted automatically by left button group
}

void ShipCpanel::ChangeMultiFunctionDisplay(multifuncfunc_t f)
{
	Gui::Widget *selected = 0;
	if (f == MFUNC_EQUIPMENT) selected = m_useEquipWidget;
	//if (f == MFUNC_MSGLOG) selected = m_msglog;

	Remove(m_scanner);
	Remove(m_useEquipWidget);
	//Remove(m_msglog);
	Remove(m_inflog);
	if (selected) {
		//if (selected == m_msglog)
		//	Add(selected, 214, -34 + 520);
		//else
			Add(selected, 200, 18 + 520);
		selected->ShowAll();
	}

	//XXX hack to keep the scanner on, always.
	Add(m_scanner, 200, 18 + 520);
	m_scanner->ShowAll();

	//XXX hack to keep InfoBanner on
	//Add(m_inflog, 350, -340 + 520);
	Add(m_inflog, Gui::Screen::GetWidth() / 2, -340 + 520);
	m_inflog->ShowAll();

	//XXX hack to always show missiles
	Add(m_useEquipWidget, 200, 18 + 520);
	m_useEquipWidget->ShowAll();
}

void ShipCpanel::OnMultiFuncGrabFocus(multifuncfunc_t f)
{
	ChangeMultiFunctionDisplay(f);
}

void ShipCpanel::OnMultiFuncUngrabFocus(multifuncfunc_t f)
{
	ChangeMultiFunctionDisplay(m_userSelectedMfuncWidget);
}

void ShipCpanel::OnDockingClearanceExpired(const SpaceStation *s)
{
	//MsgLog()->ImportantMessage(s->GetLabel(), Lang::DOCKING_CLEARANCE_EXPIRED);
	Pi::game->log->Add(s->GetLabel(), Lang::DOCKING_CLEARANCE_EXPIRED);
}

void ShipCpanel::Update()
{
	PROFILE_SCOPED()

	m_scanner->Update();
	m_useEquipWidget->Update();
	//m_msglog->Update();
	m_inflog->Update();

	// New comms button behavior, only activated when player is docked
	if (Pi::player->GetFlightState() == Ship::DOCKED) {
		if (m_commsButton->GetState() == FlightButtonStatus::FLIGHT_BUTTON_UNAVAILABLE) {
			m_commsButton->SetEnabled(true);
			m_commsButton->SetActiveState(FlightButtonStatus::FLIGHT_BUTTON_OFF);
		}
	} else {
		m_commsButton->SetActiveState(FlightButtonStatus::FLIGHT_BUTTON_UNAVAILABLE);
		m_commsButton->SetEnabled(false);
	}
}

void ShipCpanel::Draw()
{
	std::string time = format_date(Pi::game->GetTime());
	m_clock->SetText(time);

	View *cur = Pi::GetView();
	if ((cur != Pi::sectorView) && (cur != Pi::systemView) &&
	    (cur != Pi::systemInfoView) && (cur != Pi::galacticView)) {
		HideMapviewButtons();
	}

	Gui::Fixed::Draw();
}

void ShipCpanel::OnChangeCamView(Gui::MultiStateImageButton *b)
{
	Pi::BoinkNoise();
	const int newState = b->GetState();
	b->SetActiveState(newState);
	Pi::worldView->SetCamType(WorldView::CamType(newState));
	Pi::SetView(Pi::worldView);
}

void ShipCpanel::OnChangeInfoView(Gui::MultiStateImageButton *b)
{
	Pi::BoinkNoise();
	if (Pi::GetView() != Pi::infoView)
		Pi::SetView(Pi::infoView);
}

void ShipCpanel::OnChangeToMapView(Gui::MultiStateImageButton *b)
{
	Pi::BoinkNoise();
	OnChangeMapView(m_currentMapView);
}

void ShipCpanel::OnChangeMapView(enum MapView view)
{
	m_currentMapView = view;
	switch (m_currentMapView) {
		case MAP_SECTOR: Pi::SetView(Pi::sectorView); break;
		case MAP_SYSTEM: Pi::SetView(Pi::systemView); break;
		case MAP_INFO:
			if (Pi::GetView() == Pi::systemInfoView) {
				Pi::systemInfoView->NextPage();
			} else {
				Pi::SetView(Pi::systemInfoView);
			}
			break;
		case MAP_GALACTIC: Pi::SetView(Pi::galacticView); break;
	}
	for (int i=0; i<4; i++) m_mapViewButtons[i]->Show();
}

void ShipCpanel::HideMapviewButtons()
{
	for (int i=0; i<4; i++) m_mapViewButtons[i]->Hide();
}

void ShipCpanel::OnClickComms(Gui::MultiStateImageButton *b)
{
	Pi::BoinkNoise();
	if (Pi::player->GetFlightState() == Ship::DOCKED) {
		Pi::SetView(Pi::spaceStationView);
	} else {
		assert(0);
		//Pi::SetView(Pi::worldView);
		//Pi::worldView->ToggleTargetActions();
	}
}

void ShipCpanel::SetAlertState(Ship::AlertState as)
{
	switch (as) {
		case Ship::ALERT_NONE:
			m_alertLights[0]->Show();
			m_alertLights[1]->Hide();
			m_alertLights[2]->Hide();
			break;
		case Ship::ALERT_SHIP_NEARBY:
			m_alertLights[0]->Hide();
			m_alertLights[1]->Show();
			m_alertLights[2]->Hide();
			break;
		case Ship::ALERT_SHIP_FIRING:
			m_alertLights[0]->Hide();
			m_alertLights[1]->Hide();
			m_alertLights[2]->Show();
			break;
	}
}

void ShipCpanel::TimeStepUpdate(float step)
{
	PROFILE_SCOPED()
	m_scanner->TimeStepUpdate(step);
}

void ShipCpanel::Save(Serializer::Writer &wr)
{
	m_scanner->Save(wr);
	wr.Int32(m_camButton->GetState());
}

void ShipCpanel::SetOverlayText(OverlayTextPos pos, const std::string &text)
{
	m_overlay[pos]->SetText(text);
	if (text.length() == 0)
		m_overlay[pos]->Hide();
	else
		m_overlay[pos]->Show();
}

void ShipCpanel::SetOverlayToolTip(OverlayTextPos pos, const std::string &text)
{
	m_overlay[pos]->SetToolTip(text);
}

void ShipCpanel::ClearOverlay()
{
	for (int i = 0; i < static_cast<int>(OVERLAY_COUNT); i++) {
		m_overlay[i]->SetText("");
		m_overlay[i]->SetToolTip("");
	}
}

// This is used by WorldView to update camera button when camera type changes
void ShipCpanel::ChangeCamButtonState(WorldView::CamType cam_type)
{
	m_camButton->SetActiveState(static_cast<int>(cam_type));
}

void ShipCpanel::SetCircleOverlayVisiblity(bool visible)
{
	if(visible) {
		for(unsigned i = OVERLAY_CIRCLE_TOP_1; i <= OVERLAY_CIRCLE_BOTTOM_2; ++i) {
			m_overlay[i]->Show();
		}
	} else {
		for (unsigned i = OVERLAY_CIRCLE_TOP_1; i <= OVERLAY_CIRCLE_BOTTOM_2; ++i) {
			m_overlay[i]->Hide();
		}
	}
}
