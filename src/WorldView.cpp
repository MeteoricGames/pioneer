// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// d*
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "WorldView.h"
#include "Pi.h"
#include "Frame.h"
#include "Player.h"
#include "Planet.h"
#include "galaxy/Sector.h"
#include "galaxy/SectorCache.h"
#include "SectorView.h"
#include "Serializer.h"
#include "ShipCpanel.h"
#include "ThrusterTrail.h"
#include "Sound.h"
#include "Space.h"
#include "SpaceStation.h"
#include "galaxy/StarSystem.h"
#include "HyperspaceCloud.h"
#include "KeyBindings.h"
#include "perlin.h"
#include "Lang.h"
#include "StringF.h"
#include "Game.h"
#include "graphics/Graphics.h"
#include "graphics/Renderer.h"
#include "graphics/Frustum.h"
#include "graphics/TextureBuilder.h"
#include "graphics/Drawables.h"
#include "matrix4x4.h"
#include "Quaternion.h"
#include <algorithm>
#include <sstream>
#include <SDL_stdinc.h>
#include "gui/GuiVScrollPortal.h"
#include "gui/GuiFixed.h"
#include "MainMaterial.h"
#include "Tweaker.h"

const double WorldView::PICK_OBJECT_RECT_SIZE = 20.0;
static const Color s_hudTextColor(0,255,0,230);
static const float ZOOM_SPEED = 1.f;
static const float WHEEL_SENSITIVITY = .05f;	// Should be a variable in user settings.

static const float HUD_CROSSHAIR_SIZE	= 10.0f;
static const Uint8 HUD_ALPHA			= 87;
static const Uint8 CONTACT_LIST_LIMIT   = 4;
static Uint32 m_fastTravelTimeout = 0;
static const float HUD_RETICLE_SIZE_X	= 0.084f; // 162 / 1920
static const float HUD_RETICLE_SIZE_Y	= 0.15;	  // 162 / 1080
static bool isDocking = true;

Body *m_fastTravelTarget = nullptr;

static void autopilot_flyto(Body *b);
static void autopilot_dock(Body *b);
static void autopilot_orbit(Body *b, double alt);
static void player_target_hypercloud(HyperspaceCloud *cloud);


WorldView::WorldView(): UIView()
{
	m_tutorialSeen = false;
	m_camType = CAM_INTERNAL;
	InitObject();
}

WorldView::WorldView(Serializer::Reader &rd): UIView()
{
	m_tutorialSeen = rd.Bool();
	m_camType = CamType(rd.Int32());
	InitObject();
	m_internalCameraController->Load(rd);
	m_externalCameraController->Load(rd);
	m_siderealCameraController->Load(rd);
}

static const float LOW_THRUST_LEVELS[] = { 0.75, 0.5, 0.25, 0.1, 0.05, 0.01 };

void WorldView::InitObject()
{
	Graphics::Renderer* renderer = Gui::Screen::GetRenderer();

	float size[2];
	GetSizeRequested(size);

	m_showTargetActionsTimeout = 0;
	m_showLowThrustPowerTimeout = 0;
	m_labelsOn = true;
	SetTransparency(true);

	Graphics::RenderStateDesc rsd;
	rsd.blendMode = Graphics::BLEND_ALPHA;
	rsd.depthWrite = false;
	rsd.depthTest = false;
	m_blendState = Pi::renderer->CreateRenderState(rsd); //XXX m_renderer not set yet
	m_navTunnel = new NavTunnelWidget(this, m_blendState);
	Add(m_navTunnel, 0, 0);

	m_commsOptions = new Fixed(size[0], size[1]/2);
	m_commsOptions->SetTransparency(true);
	Add(m_commsOptions, 130, 200);

	m_commsNavOptionsContainer = new Gui::HBox();
	m_commsNavOptionsContainer->SetSpacing(5);
	m_commsNavOptionsContainer->SetSizeRequest(130, size[1]-360);
	Add(m_commsNavOptionsContainer, 10, size[1] / 2 - (size[1] - 360) / 2);

	Gui::VScrollPortal *portal = new Gui::VScrollPortal();
	m_scroll = new Gui::NavVScrollBar();
	m_scroll->SetAdjustment(&portal->vscrollAdjust);
	m_commsNavOptionsContainer->PackStart(m_scroll);
	m_commsNavOptionsContainer->PackStart(portal);

	m_commsNavOptions = new Gui::VBox();
	m_commsNavOptions->SetSpacing(2);
	m_commsNavOptions->SetTransparency(true);
	portal->Add(m_commsNavOptions);

	m_lowThrustPowerOptions = new Gui::Fixed(size[0], size[1]/2);
	m_lowThrustPowerOptions->SetTransparency(true);
	Add(m_lowThrustPowerOptions, 10, 200);
	for (int i = 0; i < int(COUNTOF(LOW_THRUST_LEVELS)); ++i) {
		assert(i < 9); // otherwise the shortcuts break
		const int ypos = i*32;

		Gui::Label *label = new Gui::Label(
				stringf(Lang::SET_LOW_THRUST_POWER_LEVEL_TO_X_PERCENT,
					formatarg("power", 100.0f * LOW_THRUST_LEVELS[i], "f.0")));
		m_lowThrustPowerOptions->Add(label, 50, float(ypos));

		char buf[8];
		snprintf(buf, sizeof(buf), "%d", (i + 1));
		Gui::Button *btn = new Gui::LabelButton(new Gui::Label(buf));
		btn->SetShortcut(SDL_Keycode(SDLK_1 + i), KMOD_NONE);
		m_lowThrustPowerOptions->Add(btn, 16, float(ypos));

		btn->onClick.connect(sigc::bind(sigc::mem_fun(this, &WorldView::OnSelectLowThrustPower), LOW_THRUST_LEVELS[i]));
	}

	m_windowSizeUniformId = -1;
	m_trailGradient = nullptr;

	//// Altitude
	m_bAltitudeAvailable = false;
	m_altitude = 0.0;

	//// Paragon Flight System
	// Autopilot button
	m_flightAutopilotButton = new Gui::MultiStateImageButton();
	m_flightAutopilotButton->SetShortcut(SDLK_F5, KMOD_NONE);
	m_flightAutopilotButton->AddState(FLIGHT_BUTTON_UNAVAILABLE, "icons/autopilot_unavailable.png", Lang::AUTOPILOT_UNAVAILABLE);
	m_flightAutopilotButton->AddState(FLIGHT_BUTTON_OFF, "icons/autopilot_off.png", Lang::AUTOPILOT_OFF);
	m_flightAutopilotButton->AddState(FLIGHT_BUTTON_ON, "icons/autopilot_on.png", Lang::AUTOPILOT_ON);
	m_flightAutopilotButton->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickAutopilotButton));
	m_flightAutopilotButton->SetRenderDimensions(30.0f, 22.0f);
	m_rightButtonBar->Add(m_flightAutopilotButton, 0.0f, 2.0f);
	m_flightAutopilotButton->SetEnabled(false);
	// Maneuver button
	m_flightManeuverButton = new Gui::MultiStateImageButton();
	m_flightManeuverButton->SetShortcut(SDLK_F6, KMOD_NONE);
	m_flightManeuverButton->AddState(FLIGHT_BUTTON_UNAVAILABLE, "icons/maneuver_unavailable.png", Lang::MANEUVER_CONTROL_UNAVAILABLE);
	m_flightManeuverButton->AddState(FLIGHT_BUTTON_OFF, "icons/maneuver_off.png", Lang::MANEUVER_CONTROL_OFF);
	m_flightManeuverButton->AddState(FLIGHT_BUTTON_ON, "icons/maneuver_on.png", Lang::MANEUVER_CONTROL_ON);
	m_flightManeuverButton->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickManeuverButton));
	m_flightManeuverButton->SetRenderDimensions(30.0f, 22.0f);
	m_rightButtonBar->Add(m_flightManeuverButton, 34.0f, 2.0f);
	m_flightManeuverButton->SetEnabled(false);
	// Transit button
	m_flightTransitButton = new Gui::MultiStateImageButton();
	m_flightTransitButton->SetShortcut(SDLK_F7, KMOD_NONE);
	m_flightTransitButton->AddState(FLIGHT_BUTTON_UNAVAILABLE, "icons/transit_unavailable.png", Lang::TRANSIT_CONTROL_UNAVAILABLE);
	m_flightTransitButton->AddState(FLIGHT_BUTTON_OFF, "icons/transit_off.png", Lang::TRANSIT_CONTROL_OFF);
	m_flightTransitButton->AddState(FLIGHT_BUTTON_ON, "icons/transit_on.png", Lang::TRANSIT_CONTROL_ON);
	m_flightTransitButton->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickTransitButton));
	m_flightTransitButton->SetRenderDimensions(30.0f, 22.0f);
	m_rightButtonBar->Add(m_flightTransitButton, 66.0f, 2.0f);
	m_flightTransitButton->SetEnabled(false);
	// Jump button
	m_flightJumpButton = new Gui::MultiStateImageButton();
	m_flightJumpButton->SetShortcut(SDLK_F8, KMOD_NONE);
	m_flightJumpButton->AddState(FLIGHT_BUTTON_UNAVAILABLE, "icons/jump_unavailable.png", 
		Lang::JUMP_CONTROL_UNAVAILABLE);
	m_flightJumpButton->AddState(FLIGHT_BUTTON_OFF, "icons/jump_off.png", Lang::JUMP_CONTROL_OFF);
	m_flightJumpButton->AddState(FLIGHT_BUTTON_ON, "icons/jump_on.png", Lang::JUMP_CONTROL_ON);
	m_flightJumpButton->AddState(FLIGHT_PHASE_BUTTON_UNAVAILABLE, "icons/phase_jump_unavailable.png", 
		Lang::PHASE_JUMP_CONTROL_UNAVAILABLE);
	m_flightJumpButton->AddState(FLIGHT_PHASE_BUTTON_OFF, "icons/phase_jump_off.png", 
		Lang::PHASE_JUMP_CONTROL_OFF);
	m_flightJumpButton->AddState(FLIGHT_PHASE_BUTTON_ON, "icons/phase_jump_on.png", 
		Lang::PHASE_JUMP_CONTROL_ON);
	m_flightJumpButton->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickJumpButton));
	m_flightJumpButton->SetRenderDimensions(30.0f, 22.0f);
	m_rightButtonBar->Add(m_flightJumpButton, 98.0f, 2.0f);
	m_flightJumpButton->SetEnabled(false);
	////-----------------------------------------------------------

	// Hypserspace button
	m_hyperspaceButton = new Gui::ImageButton("icons/hyperspace_f8.png");
	m_hyperspaceButton->SetShortcut(SDLK_F7, KMOD_NONE);
	m_hyperspaceButton->SetToolTip(Lang::HYPERSPACE_JUMP);
	m_hyperspaceButton->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickHyperspace));
	m_hyperspaceButton->SetRenderDimensions(30.0f, 22.0f);
	//m_rightButtonBar->Add(m_hyperspaceButton, 66, 2);

	// Launch button
	m_launchButton = new Gui::ImageButton("icons/blastoff.png");
	m_launchButton->SetShortcut(SDLK_F5, KMOD_NONE);
	m_launchButton->SetToolTip(Lang::TAKEOFF);
	m_launchButton->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickBlastoff));
	m_launchButton->SetRenderDimensions(30.0f, 22.0f);
	m_rightButtonBar->Add(m_launchButton, 0.0f, 2);

	const float flightstatus_margin = 15.0f;

	Gui::Screen::PushFont("OverlayFont");
	m_flightStatus = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	//Add(m_flightStatus, 2, Gui::Screen::GetHeight() - flightstatus_margin);
	Gui::Screen::PopFont();

#if WITH_DEVKEYS
	Gui::Screen::PushFont("ConsoleFont");
	m_debugInfo = (new Gui::Label(""))->Color(204, 204, 204);
	Add(m_debugInfo, 10, 200);
	Gui::Screen::PopFont();
#endif

	m_hudHyperspaceInfo = (new Gui::Label(""))->Color(s_hudTextColor);
	m_hudHyperspaceInfo->SetAlignment(Gui::ALIGN_CENTER);
	Add(m_hudHyperspaceInfo, Gui::Screen::GetWidth() * 0.5f, Gui::Screen::GetHeight()*0.3f);

	float cursor, cursor_incr = 13.0f;

	// Location
	Gui::Screen::PushFont("HudFontLarge");
	m_hudLocationPrimary = (new Gui::Label("PROCYON"))->Color(Color::PARAGON_BLUE);
	m_hudLocationPrimary->SetAlignment(Gui::Alignment::ALIGN_CENTER);
	Gui::Screen::PopFont();
	Gui::Screen::PushFont("HudFont");
	m_hudLocationSecondary = (new Gui::Label("CLEARSPACE"))->Color(Color::PARAGON_GREEN);
	m_hudLocationSecondary->SetAlignment(Gui::Alignment::ALIGN_CENTER);
	Gui::Screen::PopFont();
	Add(m_hudLocationPrimary, Gui::Screen::GetWidth() / 2.0f, 2.0f);
	Add(m_hudLocationSecondary, Gui::Screen::GetWidth() / 2.0f, 14.0f);

	// Player guages
	Gui::Screen::PushFont("HudFont");
	m_hudPlayerShip = (new Gui::Label(Pi::player->GetLabel().c_str()))->Color(s_hudTextColor);
	Gui::Screen::PopFont();
	m_hudFuelGauge = new Gui::MeterBar(120.f, Lang::P_UI_FUEL, Color::PARAGON_GREEN);
	m_hudHydrogenGauge = new Gui::MeterBar(120.0f, Lang::P_UI_HYDROGEN, Color::PARAGON_GREEN);
	m_hudHullTemp = new Gui::MeterBar(120.0f, Lang::P_UI_TEMP, Color::PARAGON_GREEN);
	m_hudWeaponTemp = new Gui::MeterBar(120.0f, Lang::P_UI_WEAP, Color::PARAGON_GREEN);
	m_hudHullIntegrity = new Gui::MeterBar(120.0f, Lang::P_UI_HULL, Color::PARAGON_GREEN);
	m_hudShieldIntegrity = new Gui::MeterBar(120.0f, Lang::P_UI_SHLD, Color::PARAGON_GREEN);

	cursor = 8.0f;
	Add(m_hudPlayerShip, 10.0f, cursor); cursor += cursor_incr;
	//Add(m_hudFuelGauge, 5.0f, cursor); cursor += cursor_incr;
	Add(m_hudHydrogenGauge, 5.0f, cursor); cursor += cursor_incr;
	Add(m_hudHullTemp, 5.0f, cursor); cursor += cursor_incr;
	Add(m_hudWeaponTemp, 5.0f, cursor); cursor += cursor_incr;
	Add(m_hudHullIntegrity, 5.0f, cursor); cursor += cursor_incr;
	Add(m_hudShieldIntegrity, 5.0f, cursor); cursor += cursor_incr;

	m_hudShieldIntegrity->Hide();

	// Target name and gauges
	Gui::Screen::PushFont("HudFont");
	m_hudTargetShip = (new Gui::Label(""))->Color(Color::PARAGON_TARGET);
	m_hudTargetDesc = (new Gui::Label(""))->Color(Color::PARAGON_TARGET);
	m_hudTargetCargo = (new Gui::Label(""))->Color(Color::PARAGON_TARGET);
	Gui::Screen::PopFont();
	m_hudTargetShip->SetAlignment(Gui::Alignment::ALIGN_RIGHT);
	m_hudTargetDesc->SetAlignment(Gui::Alignment::ALIGN_RIGHT);
	m_hudTargetCargo->SetAlignment(Gui::Alignment::ALIGN_RIGHT);

	m_hudTargetHullIntegrity = new Gui::MeterBar(120.0f, Lang::P_UI_HULL, Color::PARAGON_TARGET,
		Gui::MeterBar::MeterBarAlign::METERBAR_RIGHT, Color::PARAGON_TARGET);
	m_hudTargetShieldIntegrity = new Gui::MeterBar(120.0f, Lang::P_UI_SHLD, Color::PARAGON_TARGET,
		Gui::MeterBar::MeterBarAlign::METERBAR_RIGHT, Color::PARAGON_TARGET);
	m_hudTargetFuel = new Gui::MeterBar(120.0f, Lang::P_UI_FUEL, Color::PARAGON_TARGET,
		Gui::MeterBar::MeterBarAlign::METERBAR_RIGHT, Color::PARAGON_TARGET);

	cursor = 8.0f;
	float target_x = 0.0f;

	Add(m_hudTargetShip, Gui::Screen::GetWidth() - 150.0f, cursor); cursor += cursor_incr;
	m_hudTargetShip->SetRightMargin(140.0f);
	Add(m_hudTargetDesc, Gui::Screen::GetWidth() - 150.0f, cursor); cursor += cursor_incr;
	m_hudTargetDesc->SetRightMargin(140.0f);
	Add(m_hudTargetCargo, Gui::Screen::GetWidth() - 150.0f, cursor); cursor += cursor_incr;
	m_hudTargetCargo->SetRightMargin(140.0f);

	target_x = Gui::Screen::GetWidth() - 125.0f;
	Add(m_hudTargetHullIntegrity, target_x, cursor); cursor += cursor_incr;
	Add(m_hudTargetShieldIntegrity, target_x, cursor); cursor += cursor_incr;
	Add(m_hudTargetFuel, target_x, cursor); cursor += cursor_incr + 5.0f;

	m_hudTargetShip->Hide();
	m_hudTargetDesc->Hide();
	m_hudTargetCargo->Hide();
	m_hudTargetHullIntegrity->Hide();
	m_hudTargetShieldIntegrity->Hide();
	m_hudTargetFuel->Hide();

	Gui::Screen::PushFont("HudFont");

	for (unsigned i = 0; i < 5; ++i) {
		Gui::Label* lbl = (new Gui::Label(""))->Color(Color::PARAGON_TARGET);
		lbl->SetAlignment(Gui::Alignment::ALIGN_RIGHT);
		m_vHudTargetInfo.push_back(lbl);
		Add(lbl, target_x, cursor + (cursor_incr * i));
	}

	m_bodyLabels = new Gui::LabelSet();
	m_bodyLabels->SetLabelColor(Color::PARAGON_BLUE);
	Add(m_bodyLabels, 0, 0);

	{
		m_pauseText = new Gui::Label(Lang::PAUSED);
		m_pauseText->Color(Color::PARAGON_GREEN);
		float w, h;
		Gui::Screen::MeasureString(Lang::PAUSED, w, h);
		Add(m_pauseText, 0.5f * (Gui::Screen::GetWidth() - w), 100);
	}

	m_navTargetIndicator.label = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_navTargetIndicator.label2 = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_navTargetIndicator.label->SetAlignment(Gui::ALIGN_CENTER);
	m_navTargetIndicator.label2->SetAlignment(Gui::ALIGN_CENTER);
	m_navVelIndicator.label = (new Gui::Label(""))->Color(Color::PARAGON_GREEN);
	m_navVelIndicator.label->SetAlignment(Gui::ALIGN_CENTER);
	m_combatTargetIndicator.label = new Gui::Label(""); // colour set dynamically
	m_combatTargetIndicator.label->SetAlignment(Gui::ALIGN_CENTER);

	Gui::Screen::PopFont();

	// these labels are repositioned during Draw3D()
	Add(m_navTargetIndicator.label, 0, 0);
	Add(m_navTargetIndicator.label2, 0, 0);
	Add(m_navVelIndicator.label, 0, 0);
	Add(m_combatTargetIndicator.label, 0, 0);

	// XXX m_renderer not set yet
	Graphics::TextureBuilder b = Graphics::TextureBuilder::UI("icons/indicator_mousedir.png");
	m_indicatorMousedir.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));

	const Graphics::TextureDescriptor &descriptor = b.GetDescriptor();
	m_indicatorMousedirSize = vector2f(descriptor.dataSize.x*descriptor.texSize.x,descriptor.dataSize.y*descriptor.texSize.y);

	// Reticle
	b = Graphics::TextureBuilder::UI("icons/reticle.png");
	m_reticle.reset(new Gui::TexturedQuad(renderer,
		b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/reticle_target.png");
	m_reticleTarget.reset(new Gui::TexturedQuad(renderer,
		b.GetOrCreateTexture(renderer, "ui")));

	// Speedlines

    m_speedLines.reset(new SpeedLines(Pi::player));

	//get near & far clipping distances
	//XXX m_renderer not set yet
	float znear;
	float zfar;
	Pi::renderer->GetNearFarRange(znear, zfar);

	const float fovY = Pi::config->Float("FOVVertical");

	m_cameraContext.Reset(new CameraContext(Graphics::GetScreenWidth(), Graphics::GetScreenHeight(), fovY, znear, zfar));
	m_camera.reset(new Camera(m_cameraContext, Pi::renderer));
	m_internalCameraController.reset(new InternalCameraController(m_cameraContext, Pi::player));
	m_externalCameraController.reset(new ExternalCameraController(m_cameraContext, Pi::player));
	m_siderealCameraController.reset(new SiderealCameraController(m_cameraContext, Pi::player));
	SetCamType(m_camType); //set the active camera

	DefineTweaks();

	m_onHyperspaceTargetChangedCon =
		Pi::sectorView->onHyperspaceTargetChanged.connect(sigc::mem_fun(this, &WorldView::OnHyperspaceTargetChanged));

	m_onPlayerChangeTargetCon =
		Pi::onPlayerChangeTarget.connect(sigc::mem_fun(this, &WorldView::OnPlayerChangeTarget));
	m_onChangeFlightControlStateCon =
		Pi::onPlayerChangeFlightControlState.connect(sigc::mem_fun(this, &WorldView::OnPlayerChangeFlightControlState));
	m_onMouseWheelCon =
		Pi::onMouseWheel.connect(sigc::mem_fun(this, &WorldView::MouseWheel));

	Pi::player->GetPlayerController()->SetMouseForRearView(GetCamType() == CAM_INTERNAL && m_internalCameraController->GetMode() == InternalCameraController::MODE_REAR);
	KeyBindings::toggleHudMode.onPress.connect(sigc::mem_fun(this, &WorldView::OnToggleLabels));

	// Hud2 Icons
	b = Graphics::TextureBuilder::UI("icons/hud2/planet.png");
	m_hud2Planet.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/settlement.png");
	m_hud2Settlement.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/ship.png");
	m_hud2Ship.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/ship_indicator.png");
	m_hud2ShipIndicator.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/ship_offscreen_indicator.png");
	m_hud2ShipOffscreen.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/star.png");
	m_hud2Star.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/station.png");
	m_hud2Station.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/wake.png");
	m_hud2HyperspaceCloud.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/target_offscreen_selector.png");
	m_hud2TargetOffscreen.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/target_selector.png");
	m_hud2TargetSelector.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/hud2/unknown.png");
	m_hud2Unknown.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/missions/current_mission_up.png");
	m_hud2CurrentMission1.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));
	b = Graphics::TextureBuilder::UI("icons/missions/current_mission_down.png");
	m_hud2CurrentMission2.reset(new Gui::TexturedQuad(renderer, b.GetOrCreateTexture(renderer, "ui")));

	// Tutorial	
	if(!m_tutorialSeen) {
		const float dw = 500.0f, dh = 350.0f;
		const float pw = 5.0f, ph = 5.0f;
		Gui::Screen::PushFont("OverlayFont");

		m_tutorialDialog = new Gui::Fixed();
		m_tutorialDialog->SetTransparency(false);
		m_tutorialDialog->SetBgColor(Gui::Theme::Colors::bg);
		m_tutorialDialog->SetSizeRequest(dw, dh);
		m_tutorialDialog->SetSize(dw, dh);

		auto box = new Gui::Fixed;
		box->SetTransparency(false);
		box->SetBgColor(Color(32, 50, 80, 128));
		auto paragraph = new Gui::VBox();
		paragraph->SetPadding(pw);
		box->Add(paragraph, 0.0f, 0.0f);

		auto scrollbox = new Gui::HBox();
		scrollbox->SetSpacing(5.0f);
		scrollbox->SetSizeRequest(dw - (pw * 2), dh - (ph * 2) - 35.0f);
		m_tutorialDialog->Add(scrollbox, pw, ph);

		auto cl = (new Gui::Label("Got it!"))->Color(Color::WHITE);
		auto cb = new Gui::LabelButton(cl);
		cb->SetPadding(50.0f, 10.0f);
		m_tutorialDialog->Add(cb, (dw - 140) / 2.0f, dh - 36.0f);
		cb->onClick.connect(sigc::mem_fun(this, &WorldView::OnClickTutorial));

		auto scrollbar = new Gui::VScrollBar();
		auto scrollportal = new Gui::VScrollPortal(dw - 25.0f - pw);
		scrollbar->SetAdjustment(&scrollportal->vscrollAdjust);

		auto t1 = (new Gui::Label(Lang::TUTORIAL_1))->Color(Color::WHITE);
		auto t2 = (new Gui::Label(Lang::TUTORIAL_2))->Color(Color::WHITE);
		auto t3 = (new Gui::Label(Lang::TUTORIAL_3))->Color(Color::WHITE);
		auto t4 = (new Gui::Label(Lang::TUTORIAL_4))->Color(Color::WHITE);
		auto t5 = (new Gui::Label(Lang::TUTORIAL_5))->Color(Color::WHITE);
		auto t6 = (new Gui::Label(Lang::TUTORIAL_6))->Color(Color::WHITE);

		paragraph->PackStart(t6);
		paragraph->PackStart(t5);
		paragraph->PackStart(t4);
		paragraph->PackStart(t3);
		paragraph->PackStart(t2);
		paragraph->PackStart(t1);

		scrollportal->Add(box);
		scrollbox->PackStart(scrollbar);
		scrollbox->PackStart(scrollportal);

		Add(m_tutorialDialog, 
			(Gui::Screen::GetWidth() - dw) / 2.0f, (Gui::Screen::GetHeight() - dh) / 2.0f);
		Gui::Screen::PopFont();

		if(Pi::cpan) {
			Pi::cpan->SetCircleOverlayVisiblity(false);
		}
	} else {
		m_tutorialDialog = nullptr;
	}

	// Trail depth effect
	m_renderer = Pi::renderer;
	Graphics::MaterialDescriptor ttd;
	ttd.effect = Graphics::EffectType::EFFECT_THRUSTERTRAILS_DEPTH;
	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor desc;
		desc.uniforms.push_back("su_ModelViewProjectionMatrix");
		desc.uniforms.push_back("invLogZfarPlus1");
		desc.vertex_shader = "gl3/thruster_trails/depth.vert";
		desc.fragment_shader = "gl3/thruster_trails/depth.frag";
		m_trailDepthMtrl.reset(new Graphics::GL3::EffectMaterial(m_renderer, desc));
	} else {
		m_trailDepthMtrl.reset(m_renderer->CreateMaterial(ttd));
	}
	// Trail depth render target
	Graphics::RenderTargetDesc rtd(
		m_renderer->GetWindow()->GetWidth(),
		m_renderer->GetWindow()->GetHeight(),
		Graphics::TextureFormat::TEXTURE_RGBA_8888, Graphics::TextureFormat::TEXTURE_NONE,
		false);
	m_trailDepthRT.reset(m_renderer->CreateRenderTarget(rtd));
	// Trail material
	ttd.effect = Graphics::EffectType::EFFECT_THRUSTERTRAILS;
	if(Graphics::Hardware::GL3()) {
		Graphics::GL3::EffectDescriptor desc;
		desc.uniforms.push_back("su_ModelViewProjectionMatrix");
		desc.uniforms.push_back("invLogZfarPlus1");
		desc.uniforms.push_back("texture0");
		desc.uniforms.push_back("texture1");
		desc.uniforms.push_back("u_windowSize");
		desc.vertex_shader = "gl3/thruster_trails/draw.vert";
		desc.fragment_shader = "gl3/thruster_trails/draw.frag";
		m_trailMtrl.reset(new Graphics::GL3::EffectMaterial(m_renderer, desc));
		m_windowSizeUniformId = m_trailMtrl->GetEffect()->GetUniformID("u_windowSize");
	} else {
		m_trailMtrl.reset(m_renderer->CreateMaterial(ttd));
	}
	// Trail gradient texture
	m_trailGradient = Graphics::TextureBuilder::UI("textures/exhaust_gradient.png").GetOrCreateTexture(m_renderer, "effect");

    m_contactListContainer = new Gui::VBox();
	Add(m_contactListContainer, size[0] - 110, size[1] - 15);
    m_contactListButton = new Gui::ClickableLabel(new Gui::Label("test"));
    m_contactListButton->onClick.connect(sigc::mem_fun(this, &WorldView::ToggleContactListSize));
    m_contactListContainer->PackStart(m_contactListButton);
    UpdateContactList();

	DefineTweaks();
}

WorldView::~WorldView()
{
	RemoveTweaks();
	m_onHyperspaceTargetChangedCon.disconnect();
	m_onPlayerChangeTargetCon.disconnect();
	m_onChangeFlightControlStateCon.disconnect();
	m_onMouseWheelCon.disconnect();
}

void WorldView::Save(Serializer::Writer &wr)
{
	wr.Bool(m_tutorialSeen);
	wr.Int32(int(m_camType));
	m_internalCameraController->Save(wr);
	m_externalCameraController->Save(wr);
	m_siderealCameraController->Save(wr);
}

void WorldView::DefineTweaks()
{
	// Define any tweaks specific to worldview
	if(!Tweaker::IsTweakDefined("PermaCloud")) {
		std::vector<STweakValue> vtv = {
			STweakValue(ETWEAK_FLOAT, "Size", &Pi::ts_perma_cloud.size, &Pi::ts_perma_cloud_default.size, 
				"min=0.25 max=10.0 step=0.1"),
			STweakValue(ETWEAK_COLOR, "Color", &Pi::ts_perma_cloud.color.r, &Pi::ts_perma_cloud_default.color.r, 
				"opened=true"),
		};
		Tweaker::DefineTweak("PermaCloud", vtv);
	}
}

const TS_HypercloudVisual& WorldView::GetHyperCloudTweak() const
{
	return Pi::ts_perma_cloud;
}

void WorldView::RemoveTweaks()
{
	// Remove any tweaks specific to world view (using Tweaker::RemoveTweak)
	Tweaker::RemoveTweak("PermaCloud");
}

void WorldView::SetCamType(enum CamType c)
{
	Pi::BoinkNoise();

	// don't allow external cameras when docked inside space stations.
	// they would clip through the station model
	if (Pi::player->GetFlightState() == Ship::DOCKED && !Pi::player->GetDockedWith()->IsGroundStation())
		c = CAM_INTERNAL;

	m_camType = c;

	switch(m_camType) {
		case CAM_INTERNAL:
			m_activeCameraController = m_internalCameraController.get();
			Pi::player->OnCockpitActivated();
			break;
		case CAM_EXTERNAL:
			m_activeCameraController = m_externalCameraController.get();
			break;
		case CAM_SIDEREAL:
			m_activeCameraController = m_siderealCameraController.get();
			break;
	}
	Pi::cpan->ChangeCamButtonState(m_camType);

	Pi::player->GetPlayerController()->SetMouseForRearView(m_camType == CAM_INTERNAL && m_internalCameraController->GetMode() == InternalCameraController::MODE_REAR);

	m_activeCameraController->Reset();

	onChangeCamType.emit();
}

void WorldView::ChangeInternalCameraMode(InternalCameraController::Mode m)
{
	if (m_internalCameraController->GetMode() == m) return;

	Pi::BoinkNoise();
	m_internalCameraController->SetMode(m);
	Pi::player->GetPlayerController()->SetMouseForRearView(m_camType == CAM_INTERNAL && m_internalCameraController->GetMode() == InternalCameraController::MODE_REAR);
}

void WorldView::OnClickAutopilotButton(Gui::MultiStateImageButton *b)
{
	int newState = b->GetState();
	Pi::BoinkNoise();

	if(newState == FLIGHT_BUTTON_UNAVAILABLE) {		// Autopilot is turned off/disabled
		// Turned off when there are no targets
		newState = FLIGHT_BUTTON_OFF;
		m_flightAutopilotButton->SetEnabled(false);
		Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_MANEUVER);
	} else if(newState == FLIGHT_BUTTON_ON) {		// Autopilot is turned on
		// Do I need to find out if a command is issued?
		ToggleTargetActions();
		/*Body* const navtarget = Pi::player->GetNavTarget();
		Body* const comtarget = Pi::player->GetCombatTarget();
		if(navtarget || comtarget) {
			b->SetActiveState(newState);
			m_flightAutopilotButton->SetEnabled(true);
			//autopilot_flyto(navtarget? navtarget : comtarget);
			Pi::SetView(Pi::worldView);
			ToggleTargetActions();
		} else {
			assert(0); // The autopilot button should've been disabled, no targets are found
		}*/
	} else {
		// previous state was FLIGHT_BUTTON_UNAVAILABLE before click, which means autopilot button should have been disabled
		assert(0);
	}
}

void WorldView::OnClickManeuverButton(Gui::MultiStateImageButton *b)
{
	int newState = b->GetState();
	Pi::BoinkNoise();
	switch(static_cast<FlightButtonStatus>(newState)) {	
		case FLIGHT_BUTTON_UNAVAILABLE:
		case FLIGHT_BUTTON_OFF:
			newState = FLIGHT_BUTTON_OFF;
			Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_MANEUVER);
			break;
		
		case FLIGHT_BUTTON_ON:
			Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_MANEUVER);
			break;
	}
	b->SetActiveState(newState);
}

void WorldView::OnClickTransitButton(Gui::MultiStateImageButton *b)
{
	int newState = b->GetState();
	Pi::BoinkNoise();
	switch(static_cast<FlightButtonStatus>(newState)) {	
		case FLIGHT_BUTTON_UNAVAILABLE:
		case FLIGHT_BUTTON_OFF:
			newState = FLIGHT_BUTTON_OFF;
			Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_MANEUVER);
			break;
		
		case FLIGHT_BUTTON_ON:
			Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_TRANSIT);
			break;
	}
	b->SetActiveState(newState);
}

void WorldView::OnClickJumpButton(Gui::MultiStateImageButton *b)
{
	if (Pi::player->IsHyperspaceActive()) {
		// Hyperspace countdown in effect.. abort!
		Pi::player->ResetHyperspaceCountdown();
		//Pi::cpan->MsgLog()->Message("", Lang::HYPERSPACE_JUMP_ABORTED);
		Pi::game->log->Add("", Lang::HYPERSPACE_JUMP_ABORTED);
		m_flightJumpButton->SetActiveState(FLIGHT_BUTTON_OFF);
	} else {
		// Initiate hyperspace drive
		SystemPath path = Pi::sectorView->GetHyperspaceTarget();
		Pi::player->StartHyperspaceCountdown(path, Pi::player->IsPhaseJumpMode());
		m_flightJumpButton->SetActiveState(FLIGHT_BUTTON_ON);
	}
}

void WorldView::ShowParagonFlightButtons()
{
	m_flightAutopilotButton->Show();
	m_flightManeuverButton->Show();
	m_flightTransitButton->Show();
	m_flightJumpButton->Show();
}

void WorldView::HideParagonFlightButtons()
{
	m_flightAutopilotButton->Hide();
	m_flightManeuverButton->Hide();
	m_flightTransitButton->Hide();
	m_flightJumpButton->Hide();
}

/* This is when the flight control state actually changes... */
void WorldView::OnPlayerChangeFlightControlState()
{
	
}

void WorldView::OnClickBlastoff()
{
	Pi::BoinkNoise();
	if (Pi::player->GetFlightState() == Ship::DOCKED) {
		if (!Pi::player->Undock()) {
			//Pi::cpan->MsgLog()->ImportantMessage(
			Pi::game->log->Add(
				Pi::player->GetDockedWith()->GetLabel(), Lang::LAUNCH_PERMISSION_DENIED_BUSY);
		}
	} else {
		Pi::player->Blastoff();
	}
}

void WorldView::OnClickTutorial()
{
	Pi::BoinkNoise();
	m_tutorialSeen = true;
	Remove(m_tutorialDialog);
	m_tutorialDialog = nullptr;
	if(Pi::cpan) { 
		Pi::cpan->SetCircleOverlayVisiblity(true);
	}
}

void WorldView::OnClickHyperspace()
{
	if (Pi::player->IsHyperspaceActive()) {
		// Hyperspace countdown in effect.. abort!
		Pi::player->ResetHyperspaceCountdown();
		//Pi::cpan->MsgLog()->Message(
		Pi::game->log->Add(
			"", Lang::HYPERSPACE_JUMP_ABORTED);
	} else {
		// Initiate hyperspace drive
		SystemPath path = Pi::sectorView->GetHyperspaceTarget();
		Pi::player->StartHyperspaceCountdown(path, Pi::player->IsPhaseJumpMode());
	}
}

void WorldView::Draw3D()
{
	PROFILE_SCOPED()
	assert(Pi::game);
	assert(Pi::player);
	assert(!Pi::player->IsDead());

	m_cameraContext->ApplyDrawTransforms(m_renderer);

	Body* excludeBody = nullptr;
	ShipCockpit* cockpit = nullptr;
	if(GetCamType() == CAM_INTERNAL) {
		excludeBody = Pi::player;
		if (m_internalCameraController->GetMode() == InternalCameraController::MODE_FRONT)
			cockpit = Pi::player->GetCockpit();
	}
	m_camera->BeginDraw(excludeBody);

	// Speed lines
	if (Pi::AreSpeedLinesDisplayed())
		m_speedLines->Render(m_renderer);
		
	DrawThrusterTrails();

	m_camera->EndDraw(m_cameraContext->GetCamFrame(), cockpit);
	
	m_cameraContext->EndFrame();
	UIView::Draw3D();
}

#include "graphics/effects/thruster_trails/ThrusterTrailsMaterial.h"

void WorldView::DrawThrusterTrails()
{
	// Thruster trails for ships and player
	if (Pi::game->IsHyperspace()) {
		return;
	}

	Graphics::RenderTarget *main_rt = m_renderer->GetActiveRenderTarget();
	
	// Thruster trails depth RT + material
	m_renderer->SetRenderTarget(m_trailDepthRT.get());
	m_renderer->ClearScreen();
	// Depth pass
	for(auto it = Pi::player->GetSensors()->GetContacts().begin(); 
		it != Pi::player->GetSensors()->GetContacts().end(); ++it) 
	{
		for(unsigned i = 0; i < it->ship->GetThrusterTrailsNum(); ++i) {
			Ship* ship = it->ship;
			if(ship->IsVisible()) {
				ThrusterTrail* tt = it->ship->GetThrusterTrail(i);
				if (tt) {
					tt->Render(m_trailDepthMtrl.get());
				}
			}
		}
	}
	for(unsigned int i = 0; i < Pi::player->GetThrusterTrailsNum(); ++i) {
		Pi::player->GetThrusterTrail(i)->Render(m_trailDepthMtrl.get());
	}
	
	m_renderer->SetRenderTarget(main_rt);

	m_trailMtrl->texture0 = m_trailGradient;
	m_trailMtrl->texture1 = m_trailDepthRT.get()->GetColorTexture();
	if(Graphics::Hardware::GL3()) {
		m_trailMtrl->GetEffect()->SetProgram();
		m_trailMtrl->GetEffect()->GetUniform(m_windowSizeUniformId).Set(vector3f(m_renderer->GetWindow()->GetWidth(),
			m_renderer->GetWindow()->GetHeight(), 0.0f));
	} else {
		reinterpret_cast<Graphics::Effects::ThrusterTrailsMaterial*>(
			m_trailMtrl.get())->setWindowSize(m_renderer->GetWindow()->GetWidth(), 
			m_renderer->GetWindow()->GetHeight());
	}

	// Thruster trails for ships and player
	// Diffuse pass
	for(auto it = Pi::player->GetSensors()->GetContacts().begin(); it != Pi::player->GetSensors()->GetContacts().end(); ++it) {
		for(unsigned i = 0; i < it->ship->GetThrusterTrailsNum(); ++i) {
			Ship* ship = it->ship;
			if(ship->IsVisible()) {
				ThrusterTrail* tt = it->ship->GetThrusterTrail(i);
				if (tt) {
					it->ship->GetThrusterTrail(i)->Render(m_trailMtrl.get());
				}
			}
		}
	}
	for(unsigned int i = 0; i < Pi::player->GetThrusterTrailsNum(); ++i) {
		Pi::player->GetThrusterTrail(i)->Render(m_trailMtrl.get());
	}
}

void WorldView::OnToggleLabels()
{
	if (Pi::GetView() == this) {
		if (Pi::DrawGUI && m_labelsOn) {
			m_labelsOn = false;
		} else if (Pi::DrawGUI && !m_labelsOn) {
			Pi::DrawGUI = false;
		} else if (!Pi::DrawGUI) {
			Pi::DrawGUI = true;
			m_labelsOn = true;
		}
	}
}

void WorldView::ShowAll()
{
	View::ShowAll(); // by default, just delegate back to View
	RefreshButtonStateAndVisibility();
}

static Color get_color_for_warning_meter_bar(float v) {
	Color c;
	if (v < 50.0f)
		c = Color(255,0,0,HUD_ALPHA);
	else if (v < 75.0f)
		c = Color(255,128,0,HUD_ALPHA);
	else
		c = Color(255,255,0,HUD_ALPHA);
	return c;
}

void WorldView::RefreshHyperspaceButton() {
	bool phase_jump_mode = Pi::player->IsPhaseJumpMode(true);
	bool phase_jump_range = Pi::player->IsPhaseJumpRange(true);

	if (Pi::player->IsTransitPossible() && phase_jump_mode == phase_jump_range) {
		if (Pi::player->CanHyperspaceTo(Pi::sectorView->GetHyperspaceTarget(), phase_jump_mode)) {
			m_hyperspaceButton->Show();

			int on_state = !phase_jump_mode ? FLIGHT_BUTTON_ON : FLIGHT_PHASE_BUTTON_ON;
			int off_state = !phase_jump_mode ? FLIGHT_BUTTON_OFF : FLIGHT_PHASE_BUTTON_OFF;

			if(Pi::player->IsHyperspaceActive()) {
				m_flightJumpButton->SetActiveState(on_state);
			} else {
				m_flightJumpButton->SetActiveState(off_state);
			}
			m_flightJumpButton->SetEnabled(true);
			return;
		}
	}

	m_hyperspaceButton->Hide();
	int unav_btn = !phase_jump_range ? FLIGHT_BUTTON_UNAVAILABLE : FLIGHT_PHASE_BUTTON_UNAVAILABLE;
	m_flightJumpButton->SetActiveState(unav_btn);
	m_flightJumpButton->SetEnabled(false);
}

void WorldView::RefreshFreightTeleporterState() {
	std::ostringstream ss;
	int teleporter_level = Pi::player->GetFreightTeleporterLevel();
	if (teleporter_level > 0 && Pi::player->GetFlightState() != Ship::FlightState::HYPERSPACE) {
		ss << "FT" << teleporter_level << " ";
		EFreightTeleporterStatus teleporter_status = Pi::player->GetFreightTeleporterStatus();
		EFreightTeleporterTargetType teleporter_target_type = Pi::player->GetFreightTeleporterTargetType();
		Body* ft_tgt = Pi::player->GetFreightTeleporterTarget();
		if(ft_tgt) {
			if(ft_tgt->GetType() == Body::Type::SHIP) {
				ss << "SHIP TGT";
			} else if(ft_tgt->GetType() == Body::Type::CARGOBODY) {
				ss << "CARGO TGT";
			}
		} else {
			switch(teleporter_status) {
				case EFT_S_NOT_AVAILABLE:
					ss << "Not Available";
					break;
				case EFT_S_NO_TGT:
					ss << "No Target";
					break;
				case EFT_S_FREIGHT_FULL:
					ss << "Freight Full";
					break;
				case EFT_S_TGT_OUT_OF_RANGE:
					ss << "Out of Range";
					break;
				case EFT_S_TGT_SHIELDED:
					ss << "Shielded";
					break;
				default:
					ss << "Inactive";
			}
		}
	}
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_HUD2_LEFT_5, ss.str().c_str());
}

void WorldView::RefreshOverlayHud() {
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_HUD2_LEFT, m_overlayBuffer[0]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_HUD2_LEFT_2, m_overlayBuffer[1]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_HUD2_LEFT_3, m_overlayBuffer[2]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_HUD2_LEFT_4, m_overlayBuffer[3]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_TOP_1, "");
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_TOP_2, "");
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_LEFT_1, m_circleTitleBuffer[0]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_LEFT_2, m_circleDataBuffer[0]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_RIGHT_1, m_circleTitleBuffer[1]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_RIGHT_2, m_circleDataBuffer[1]);
	//Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_BOTTOM_1, m_circleTitleBuffer[2]);
	//Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_CIRCLE_BOTTOM_2, m_circleDataBuffer[2]);
	Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_DEV, m_devBuffer);
}

void WorldView::RefreshButtonStateAndVisibility()
{
	assert(Pi::game);
	assert(Pi::player);
	assert(!Pi::player->IsDead());

	Pi::cpan->ClearOverlay();

	if (Pi::game->IsPaused()) {
		m_pauseText->Show();
	} else {
		m_pauseText->Hide();
	}

	if (Pi::player->GetFlightState() != Ship::HYPERSPACE) {
		Pi::cpan->SetOverlayToolTip(ShipCpanel::OVERLAY_HUD2_LEFT,     Lang::SHIP_VELOCITY_BY_REFERENCE_OBJECT);
		Pi::cpan->SetOverlayToolTip(ShipCpanel::OVERLAY_HUD2_LEFT_3,    Lang::DISTANCE_FROM_SHIP_TO_NAV_TARGET);
		Pi::cpan->SetOverlayToolTip(ShipCpanel::OVERLAY_HUD2_LEFT_2,  Lang::EXTERNAL_ATMOSPHERIC_PRESSURE);
		Pi::cpan->SetOverlayToolTip(ShipCpanel::OVERLAY_HUD2_LEFT_4, Lang::SHIP_ALTITUDE_ABOVE_TERRAIN);
	}

	RefreshHyperspaceButton();
	RefreshFreightTeleporterState();
	RefreshLocationInfo();
	RefreshOverlayHud();

	bool is_outside_gravity_bubble = false;
	bool is_outside_gravity_bubble_2 = false;
	if(!m_bAltitudeAvailable && Pi::player->GetFlightState() != Ship::HYPERSPACE) {
		// Check for space station bubble
		const Frame* player_frame = Pi::player->GetFrame();
		auto sbody_type = player_frame->GetSystemBody() ? player_frame->GetSystemBody()->GetSuperType() :
			SystemBody::BodySuperType::SUPERTYPE_NONE;
		if(	player_frame && 
			(player_frame->GetBody() && player_frame->GetBody()->IsType(Object::SPACESTATION) ||
			sbody_type == SystemBody::BodySuperType::SUPERTYPE_HYPERSPACE_CLOUD))
		{
			double distance = (Pi::player->GetFrame()->GetBody()->GetPosition() - Pi::player->GetPosition()).Length();
			if(distance <= TRANSIT_GRAVITY_RANGE_1) {
				is_outside_gravity_bubble = false;
			} else {
				is_outside_gravity_bubble = true;
			}
			if(distance <= TRANSIT_GRAVITY_RANGE_2) {
				is_outside_gravity_bubble_2 = false;
			} else {
				is_outside_gravity_bubble_2 = true;
			}
		} else {
			is_outside_gravity_bubble = true;
			is_outside_gravity_bubble_2 = true;
		}
	} else if(m_altitude > TRANSIT_GRAVITY_RANGE_1) {
		is_outside_gravity_bubble = true;
		if(m_altitude > TRANSIT_GRAVITY_RANGE_2) {
			is_outside_gravity_bubble_2 = true;
		}
	}

	switch(Pi::player->GetFlightState()) {
		case Ship::LANDED:
			m_flightStatus->SetText(Lang::LANDED);
			m_launchButton->Show();
			HideParagonFlightButtons();
			break;

		case Ship::DOCKING:
			m_flightStatus->SetText(Lang::DOCKING);
			m_launchButton->Hide();
			HideParagonFlightButtons();
			break;

		case Ship::DOCKED:
			m_flightStatus->SetText(Lang::DOCKED);
			m_launchButton->Show();
			HideParagonFlightButtons();
			break;

		case Ship::JUMPING:
		case Ship::HYPERSPACE:
			m_flightStatus->SetText(Lang::HYPERSPACE);
			m_launchButton->Hide();
			HideParagonFlightButtons();
			break;

		case Ship::FLYING:
		default:
			const FlightControlState fstate = Pi::player->GetPlayerController()->GetFlightControlState();
			Body * const navtarget = Pi::player->GetNavTarget();
			Body * const comtarget = Pi::player->GetCombatTarget();
			switch (fstate) {
				case CONTROL_MANEUVER: {
					std::string msg;
					const double setspeed = Pi::player->GetController()->GetSpeedLimit();
					if (setspeed > 1000 || setspeed < -1000) {
						msg = stringf(Lang::SET_SPEED_KM_S, formatarg("speed", setspeed*0.001));
					} else {
						msg = stringf(Lang::SET_SPEED_M_S, formatarg("speed", setspeed));
					}
					m_flightStatus->SetText(msg);
					m_flightManeuverButton->SetEnabled(true);
					m_flightManeuverButton->SetActiveState(FLIGHT_BUTTON_ON);

					m_flightAutopilotButton->SetActiveState(FLIGHT_BUTTON_OFF);
					m_flightAutopilotButton->SetEnabled(true);

					// If it's outside the gravity bubble, enable transit button
					if(is_outside_gravity_bubble) {
						m_flightTransitButton->SetEnabled(true);
						m_flightTransitButton->SetActiveState(FLIGHT_BUTTON_OFF);
					} else {
						m_flightTransitButton->SetEnabled(false);
						m_flightTransitButton->SetActiveState(FLIGHT_BUTTON_UNAVAILABLE);
					}
					break;
				}

				case CONTROL_TRANSIT: {
					std::string msg = "TRANSIT DRIVE ";
					if(is_outside_gravity_bubble_2) {
						msg.append("2");
					} else {
						msg.append("1");
					}
					m_flightStatus->SetText(msg);
					m_flightTransitButton->SetEnabled(true);
					m_flightTransitButton->SetActiveState(FLIGHT_BUTTON_ON);

					m_flightAutopilotButton->SetActiveState(FLIGHT_BUTTON_OFF);
					m_flightAutopilotButton->SetEnabled(true);

					m_flightManeuverButton->SetEnabled(true);
					m_flightManeuverButton->SetActiveState(FLIGHT_BUTTON_OFF);
					break;
				}

				case CONTROL_AUTOPILOT:
					m_flightStatus->SetText(Lang::AUTOPILOT);
					m_flightAutopilotButton->SetEnabled(true);
					m_flightManeuverButton->SetEnabled(true);
					m_flightTransitButton->SetEnabled(false);
					m_flightAutopilotButton->SetActiveState(FLIGHT_BUTTON_ON);
					m_flightManeuverButton->SetActiveState(FLIGHT_BUTTON_OFF);
					m_flightTransitButton->SetActiveState(FLIGHT_BUTTON_UNAVAILABLE);
					break;

				default: assert(0); break;
			}

			m_launchButton->Hide();		
			ShowParagonFlightButtons();
	}

	// Direction indicator
	vector3d vel = Pi::player->GetVelocity();

	if (m_showTargetActionsTimeout) {
		if (SDL_GetTicks() - m_showTargetActionsTimeout > 20000) {
			m_showTargetActionsTimeout = 0;
			m_commsOptions->DeleteAllChildren();
			m_commsNavOptions->DeleteAllChildren();
			m_commsOptions->Hide();
			m_commsNavOptionsContainer->Hide();
		}
		else {
			m_commsOptions->ShowAll();
			m_commsNavOptionsContainer->ShowAll();
		}
	} else {
		m_commsOptions->Hide();
		m_commsNavOptionsContainer->Hide();
	}

	if (m_showLowThrustPowerTimeout) {
		if (SDL_GetTicks() - m_showLowThrustPowerTimeout > 20000) {
			m_showLowThrustPowerTimeout = 0;
		}
		m_lowThrustPowerOptions->Show();
	} else {
		m_lowThrustPowerOptions->Hide();
	}
#if WITH_DEVKEYS
	if (Pi::showDebugInfo) {
		std::ostringstream ss;

		if (Pi::player->GetFlightState() != Ship::HYPERSPACE) {
			vector3d pos = Pi::player->GetPosition();
			vector3d abs_pos = Pi::player->GetPositionRelTo(Pi::game->GetSpace()->GetRootFrame());

			ss << stringf("Pos: %0{f.2}, %1{f.2}, %2{f.2}\n", pos.x, pos.y, pos.z);
			ss << stringf("AbsPos: %0{f.2}, %1{f.2}, %2{f.2}\n", abs_pos.x, abs_pos.y, abs_pos.z);

			const SystemPath &path(Pi::player->GetFrame()->GetSystemBody()->GetPath());
			ss << stringf("Rel-to: %0 [%1{d},%2{d},%3{d},%4{u},%5{u}] ",
				Pi::player->GetFrame()->GetLabel(),
				path.sectorX, path.sectorY, path.sectorZ, path.systemIndex, path.bodyIndex);
			ss << stringf("(%0{f.2} km), rotating: %1\n",
				pos.Length()/1000, (Pi::player->GetFrame()->IsRotFrame() ? "yes" : "no"));

			//Calculate lat/lon for ship position
			const vector3d dir = pos.NormalizedSafe();
			const float lat = RAD2DEG(asin(dir.y));
			const float lon = RAD2DEG(atan2(dir.x, dir.z));

			ss << stringf("Lat / Lon: %0{f.8} / %1{f.8}\n", lat, lon);
		}

		char aibuf[256];
		Pi::player->AIGetStatusText(aibuf); aibuf[255] = 0;
		ss << aibuf << std::endl;

		m_debugInfo->SetText(ss.str());
		m_debugInfo->Show();
	} else {
		m_debugInfo->Hide();
	}
#endif

	if (Pi::player->GetFlightState() == Ship::HYPERSPACE) {
		const SystemPath dest = Pi::player->GetHyperspaceDest();
		RefCountedPtr<StarSystem> s = StarSystemCache::GetCached(dest);

		m_overlayBuffer[0] = stringf(Lang::IN_TRANSIT_TO_N_X_X_X,
			formatarg("system", dest.IsBodyPath() ? s->GetBodyByPath(dest)->GetName() : s->GetName()),
			formatarg("x", dest.sectorX),
			formatarg("y", dest.sectorY),
			formatarg("z", dest.sectorZ));
		m_circleTitleBuffer[1] = "SPEED";
		m_circleDataBuffer[1] = "JUMPING";

		Pi::cpan->SetOverlayText(ShipCpanel::OVERLAY_TOP_RIGHT, stringf(Lang::PROBABILITY_OF_ARRIVAL_X_PERCENT,
			formatarg("probability", Pi::game->GetHyperspaceArrivalProbability()*100.0, "f3.1")));

		m_devBuffer = "";
	} else {
		{
			std::string str;
			double _vel = 0;
			const char *rel_to = 0;
			const Body *set_speed_target = Pi::player->GetSetSpeedTarget();
			if (set_speed_target) {
				rel_to = set_speed_target->GetLabel().c_str();
				_vel = Pi::player->GetVelocityRelTo(set_speed_target).Length();
			} else {
				rel_to = Pi::player->GetFrame()->GetLabel().c_str();
				_vel = vel.Length();
			}
			_vel = Unjitter(_vel, Pi::player->GetController()->GetSpeedLimit());
			if (_vel > 1000) {
				str = stringf(Lang::KM_S_RELATIVE_TO, formatarg("speed", _vel*0.001), formatarg("frame", rel_to));
			} else {
				str = stringf(Lang::M_S_RELATIVE_TO, formatarg("speed", _vel), formatarg("frame", rel_to));
			}
			m_overlayBuffer[0] = str;
			m_circleTitleBuffer[1] = "SPEED";

			if(_vel <= 1000) {
				m_circleDataBuffer[1] = PadWithZeroes(_vel, 4) + " m/s";
			} else if(_vel < TRANSIT_DRIVE_1_SPEED) {
				m_circleDataBuffer[1] = PadWithZeroes(_vel * 0.001, 3) + " km/s";
			} else if(_vel >= (TRANSIT_DRIVE_1_SPEED - 100000) && 
					    _vel <= (TRANSIT_DRIVE_1_SPEED + 100000)) {
				m_circleDataBuffer[1] = "TRANSIT1";
			} else {
				m_circleDataBuffer[1] = "TRANSIT2";
			}
		}

		if (Body *navtarget = Pi::player->GetNavTarget()) {
			double dist = Pi::player->GetPositionRelTo(navtarget).Length();
			m_overlayBuffer[2] = stringf(Lang::N_DISTANCE_TO_TARGET,
				formatarg("distance", format_distance(dist)));
			m_circleTitleBuffer[2] = "TARGET";
			m_circleDataBuffer[2] = stringf("%distance AU",
				formatarg("distance", format_distance(dist)));
		} else {
			m_overlayBuffer[2] = Lang::P_NO_TARGET;
			m_circleTitleBuffer[2] = "";
			m_circleDataBuffer[2] = "";
		}

		// altitude
		const Frame* frame = Pi::player->GetFrame();
		m_bAltitudeAvailable = false;
		if (frame->GetBody() && frame->GetBody()->IsType(Object::SPACESTATION))
			frame = frame->GetParent();
		if (frame && frame->GetBody() && frame->GetBody()->IsType(Object::TERRAINBODY) &&
				(frame->HasRotFrame() || frame->IsRotFrame())) {
			Body *astro = frame->GetBody();
			//(GetFrame()->m_sbody->GetSuperType() == SUPERTYPE_ROCKY_PLANET)) {
			assert(astro->IsType(Object::TERRAINBODY));
			TerrainBody* terrain = static_cast<TerrainBody*>(astro);
			if (!frame->IsRotFrame())
				frame = frame->GetRotFrame();
			vector3d pos = (frame == Pi::player->GetFrame() ? Pi::player->GetPosition() : Pi::player->GetPositionRelTo(frame));
			double center_dist = pos.Length();
			// Avoid calculating terrain if we are too far anyway.
			// This should rather be 1.5 * max_radius, but due to quirkses in terrain generation we must be generous.
			if (center_dist <= 3.0 * terrain->GetMaxFeatureRadius()) {
				vector3d surface_pos = pos.Normalized();
				double radius = terrain->GetTerrainHeight(surface_pos);
				double altitude = center_dist - radius;

				if (altitude < 10000000.0 && altitude < 0.5 * radius) {
					vector3d velocity = (frame == Pi::player->GetFrame() ? vel : Pi::player->GetVelocityRelTo(frame));
					double vspeed = velocity.Dot(surface_pos);
					if (fabs(vspeed) < 0.05) vspeed = 0.0; // Avoid alternating between positive/negative zero
					if (altitude < 0) altitude = 0;
					if (altitude >= 100000.0) {
						m_overlayBuffer[3] = stringf(Lang::ALT_IN_KM, formatarg("altitude", altitude / 1000.0),
							formatarg("vspeed", vspeed / 1000.0));
						m_circleTitleBuffer[0] = "ALTIM";
						m_circleDataBuffer[0] = PadWithZeroes(altitude / 1000.0, 4) + " km";
					} else {
						m_overlayBuffer[3] = stringf(Lang::ALT_IN_METRES, formatarg("altitude", altitude),
							formatarg("vspeed", vspeed));
						m_circleTitleBuffer[0] = "ALTIM";
						m_circleDataBuffer[0] = PadWithZeroes(altitude, 5) + " m";
					}
					m_bAltitudeAvailable = true;
					m_altitude = altitude;
				} else {
					m_overlayBuffer[3] = Lang::ALT_NO_DATA;
					m_circleTitleBuffer[0] = "ALTIM";
					m_circleDataBuffer[0] = "TOO FAR";
				}
			} else {
				m_overlayBuffer[3] = Lang::ALT_NO_DATA;
				m_circleTitleBuffer[0] = "ALTIM";
				m_circleDataBuffer[0] = "TOO FAR";
			}

			// atmosphere and pressure
			if (astro->IsType(Object::PLANET)) {
				double pressure, density;
				static_cast<Planet*>(astro)->GetAtmosphericState(center_dist, &pressure, &density);

				m_overlayBuffer[1] = stringf(Lang::PRESSURE_N_ATMOSPHERES, formatarg("pressure", pressure));
				float hullTemp = (float)Pi::player->GetHullTemperature();
				m_hudHullTemp->SetValue(hullTemp);
				if (hullTemp >= Pi::player->GetMaxHullTemp()) {
					m_hudHullTemp->SetColor(get_color_for_warning_meter_bar(hullTemp));
				}
				else {
					m_hudHullTemp->SetColor(Color::PARAGON_GREEN);
				}
			} else {
				m_overlayBuffer[1] = stringf(Lang::PRESSURE_N_ATMOSPHERES, formatarg("pressure", 0.0));
			}
		} else {
			m_overlayBuffer[1] = stringf(Lang::PRESSURE_N_ATMOSPHERES, formatarg("pressure", 0.0));
			m_overlayBuffer[3] = Lang::ALT_NO_DATA;
		}

		m_hudFuelGauge->SetValue(Pi::player->GetFuel());
		m_hudHydrogenGauge->SetValue(Pi::player->GetHydrogenPercentage() / 100.0);

		/*{
			std::ostringstream ss;
			ss << "HYDROGEN: " << Pi::player->GetHydrogenPercentage() << "% ("<< 
				Pi::player->GetHydrogen() << "/" <<
				Pi::player->GetHydrogenCapacity() << ")";
			m_devBuffer = ss.str();
		}*/
	}

	const float activeWeaponTemp = Pi::player->GetGunTemperature(GetActiveWeapon());
	m_hudWeaponTemp->SetValue(activeWeaponTemp);

	float hull = Pi::player->GetPercentHull();	
	m_hudHullIntegrity->SetValue(hull*0.01f);

	float shields = Pi::player->GetPercentShields();

	//m_hudShieldIntegrity->SetColor(get_color_for_warning_meter_bar(shields));
	if (shields <= 0.0f) {
		m_hudShieldIntegrity->SetValue(1.0f);
		m_hudShieldIntegrity->SetColor(get_color_for_warning_meter_bar(shields));
	}
	else {
		m_hudShieldIntegrity->SetValue(shields*0.01f);
		m_hudShieldIntegrity->SetColor(Color::PARAGON_GREEN);
	}
	m_hudShieldIntegrity->Show();

	Body *b = Pi::player->GetCombatTarget() ? Pi::player->GetCombatTarget() : Pi::player->GetNavTarget();
	if (b) {
		if ((b->IsType(Object::SHIP) && Pi::player->m_equipment.Get(Equip::SLOT_RADARMAPPER) == Equip::RADAR_MAPPER)) {
			assert(b->IsType(Object::SHIP));
			Ship *s = static_cast<Ship*>(b);

			const shipstats_t &stats = s->GetStats();

			m_hudTargetShip->Show();
			m_hudTargetDesc->Show();
			m_hudTargetCargo->Show();

			float sHull = s->GetPercentHull();
			//m_hudTargetHullIntegrity->SetColor(get_color_for_warning_meter_bar(sHull));
			m_hudTargetHullIntegrity->SetValue(sHull*0.01f);
			m_hudTargetHullIntegrity->Show();

			float sShields = 0;
			if (s->m_equipment.Count(Equip::SLOT_SHIELD, Equip::SHIELD_GENERATOR) > 0) {
				sShields = s->GetPercentShields();
			}
			//m_hudTargetShieldIntegrity->SetColor(get_color_for_warning_meter_bar(sShields));
			m_hudTargetShieldIntegrity->SetValue(sShields*0.01f);
			m_hudTargetShieldIntegrity->Show();

			m_hudTargetFuel->SetValue(s->GetFuel());
			m_hudTargetFuel->Show();

			m_hudTargetShip->SetText(s->GetLabel());
			m_hudTargetDesc->SetText(s->GetShipType()->name);
			m_hudTargetCargo->SetText(stringf(Lang::CARGO_N, formatarg("mass", stats.used_cargo)));
			
			if (s->m_equipment.Get(Equip::SLOT_ENGINE) == Equip::NONE) {
				m_vHudTargetInfo[0]->SetText(Lang::NO_HYPERDRIVE);
			} else {
				m_vHudTargetInfo[0]->SetText(Equip::types[s->m_equipment.Get(Equip::SLOT_ENGINE)].name);
			}
			m_vHudTargetInfo[1]->SetText(stringf(Lang::MASS_N_TONNES, formatarg("mass", stats.total_mass)));
			m_vHudTargetInfo[2]->SetText(stringf(Lang::SHIELD_STRENGTH_N, formatarg("shields",
				(sShields*0.01f) * float(s->m_equipment.Count(Equip::SLOT_SHIELD, Equip::SHIELD_GENERATOR)))));

			for (unsigned it = 0; it < m_vHudTargetInfo.size(); ++it) {
				MoveChild(m_vHudTargetInfo[it], Gui::Screen::GetWidth() - 150.0f, 90.0f + (14.0f * it));
				m_vHudTargetInfo[it]->SetRightMargin(140.0f);
				if (it <= 2) {
					//m_vHudTargetInfo[it]->Show(); 
				} else {
					m_vHudTargetInfo[it]->SetText("");
				}
			}
		}

		else if (b->IsType(Object::HYPERSPACECLOUD) && Pi::player->m_equipment.Get(Equip::SLOT_HYPERCLOUD) == Equip::HYPERCLOUD_ANALYZER) {
			HyperspaceCloud *cloud = static_cast<HyperspaceCloud*>(b);

			m_hudTargetShip->Hide();
			m_hudTargetDesc->Hide();
			m_hudTargetCargo->Hide();
			m_hudTargetHullIntegrity->Hide();
			m_hudTargetShieldIntegrity->Hide();
			m_hudTargetFuel->Hide();

			std::string text;

			Ship *ship = cloud->GetShip();
			if (!ship) {
				m_hudTargetShip->SetText(Lang::HYPERSPACE_ARRIVAL_CLOUD_REMNANT);
				m_hudTargetDesc->SetText("");
				m_hudTargetCargo->SetText("");
				for(unsigned it = 0; it < m_vHudTargetInfo.size(); ++it) {
					m_vHudTargetInfo[it]->SetText("");
				}
			}
			else {
				const SystemPath dest = ship->GetHyperspaceDest();
				RefCountedPtr<const Sector> s = Sector::cache.GetCached(dest);
                
				m_hudTargetShip->SetText(cloud->GetCloudTypeString());
				m_vHudTargetInfo[0]->SetText(
					stringf(Lang::SHIP_MASS_N_TONNES, formatarg("mass", ship->GetStats().total_mass)));
				text = (cloud->GetCloudDirString());
				text += ": ";
				text += s->m_systems[dest.systemIndex].name;
				m_vHudTargetInfo[1]->SetText(text);
				m_vHudTargetInfo[2]->SetText(
					stringf(Lang::DATE_DUE_N, formatarg("date", format_date(cloud->GetDueDate()))));

				for (unsigned it = 3; it < m_vHudTargetInfo.size(); ++it) {
					m_vHudTargetInfo[it]->SetText("");
				}
			}

			for (unsigned it = 0; it < m_vHudTargetInfo.size(); ++it) {
				MoveChild(m_vHudTargetInfo[it], Gui::Screen::GetWidth() - 180.0f, 5.0f + (14.0f * it));
				m_vHudTargetInfo[it]->SetRightMargin(170.0f);
				//m_vHudTargetInfo[it]->Show();
			}
		}

		else {
			b = 0;
		}
	}
	if (!b) {
		m_hudTargetShip->Hide();
		m_hudTargetDesc->Hide();
		m_hudTargetCargo->Hide();
		m_hudTargetHullIntegrity->Hide();
		m_hudTargetShieldIntegrity->Hide();
		m_hudTargetFuel->Hide();

		for(unsigned it = 0; it < m_vHudTargetInfo.size(); ++it) {
			m_vHudTargetInfo[it]->Hide();
		}
	}

	if (Pi::player->IsHyperspaceActive()) {
		float val = Pi::player->GetHyperspaceCountdown();

		if (!(int(ceil(val*2.0)) % 2)) {
			m_hudHyperspaceInfo->SetText(stringf(Lang::HYPERSPACING_IN_N_SECONDS, formatarg("countdown", ceil(val))));
			m_hudHyperspaceInfo->Show();
		} else {
			m_hudHyperspaceInfo->Hide();
		}
	} else {
		m_hudHyperspaceInfo->Hide();
	}
}

void WorldView::RefreshLocationInfo()
{
	if(Pi::player) {
		if(Pi::player->GetFlightState() == Ship::FlightState::HYPERSPACE) {
			return;
		} else {
			// TODO Primary and Secondary location m_LocationPrimary and m_LocationSecondary
			// Primary Location: system name
			if(Pi::game && Pi::game->GetSpace()) {
				m_hudLocationPrimary->SetText(Pi::game->GetSpace()->GetStarSystem()->GetName());
			}
			// Secondary Location: frame body name or CLEARSPACE when in freespace.
			Frame* pframe = Pi::player->GetFrame();
			std::string secondary_location;
			if(pframe) {
				if(pframe->GetBody() && pframe->GetLabel() != "System") {
					secondary_location = pframe->GetLabel();
				} else {
					secondary_location = "CLEARSPACE";
				}
			} else {
				secondary_location = "CLEARSPACE";
			}
			m_hudLocationSecondary->SetText(secondary_location);
		}
	}
}

void WorldView::Update()
{
	PROFILE_SCOPED()
	assert(Pi::game);
	assert(Pi::player);
	assert(!Pi::player->IsDead());

    UpdateFastTravel();

	const double frameTime = Pi::GetFrameTime();
	// show state-appropriate buttons
	RefreshButtonStateAndVisibility();

	if(Pi::MouseButtonState(SDL_BUTTON_RIGHT) || Pi::player->GetPlayerController()->GetMouseFlightMode()) {
		// when controlling your ship with the mouse you don't want to pick targets
		m_bodyLabels->SetLabelsClickable(false);
	} else {
		m_bodyLabels->SetLabelsClickable(true);
	}

	//m_bodyLabels->SetLabelsVisible(m_labelsOn);
	m_bodyLabels->SetLabelsVisible(false);

	bool targetObject = false;
	bool deselectAll = false;

	// XXX ugly hack checking for console here
	if (!Pi::IsConsoleActive()) {
		if (GetCamType() == CAM_INTERNAL) {
			if      (KeyBindings::frontCamera.IsActive())  ChangeInternalCameraMode(InternalCameraController::MODE_FRONT);
			else if (KeyBindings::rearCamera.IsActive())   ChangeInternalCameraMode(InternalCameraController::MODE_REAR);
			else if (KeyBindings::leftCamera.IsActive())   ChangeInternalCameraMode(InternalCameraController::MODE_LEFT);
			else if (KeyBindings::rightCamera.IsActive())  ChangeInternalCameraMode(InternalCameraController::MODE_RIGHT);
			else if (KeyBindings::topCamera.IsActive())    ChangeInternalCameraMode(InternalCameraController::MODE_TOP);
			else if (KeyBindings::bottomCamera.IsActive()) ChangeInternalCameraMode(InternalCameraController::MODE_BOTTOM);

			InternalCameraController* cam = static_cast<InternalCameraController*>(m_activeCameraController);
			if(cam->GetMode() == InternalCameraController::Mode::MODE_FRONT &&
				Pi::player->GetTransitState() == TRANSIT_DRIVE_OFF) 
			{
				if (KeyBindings::freelookCentre.IsActive()) {
					cam->Reset();
					cam->ResetFreelook();
				} else {
					if(KeyBindings::freelookUp.IsActive()) {
						m_internalCameraController->RotateUp(frameTime);
					}
					if(KeyBindings::freelookDown.IsActive()) {
						m_internalCameraController->RotateDown(frameTime);
					}
					if(KeyBindings::freelookLeft.IsActive()) {
						m_internalCameraController->RotateLeft(frameTime);
					}
					if(KeyBindings::freelookRight.IsActive()) {
						m_internalCameraController->RotateRight(frameTime);
					}
				}
			}
		}
		else {
			if (KeyBindings::cameraRotateUp.IsActive()) m_activeCameraController->RotateUp(frameTime);
			if (KeyBindings::cameraRotateDown.IsActive()) m_activeCameraController->RotateDown(frameTime);
			if (KeyBindings::cameraRotateLeft.IsActive()) m_activeCameraController->RotateLeft(frameTime);
			if (KeyBindings::cameraRotateRight.IsActive()) m_activeCameraController->RotateRight(frameTime);
			if (KeyBindings::viewZoomOut.IsActive()) m_activeCameraController->ZoomEvent(ZOOM_SPEED*frameTime);		// Zoom out
			if (KeyBindings::viewZoomIn.IsActive()) m_activeCameraController->ZoomEvent(-ZOOM_SPEED*frameTime);
			if (KeyBindings::cameraRollLeft.IsActive()) m_activeCameraController->RollLeft(frameTime);
			if (KeyBindings::cameraRollRight.IsActive()) m_activeCameraController->RollRight(frameTime);
			if (KeyBindings::resetCamera.IsActive()) m_activeCameraController->Reset();
			m_activeCameraController->ZoomEventUpdate(frameTime);
		}

		// note if we have to target the object in the crosshairs
		targetObject = KeyBindings::targetObject.IsActive();
		deselectAll = KeyBindings::deselectAll.IsActive();
	}

	m_activeCameraController->Update();

	m_cameraContext->BeginFrame();
	m_camera->Update();

	UpdateProjectedObjects();

	const Frame *playerFrame = Pi::player->GetFrame();
	const Frame *camFrame = m_cameraContext->GetCamFrame();

	//speedlines and contact trails need camFrame for transform, so they
	//must be updated here
	if (Pi::AreSpeedLinesDisplayed()) {
		m_speedLines->Update(Pi::game->GetTimeStep());

        matrix4x4d trans;
		Frame::GetFrameTransform(playerFrame, camFrame, trans);

		if ( m_speedLines.get() && Pi::AreSpeedLinesDisplayed() ) {
			m_speedLines->Update(Pi::game->GetTimeStep());
			trans[12] = trans[13] = trans[14] = 0.0;
			trans[15] = 1.0;
			m_speedLines->SetTransform(trans);
		}
	}

	// target object under the crosshairs. must be done after
	// UpdateProjectedObjects() to be sure that m_projectedPos does not have
	// contain references to deleted objects
	if (targetObject) {
		Body* const target = PickBody(double(Gui::Screen::GetWidth())/2.0, double(Gui::Screen::GetHeight())/2.0);
		SelectBody(target, false);
	} else if (deselectAll) {
		DeselectAll();
	}

	if(!Pi::game->IsHyperspace()) {
		// Exhaust Trails/Update
		const Frame *cam_frame = m_cameraContext->GetCamFrame();
		matrix4x4d trans;
		Frame::GetFrameTransform(Pi::player->GetFrame(), cam_frame, trans);
		// Thruster trails: other ships		
		for (auto it = Pi::player->GetSensors()->GetContacts().begin(); 
			it != Pi::player->GetSensors()->GetContacts().end(); ++it) 
		{
			for (unsigned i = 0; i < it->ship->GetThrusterTrailsNum(); ++i) {
				ThrusterTrail* tt = it->ship->GetThrusterTrail(i);
				if (tt) {
					tt->SetTransform(trans);
					tt->Update(frameTime);
				}
			}
		}
		// Thruster trails: player ship
		for(unsigned int i = 0; i < Pi::player->GetThrusterTrailsNum(); ++i) {
				Pi::player->GetThrusterTrail(i)->SetTransform(trans);
				Pi::player->GetThrusterTrail(i)->Update(frameTime);
		}
	}


    UpdateContactList();
	UIView::Update();
}

void WorldView::OnSwitchTo()
{
	UIView::OnSwitchTo();
	RefreshButtonStateAndVisibility();
}

void WorldView::OnSwitchFrom()
{
	Pi::DrawGUI = true;
}

void WorldView::ToggleTargetActions()
{
	if (Pi::game->IsHyperspace() || m_showTargetActionsTimeout)
		HideTargetActions();
	else
		ShowTargetActions();
}

void WorldView::ShowTargetActions()
{
	m_showTargetActionsTimeout = SDL_GetTicks();
	UpdateCommsOptions();
	HideLowThrustPowerOptions();
}

void WorldView::HideTargetActions()
{
	m_showTargetActionsTimeout = 0;
	UpdateCommsOptions();
}

Gui::Button *WorldView::AddCommsOption(std::string msg, int ypos, int optnum)
{
	char buffIntToString[2];
	snprintf(buffIntToString, sizeof(buffIntToString), "%d", optnum);
	Gui::Label *l = new Gui::Label(std::string(buffIntToString) + ". " + msg);
	l->Color(Color::PARAGON_BLUE);

	Gui::ClickableLabel *b = new Gui::ClickableLabel(l);
	b->SetShortcut(SDL_Keycode(SDLK_0 + optnum), KMOD_NONE);

	// hide target actions when things get clicked on
	b->onClick.connect(sigc::mem_fun(this, &WorldView::ToggleTargetActions));
	m_commsOptions->Add(b, 16, float(ypos));
	return b;
}

void WorldView::OnClickCommsNavOption(Body *target, Gui::ClickableLabel *button)
{
	Pi::player->SetNavTarget(target);
	m_showTargetActionsTimeout = SDL_GetTicks();
	HideLowThrustPowerOptions();
}

void WorldView::AddCommsNavOption(std::string msg, Body *target)
{
	Gui::HBox *hbox = new Gui::HBox();
	hbox->SetSpacing(5);

	std::string imgLocation;
	GetNavIconFile(target->GetType(), imgLocation);

	Gui::Screen::PushFont("HudFont");
	Gui::Label *l;
	Color clr;
	if (Pi::player->GetNavTarget() != nullptr && Pi::player->GetNavTarget()->GetLabel() == msg) // Currently done here but might move inside the ClickableLabel class.
	{
		clr = Color::PARAGON_GREEN;
		l = (new Gui::Label("      " + msg))->Color(clr); // Bandaid fix to the padding issue.
	}
	else
	{
		clr = Color::PARAGON_BLUE;
		l = (new Gui::Label("      " + msg))->Color(clr); // Bandaid fix to the padding issue.
	}
	Gui::Screen::PopFont();
	//hbox->PackStart(l);;

	Gui::ClickableLabel *b = new Gui::ClickableLabel(l, imgLocation.c_str(), clr);
	b->onClick.connect(sigc::bind(sigc::mem_fun(this, &WorldView::OnClickCommsNavOption), target, b));
	hbox->PackStart(b);

	m_commsNavOptions->PackEnd(hbox);
}

void WorldView::BuildCommsNavOptions()
{
	std::map< Uint32,std::vector<SystemBody*> > groups;
	Body * const navTarget = Pi::player->GetNavTarget();
	Body * const comTarget = Pi::player->GetCombatTarget();
	bool targetFoundInList = false;
	for (SystemBody* station : Pi::game->GetSpace()->GetStarSystem()->GetSpaceStations()) {
		groups[station->GetParent()->GetPath().bodyIndex].push_back(station);
	}

	HyperspaceCloud* hc = Pi::game->GetSpace()->GetNearestPermaHyperspaceCloud(Pi::player);
    	if(hc) {
        	SystemBody* sb = const_cast<SystemBody*>(hc->GetSystemBody());
        	groups[sb->GetPath().bodyIndex].push_back(sb);
    	}


	for (std::map<Uint32, std::vector<SystemBody*> >::const_iterator i = groups.begin(); i != groups.end(); ++i) {
		if (i != groups.begin()) {
			m_commsNavOptions->PackEnd((new Gui::Label("\n"))->Color(Color::PARAGON_BLUE));
		}

		for ( std::vector<SystemBody*>::const_iterator j = (*i).second.begin(); j != (*i).second.end(); ++j) {
			SystemPath path = Pi::game->GetSpace()->GetStarSystem()->GetPathOf(*j);
			Body *body = Pi::game->GetSpace()->FindBodyForPath(&path);
			AddCommsNavOption((*j)->GetName(), body);
			if (navTarget && navTarget->GetLabel() == (*j)->GetName())
				targetFoundInList = true;
		}
	}
	if (comTarget) {
		m_commsNavOptions->PackEnd((new Gui::Label("\n"))->Color(Color::PARAGON_BLUE));
		AddCommsNavOption(comTarget->GetLabel(), comTarget);
	}
	if (navTarget && comTarget != navTarget && !targetFoundInList) {
		m_commsNavOptions->PackEnd((new Gui::Label("\n"))->Color(Color::PARAGON_BLUE));
		AddCommsNavOption(navTarget->GetLabel(), navTarget);
	}
	m_commsNavOptions->ResizeRequest();
}

void WorldView::HideLowThrustPowerOptions()
{
	m_showLowThrustPowerTimeout = 0;
	m_lowThrustPowerOptions->Hide();
}

void WorldView::ShowLowThrustPowerOptions()
{
	m_showLowThrustPowerTimeout = SDL_GetTicks();
	m_lowThrustPowerOptions->Show();
	HideTargetActions();
}

void WorldView::OnSelectLowThrustPower(float power)
{
	Pi::player->GetPlayerController()->SetLowThrustPower(power);
	HideLowThrustPowerOptions();
}

static void PlayerRequestDockingClearance(SpaceStation *s)
{
	std::string msg;
	s->GetDockingClearance(Pi::player, msg);
	//Pi::cpan->MsgLog()->ImportantMessage(s->GetLabel(), msg);
	Pi::game->log->Add("", msg);
}

// XXX paying fine remotely can't really be done until crime and
// worldview are in Lua. I'm leaving this code here so its not
// forgotten
/*
static void PlayerPayFine()
{
	Sint64 crime, fine;
	Polit::GetCrime(&crime, &fine);
	if (Pi::player->GetMoney() == 0) {
		//Pi::cpan->MsgLog()->Message("", Lang::YOU_NO_MONEY);
		Pi::game->log->Add("", Lang::YOU_NO_MONEY);
	} else if (fine > Pi::player->GetMoney()) {
		Polit::AddCrime(0, -Pi::player->GetMoney());
		Polit::GetCrime(&crime, &fine);
		//Pi::cpan->MsgLog()->Message("", stringf(
		Pi::game->log->Add("", stringf(
			Lang::FINE_PAID_N_BUT_N_REMAINING,
				formatarg("paid", format_money(Pi::player->GetMoney())),
				formatarg("fine", format_money(fine))));
		Pi::player->SetMoney(0);
	} else {
		Pi::player->SetMoney(Pi::player->GetMoney() - fine);
		//Pi::cpan->MsgLog()->Message("", stringf(Lang::FINE_PAID_N,
		Pi::game->log->Add("", stringf(Lang::FINE_PAID_N,
				formatarg("fine", format_money(fine))));
		Polit::AddCrime(0, -fine);
	}
}
*/

void WorldView::OnHyperspaceTargetChanged()
{
	if (Pi::player->IsHyperspaceActive()) {
		Pi::player->ResetHyperspaceCountdown();
		//Pi::cpan->MsgLog()->Message("", Lang::HYPERSPACE_JUMP_ABORTED);
		Pi::game->log->Add("", Lang::HYPERSPACE_JUMP_ABORTED);
	}

	const SystemPath path = Pi::sectorView->GetHyperspaceTarget();

	RefCountedPtr<StarSystem> system = StarSystemCache::GetCached(path);
	const std::string& name = path.IsBodyPath() ? system->GetBodyByPath(path)->GetName() : system->GetName() ;
	//Pi::cpan->MsgLog()->Message("", stringf(Lang::SET_HYPERSPACE_DESTINATION_TO, formatarg("system", name)));
	Pi::game->log->Add("", stringf(Lang::SET_HYPERSPACE_DESTINATION_TO, formatarg("system", name)));
}

void WorldView::OnPlayerChangeTarget()
{
	Body *b = Pi::player->GetNavTarget();
	if (b) {
		Sound::PlaySfx("OK");
		Ship *s = b->IsType(Object::HYPERSPACECLOUD) ? static_cast<HyperspaceCloud*>(b)->GetShip() : 0;
		if (!s || !Pi::sectorView->GetHyperspaceTarget().IsSameSystem(s->GetHyperspaceDest()))
			Pi::sectorView->FloatHyperspaceTarget();
	}

	UpdateCommsOptions();
}

static void autopilot_flyto(Body *b)
{
    if (isDocking && m_fastTravelTarget) {
        if (static_cast<SpaceStation*>(m_fastTravelTarget))
        static_cast<SpaceStation*>(m_fastTravelTarget)->CancelDockingApproach(Pi::player);
        m_fastTravelTarget = nullptr;
    }
    isDocking = false;
    m_fastTravelTarget = b;
    Pi::player->SetInvulnerable(true);
    Pi::game->SetFastTravel(true);
    m_fastTravelTimeout = SDL_GetTicks();
}
static void autopilot_dock(Body *b)
{
    if (isDocking && m_fastTravelTarget) {
        if (static_cast<SpaceStation*>(m_fastTravelTarget))
        static_cast<SpaceStation*>(m_fastTravelTarget)->CancelDockingApproach(Pi::player);
        m_fastTravelTarget = nullptr;
    }
	if(Pi::player->GetFlightState() != Ship::FLYING)
		return;


    isDocking = true;
    m_fastTravelTarget = b;
    Pi::player->SetInvulnerable(true);
    Pi::game->SetFastTravel(true);
    m_fastTravelTimeout = SDL_GetTicks();

}
static void autopilot_orbit(Body *b, double alt)
{
	Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_AUTOPILOT);
	Pi::player->AIOrbit(b, alt);
}

static void player_target_hypercloud(HyperspaceCloud *cloud)
{
	if(cloud->GetShip()) {
		Pi::sectorView->SetHyperspaceTarget(cloud->GetShip()->GetHyperspaceDest());
	}
}

void WorldView::UpdateCommsOptions()
{
	Gui::Screen::PushFont("HudFont");

	m_commsOptions->DeleteAllChildren();
	m_commsNavOptions->DeleteAllChildren();
	
	if (m_showTargetActionsTimeout == 0) return;

	BuildCommsNavOptions();

	Gui::Button *button;
	int ypos = 0;
	Body * const navtarget = Pi::player->GetNavTarget();

	int optnum = 1;	const bool hasAutopilot = (Pi::player->m_equipment.Get(Equip::SLOT_AUTOPILOT) == Equip::AUTOPILOT) && 
		(Pi::player->GetFlightState() == Ship::FLYING);

	if (navtarget) {
		ypos += 16;
		if (navtarget->IsType(Object::SPACESTATION)) {
			if( hasAutopilot )
			{
				button = AddCommsOption(Lang::AUTOPILOT_DOCK_WITH_STATION, ypos, optnum++);
				button->onClick.connect(sigc::bind(sigc::ptr_fun(&autopilot_dock), navtarget));
				ypos += 16;
			}
		}
		if (hasAutopilot) {
			button = AddCommsOption(stringf(Lang::AUTOPILOT_FLY_TO_VICINITY_OF, formatarg("target", navtarget->GetLabel())), ypos, optnum++);
			button->onClick.connect(sigc::bind(sigc::ptr_fun(&autopilot_flyto), navtarget));
			ypos += 16;

			if (navtarget->IsType(Object::PLANET) || navtarget->IsType(Object::STAR)) {
				button = AddCommsOption(stringf(Lang::AUTOPILOT_ENTER_LOW_ORBIT_AROUND, formatarg("target", navtarget->GetLabel())), ypos, optnum++);
				button->onClick.connect(sigc::bind(sigc::ptr_fun(autopilot_orbit), navtarget, 1.2));
				ypos += 16;


				//button = AddCommsOption(stringf(Lang::AUTOPILOT_ENTER_HIGH_ORBIT_AROUND, formatarg("target", navtarget->GetLabel())), ypos, optnum++);
				//button->onClick.connect(sigc::bind(sigc::ptr_fun(autopilot_orbit), navtarget, 3.2));
				//ypos += 32;

				if (navtarget->IsType(Object::PLANET) && Pi::player->m_equipment.Get(Equip::SLOT_FUELSCOOP) != Equip::NONE) {
					if (navtarget->GetSystemBody()->IsScoopable() && navtarget->GetSystemBody()->GetMass() < EARTH_MASS*150.0) {
						button = AddCommsOption(stringf(Lang::AUTOPILOT_ENTER_SCOOP_ORBIT_AROUND, formatarg("target", navtarget->GetLabel())), ypos, optnum++);
						button->onClick.connect(sigc::bind(sigc::ptr_fun(autopilot_orbit), navtarget, 0.99998));
						ypos += 16;
					}
				}
			}
		}

		const Equip::Type t = Pi::player->m_equipment.Get(Equip::SLOT_HYPERCLOUD);
		if ((t != Equip::NONE) && navtarget->IsType(Object::HYPERSPACECLOUD)) {
			HyperspaceCloud *cloud = static_cast<HyperspaceCloud*>(navtarget);
			if (cloud->SupportsDeparture() && cloud->HasShip()) {
				button = AddCommsOption(Lang::SET_HYPERSPACE_TARGET_TO_FOLLOW_THIS_DEPARTURE, ypos, optnum++);
				button->onClick.connect(sigc::bind(sigc::ptr_fun(player_target_hypercloud), cloud));
			}
		}
	}

	CenterAutopilotOptions();

	Gui::Screen::PopFont();
}

void WorldView::SelectBody(Body *target, bool reselectIsDeselect)
{
	if (!target || target == Pi::player) return;		// don't select self
	if (target->IsType(Object::PROJECTILE)) return;

	if (target->IsType(Object::SHIP)) {
		if (Pi::player->GetCombatTarget() == target) {
			if (reselectIsDeselect) Pi::player->SetCombatTarget(0);
		} else {
			Pi::player->SetCombatTarget(target, Pi::KeyState(SDLK_LCTRL) || Pi::KeyState(SDLK_RCTRL));
		}
	} else { // For cargo as well
		if (Pi::player->GetNavTarget() == target) {
			if (reselectIsDeselect) Pi::player->SetNavTarget(0);
		} else {
			Pi::player->SetNavTarget(target, Pi::KeyState(SDLK_LCTRL) || Pi::KeyState(SDLK_RCTRL));
		}
	}
}

void WorldView::DeselectAll()
{
	Pi::player->SetCombatTarget(nullptr);
	Pi::player->SetNavTarget(nullptr);
}

Body* WorldView::PickBody(const double screenX, const double screenY) const
{
	for (std::map<Body*,vector3d>::const_iterator
		i = m_projectedPos.begin(); i != m_projectedPos.end(); ++i) {
		Body *b = i->first;

		if (b == Pi::player || b->IsType(Object::PROJECTILE))
			continue;

		const double x1 = i->second.x - PICK_OBJECT_RECT_SIZE * 0.5;
		const double x2 = x1 + PICK_OBJECT_RECT_SIZE;
		const double y1 = i->second.y - PICK_OBJECT_RECT_SIZE * 0.5;
		const double y2 = y1 + PICK_OBJECT_RECT_SIZE;
		if(screenX >= x1 && screenX <= x2 && screenY >= y1 && screenY <= y2)
			return b;
	}

	return 0;
}

int WorldView::GetActiveWeapon() const
{
	switch (GetCamType()) {
		case CAM_INTERNAL:
			return m_internalCameraController->GetMode() == InternalCameraController::MODE_REAR ? 1 : 0;

		case CAM_EXTERNAL:
		case CAM_SIDEREAL:
		default:
			return 0;
	}
}

static inline bool project_to_screen(const vector3d &in, vector3d &out, const Graphics::Frustum &frustum, const int guiSize[2])
{
	if (!frustum.ProjectPoint(in, out)) return false;
	out.x *= guiSize[0];
	out.y = Gui::Screen::GetHeight() - out.y * guiSize[1];
	return true;
}

void WorldView::UpdateProjectedObjects()
{
	const int guiSize[2] = { Gui::Screen::GetWidth(), Gui::Screen::GetHeight() };
	const Graphics::Frustum frustum = m_cameraContext->GetFrustum();

	const Frame *cam_frame = m_cameraContext->GetCamFrame();
	matrix3x3d cam_rot = cam_frame->GetOrient();

	// determine projected positions and update labels
	m_bodyLabels->Clear();
	m_projectedPos.clear();
	m_hud2BodyIcons.clear();

	double dist = 9999999999.0; //any high number will do.

	SystemPath* mission_system = Pi::player->GetCurrentMissionPath();
	Body* mission_body = nullptr;
	Space* space = Pi::game->GetSpace();
	if(mission_system && space && mission_system->IsBodyPath() && !Pi::game->IsHyperspace() &&
		space->GetStarSystem()->GetPath().IsSameSystem(mission_system)) {
		mission_body = Pi::game->GetSpace()->FindBodyForPath(mission_system);
	}

	for (Body* b : Pi::game->GetSpace()->GetBodies()) {
		// don't show the player label on internal camera
		if (b->IsType(Object::PLAYER) && GetCamType() == CAM_INTERNAL)
			continue;

		vector3d pos = b->GetInterpPositionRelTo(cam_frame);
		if ((pos.z < -1.0) && project_to_screen(pos, pos, frustum, guiSize)) {

			// only show labels on large or nearby bodies
			if (b->IsType(Object::PLANET) || b->IsType(Object::STAR) || b->IsType(Object::SPACESTATION)
				|| Pi::player->GetPositionRelTo(b).LengthSqr() < 1000000.0*1000000.0)
			{
				m_bodyLabels->Add(b->GetLabel(), sigc::bind(
					sigc::mem_fun(this, &WorldView::SelectBody), b, true), float(pos.x), float(pos.y));
				bool current_mission = mission_body == b;
				m_hud2BodyIcons.push_back(SBodyIcon(pos.x, pos.y, b->GetType(), current_mission));
			}

			m_projectedPos[b] = pos;
		}

		// get nearest target for combat
		if (Pi::KeyState(SDLK_RCTRL) && b->IsType(Object::SHIP) && !b->IsType(Object::PLAYER) && Pi::player->GetPositionRelTo(b).Length() < 10000.0 && Pi::player->GetPositionRelTo(b).Length() < dist) {
			dist = Pi::player->GetPositionRelTo(b).Length();
			Pi::player->SetCombatTarget(b);
		}
	}

	// velocity relative to current frame (white)
	const vector3d camSpaceVel = Pi::player->GetVelocity() * cam_rot;
	if (camSpaceVel.LengthSqr() >= 1e-4)
		UpdateIndicator(m_velIndicator, camSpaceVel);
	else
		HideIndicator(m_velIndicator);

	// orientation according to mouse
	if (Pi::player->GetPlayerController()->IsMouseActive()) {
		vector3d mouseDir = Pi::player->GetPlayerController()->GetMouseDir() * cam_rot;
		if (GetCamType() == CAM_INTERNAL && m_internalCameraController->GetMode() == InternalCameraController::MODE_REAR)
			mouseDir = -mouseDir;
		UpdateIndicator(m_mouseDirIndicator, (Pi::player->GetPhysRadius() * 1.5) * mouseDir);
	} else
		HideIndicator(m_mouseDirIndicator);

	// navtarget info
	if (Body *navtarget = Pi::player->GetNavTarget()) {
		// if navtarget and body frame are the same,
		// then we hide the frame-relative velocity indicator
		// (which would be hidden underneath anyway)
		if (navtarget == Pi::player->GetFrame()->GetBody())
			HideIndicator(m_velIndicator);

		// navtarget distance/target square indicator (displayed with navtarget label)
		double dist2 = (navtarget->GetTargetIndicatorPosition(cam_frame)
			- Pi::player->GetPositionRelTo(cam_frame)).Length();
		m_navTargetIndicator.label->SetText(navtarget->GetLabel());
		m_navTargetIndicator.label2->SetText(format_distance(dist2).c_str());
		UpdateIndicator(m_navTargetIndicator, navtarget->GetTargetIndicatorPosition(cam_frame));

		// velocity relative to navigation target
		vector3d navvelocity = -navtarget->GetVelocityRelTo(Pi::player);
		double navspeed = navvelocity.Length();
		const vector3d camSpaceNavVel = navvelocity * cam_rot;

		if (navspeed >= 0.01) { // 1 cm per second
			char buf[128];
			if (navspeed > 1000)
				snprintf(buf, sizeof(buf), "%.2f km/s", navspeed*0.001);
			else
				snprintf(buf, sizeof(buf), "%.0f m/s", navspeed);
			m_navVelIndicator.label->SetText(buf);
			if(Pi::AreTargetIndicatorsDisplayed()) {
				UpdateIndicator(m_navVelIndicator, camSpaceNavVel);
			} else {
				HideIndicator(m_navVelIndicator);
			}

			//assert(m_navTargetIndicator.side != INDICATOR_HIDDEN);
			//assert(m_navVelIndicator.side != INDICATOR_HIDDEN);
			SeparateLabels(m_navTargetIndicator.label, m_navVelIndicator.label);
		} else
			HideIndicator(m_navVelIndicator);

	} else {
		HideIndicator(m_navTargetIndicator);
		HideIndicator(m_navVelIndicator);
	}

	// later we might want non-ship enemies (e.g., for assaults on military bases)
	assert(!Pi::player->GetCombatTarget() || Pi::player->GetCombatTarget()->IsType(Object::SHIP));

	// update combat HUD
	Ship *enemy = static_cast<Ship *>(Pi::player->GetCombatTarget());
	if (enemy) {
		char buf[128];
		const vector3d targpos = enemy->GetInterpPositionRelTo(Pi::player) * cam_rot;
		const double dist2 = targpos.Length();
		const vector3d targScreenPos = enemy->GetInterpPositionRelTo(cam_frame);

		snprintf(buf, sizeof(buf), "%.0fm", dist2);
		m_combatTargetIndicator.label->SetText(buf);
		UpdateIndicator(m_combatTargetIndicator, targScreenPos);

		// calculate firing solution and relative velocity along our z axis
		int laser = -1;
		if (GetCamType() == CAM_INTERNAL) {
			switch (m_internalCameraController->GetMode()) {
				case InternalCameraController::MODE_FRONT: laser = 0; break;
				case InternalCameraController::MODE_REAR:  laser = 1; break;
				default: break;
			}
		}

		if (laser >= 0) {
			laser = Pi::player->m_equipment.Get(Equip::SLOT_LASER, laser);
			laser = Equip::types[laser].tableIndex;
		}
		if (laser >= 0) { // only display target lead position on views with lasers
			double projspeed = Equip::lasers[laser].speed;

			const vector3d targvel = enemy->GetVelocityRelTo(Pi::player) * cam_rot;
			vector3d leadpos = targpos + targvel*(targpos.Length()/projspeed);
			leadpos = targpos + targvel*(leadpos.Length()/projspeed); // second order approx

			// now the text speed/distance
			// want to calculate closing velocity that you couldn't counter with retros

			double vel = targvel.Dot(targpos.NormalizedSafe()); // position should be towards
			double raccel =
				Pi::player->GetShipType()->linThrust[ShipType::THRUSTER_REVERSE] / Pi::player->GetMass();

			double c = Clamp(vel / sqrt(2.0 * raccel * dist2), -1.0, 1.0);
			float r = float(0.2+(c+1.0)*0.4);
			float b = float(0.2+(1.0-c)*0.4);

			m_combatTargetIndicator.label->Color(255, 0, 0);
		}
	} else {
		HideIndicator(m_combatTargetIndicator);
	}
}

void WorldView::UpdateIndicator(Indicator &indicator, const vector3d &cameraSpacePos)
{
	const int guiSize[2] = { Gui::Screen::GetWidth(), Gui::Screen::GetHeight() };
	const Graphics::Frustum frustum = m_cameraContext->GetFrustum();

	const float BORDER = 10.0;
	const float BORDER_BOTTOM = 90.0;
	// XXX BORDER_BOTTOM is 10+the control panel height and shouldn't be needed at all

	const float w = Gui::Screen::GetWidth();
	const float h = Gui::Screen::GetHeight();

	if (cameraSpacePos.LengthSqr() < 1e-6) { // length < 1e-3
		indicator.pos.x = w/2.0f;
		indicator.pos.y = h/2.0f;
		indicator.side = INDICATOR_ONSCREEN;
	} else {
		vector3d proj;
		bool success = project_to_screen(cameraSpacePos, proj, frustum, guiSize);
		if (! success)
			proj = vector3d(w/2.0, h/2.0, 0.0);

		indicator.realpos.x = int(proj.x);
		indicator.realpos.y = int(proj.y);

		bool onscreen =
			(cameraSpacePos.z < 0.0) &&
			(proj.x >= BORDER) && (proj.x < w - BORDER) &&
			(proj.y >= BORDER) && (proj.y < h - BORDER_BOTTOM);

		if (onscreen) {
			indicator.pos.x = int(proj.x);
			indicator.pos.y = int(proj.y);
			indicator.side = INDICATOR_ONSCREEN;
		} else {
			// homogeneous 2D points and lines are really useful
			const vector3d ptCentre(w/2.0, h/2.0, 1.0);
			const vector3d ptProj(proj.x, proj.y, 1.0);
			const vector3d lnDir = ptProj.Cross(ptCentre);

			indicator.side = INDICATOR_TOP;

			// this fallback is used if direction is close to (0, 0, +ve)
			indicator.pos.x = w/2.0;
			indicator.pos.y = BORDER;

			if (cameraSpacePos.x < -1e-3) {
				vector3d ptLeft = lnDir.Cross(vector3d(-1.0, 0.0, BORDER));
				ptLeft /= ptLeft.z;
				if (ptLeft.y >= BORDER && ptLeft.y < h - BORDER_BOTTOM) {
					indicator.pos.x = ptLeft.x;
					indicator.pos.y = ptLeft.y;
					indicator.side = INDICATOR_LEFT;
				}
			} else if (cameraSpacePos.x > 1e-3) {
				vector3d ptRight = lnDir.Cross(vector3d(-1.0, 0.0,  w - BORDER));
				ptRight /= ptRight.z;
				if (ptRight.y >= BORDER && ptRight.y < h - BORDER_BOTTOM) {
					indicator.pos.x = ptRight.x;
					indicator.pos.y = ptRight.y;
					indicator.side = INDICATOR_RIGHT;
				}
			}

			if (cameraSpacePos.y < -1e-3) {
				vector3d ptBottom = lnDir.Cross(vector3d(0.0, -1.0, h - BORDER_BOTTOM));
				ptBottom /= ptBottom.z;
				if (ptBottom.x >= BORDER && ptBottom.x < w-BORDER) {
					indicator.pos.x = ptBottom.x;
					indicator.pos.y = ptBottom.y;
					indicator.side = INDICATOR_BOTTOM;
				}
			} else if (cameraSpacePos.y > 1e-3) {
				vector3d ptTop = lnDir.Cross(vector3d(0.0, -1.0, BORDER));
				ptTop /= ptTop.z;
				if (ptTop.x >= BORDER && ptTop.x < w - BORDER) {
					indicator.pos.x = ptTop.x;
					indicator.pos.y = ptTop.y;
					indicator.side = INDICATOR_TOP;
				}
			}
		}
	}
	
	// update the label position
	if (indicator.label) {
		if(Pi::AreTargetIndicatorsDisplayed() || 1) {
			if (indicator.side != INDICATOR_HIDDEN) {
				float labelSize[2] = { 500.0f, 500.0f };
				indicator.label->GetSizeRequested(labelSize);
				if(indicator.label2) {
					indicator.label2->GetSizeRequested(labelSize);
				}

				int pos[2] = {0,0};
				switch (indicator.side) {
				case INDICATOR_HIDDEN: break;
				case INDICATOR_ONSCREEN: // when onscreen, default to label-below unless it would clamp to be on top of the marker
					pos[0] = -(labelSize[0]/2.0f);
					if (indicator.pos.y + pos[1] + labelSize[1] + HUD_CROSSHAIR_SIZE + 2.0f > h - BORDER_BOTTOM)
						pos[1] = -(labelSize[1] + HUD_CROSSHAIR_SIZE + 2.0f);
					else
						pos[1] = HUD_CROSSHAIR_SIZE + 2.0f;
					break;
				case INDICATOR_TOP:
					pos[0] = -(labelSize[0]/2.0f);
					pos[1] = HUD_CROSSHAIR_SIZE + 2.0f;
					break;
				case INDICATOR_LEFT:
					pos[0] = HUD_CROSSHAIR_SIZE + 2.0f;
					pos[1] = -(labelSize[1]/2.0f);
					break;
				case INDICATOR_RIGHT:
					pos[0] = -(labelSize[0] + HUD_CROSSHAIR_SIZE + 2.0f);
					pos[1] = -(labelSize[1]/2.0f);
					break;
				case INDICATOR_BOTTOM:
					pos[0] = -(labelSize[0]/2.0f);
					pos[1] = -(labelSize[1] + HUD_CROSSHAIR_SIZE + 2.0f);
					break;
				}

				pos[0] = Clamp(pos[0] + indicator.pos.x, BORDER, w - BORDER - labelSize[0]);
				pos[1] = Clamp(pos[1] + indicator.pos.y, BORDER, h - BORDER_BOTTOM - labelSize[1]);
				MoveChild(indicator.label, pos[0], pos[1]);
				indicator.label->Show();
				if(indicator.label2) {
					MoveChild(indicator.label2, pos[0], pos[1] + 10);
					indicator.label2->Show();
				}
			} else {
				indicator.label->Hide();
				if(indicator.label2) {
					indicator.label2->Hide();
				}
			}
		} else {
			indicator.label->Hide();
			if(indicator.label2) {
				indicator.label2->Hide();
			}
		}
	}
}

void WorldView::HideIndicator(Indicator &indicator)
{
	indicator.side = INDICATOR_HIDDEN;
	indicator.pos = vector2f(0.0f, 0.0f);
	if (indicator.label)
		indicator.label->Hide();
}

void WorldView::SeparateLabels(Gui::Label *a, Gui::Label *b)
{
	float posa[2], posb[2], sizea[2], sizeb[2];
	GetChildPosition(a, posa);
	a->GetSize(sizea);
	sizea[0] *= 0.5f;
	sizea[1] *= 0.5f;
	posa[0] += sizea[0];
	posa[1] += sizea[1];
	GetChildPosition(b, posb);
	b->GetSize(sizeb);
	sizeb[0] *= 0.5f;
	sizeb[1] *= 0.5f;
	posb[0] += sizeb[0];
	posb[1] += sizeb[1];

	float overlapX = sizea[0] + sizeb[0] - fabs(posa[0] - posb[0]);
	float overlapY = sizea[1] + sizeb[1] - fabs(posa[1] - posb[1]);

	if (overlapX > 0.0f && overlapY > 0.0f) {
		if (overlapX <= 4.0f) {
			// small horizontal overlap; bump horizontally
			if (posa[0] > posb[0]) overlapX *= -1.0f;
			MoveChild(a, posa[0] - overlapX*0.5f - sizea[0], posa[1] - sizea[1]);
			MoveChild(b, posb[0] + overlapX*0.5f - sizeb[0], posb[1] - sizeb[1]);
		} else {
			// large horizonal overlap; bump vertically
			if (posa[1] > posb[1]) overlapY *= -1.0f;
			MoveChild(a, posa[0] - sizea[0], posa[1] - overlapY*0.5f - sizea[1]);
			MoveChild(b, posb[0] - sizeb[0], posb[1] + overlapY*0.5f - sizeb[1]);
		}
	}
}

double getSquareDistance(double initialDist, double scalingFactor, int num) {
	return pow(scalingFactor, num - 1) * num * initialDist;
}

double getSquareHeight(double distance, double angle) {
	return distance * tan(angle);
}

void WorldView::Draw()
{
	assert(Pi::game);
	assert(Pi::player);
	assert(!Pi::player->IsDead());

	m_renderer->ClearDepthBuffer();

	// don't draw crosshairs etc in hyperspace
	if (Pi::player->GetFlightState() == Ship::HYPERSPACE) {
		return;
	} else {
		Graphics::Renderer::LineWidthTicket lwt(m_renderer);
		m_renderer->SetLineWidth(2.0f);

		Color white(255, 255, 255, 204);
		Color green(0, 255, 0, 204);
		Color yellow(230, 230, 77, 255);
		Color red(255, 0, 0, 128);

		// Body icons
		DrawBodyIcons();

		// nav target square
		//DrawTargetSquare(m_navTargetIndicator, green);
		DrawIconIndicator(m_navTargetIndicator, m_hud2TargetSelector.get(), Color::PARAGON_BLUE, vector2f(35.0f, 35.0f));

		m_renderer->SetLineWidth(1.0f);

		// velocity indicators
		if (Pi::AreTargetIndicatorsDisplayed()) {
			DrawVelocityIndicator(m_velIndicator, white);
			DrawVelocityIndicator(m_navVelIndicator, green);
		}

		m_renderer->SetLineWidth(2.0f);

		// Reticle
		vector2f reticle_size, reticle_pos;
		reticle_size.x = HUD_RETICLE_SIZE_X * Gui::Screen::GetWidth();
		reticle_size.y = HUD_RETICLE_SIZE_Y * Gui::Screen::GetHeight();
		reticle_pos.x = (Gui::Screen::GetWidth() - reticle_size.x) / 2.0f;
		reticle_pos.y = (Gui::Screen::GetHeight() - reticle_size.y) / 2.0f;

		if ((GetCamType() == CAM_INTERNAL
			&& m_internalCameraController->GetMode() == InternalCameraController::MODE_FRONT
			&& !m_internalCameraController->IsFreelooking())
			&& m_tutorialDialog == nullptr)
		{
			m_reticle->Draw(reticle_pos, reticle_size);
			if(Pi::cpan) {
				Pi::cpan->SetCircleOverlayVisiblity(true);
			}
		} else {
			if(Pi::cpan) {
				Pi::cpan->SetCircleOverlayVisiblity(false);
			}
		}

		// Mouse dir
		DrawImageIndicator(m_mouseDirIndicator, m_indicatorMousedir.get(), yellow);

		// Selection illustration
		//DrawTargetSquare(m_combatTargetIndicator, green);
		DrawTargetIndicator(m_combatTargetIndicator, red);

		// combat target indicator
		if (Pi::player->TargetInSight()) {
			// Reticle target mode
			m_reticleTarget->Draw(reticle_pos, reticle_size);
		}
	}

    //m_renderer->ClearScreen();

	View::Draw();
}

void WorldView::DrawCrosshair(float px, float py, float sz, const Color &c)
{
	const vector2f vts[] = {
		vector2f(px-sz, py),
		vector2f(px-0.5f*sz, py),
		vector2f(px+sz, py),
		vector2f(px+0.5f*sz, py),
		vector2f(px, py-sz),
		vector2f(px, py-0.5f*sz),
		vector2f(px, py+sz),
		vector2f(px, py+0.5f*sz)
	};
	m_renderer->DrawLines2D(COUNTOF(vts), vts, c, m_blendState);
}

void WorldView::DrawCombatTargetIndicator(const Indicator &target, const Indicator &lead, const Color &c)
{
	if (target.side == INDICATOR_HIDDEN) return;

	if (target.side == INDICATOR_ONSCREEN) {
		float x1 = target.pos.x, y1 = target.pos.y;
		float x2 = lead.pos.x, y2 = lead.pos.y;

		float xd = x2 - x1, yd = y2 - y1;
		if (lead.side != INDICATOR_ONSCREEN) {
			xd = 1.0f; yd = 0.0f;
		} else {
			float len = xd*xd + yd*yd;
			if (len < 1e-6) {
				xd = 1.0f; yd = 0.0f;
			} else {
				len = sqrt(len);
				xd /= len;
				yd /= len;
			}
		}

		const vector2f vts[] = {
			// target crosshairs
			vector2f(x1+10*xd, y1+10*yd),
			vector2f(x1+20*xd, y1+20*yd),
			vector2f(x1-10*xd, y1-10*yd),
			vector2f(x1-20*xd, y1-20*yd),
			vector2f(x1-10*yd, y1+10*xd),
			vector2f(x1-20*yd, y1+20*xd),
			vector2f(x1+10*yd, y1-10*xd),
			vector2f(x1+20*yd, y1-20*xd),

			// lead crosshairs
			vector2f(x2-10*xd, y2-10*yd),
			vector2f(x2+10*xd, y2+10*yd),
			vector2f(x2-10*yd, y2+10*xd),
			vector2f(x2+10*yd, y2-10*xd),

			// line between crosshairs
			vector2f(x1+20*xd, y1+20*yd),
			vector2f(x2-10*xd, y2-10*yd)
		};
		if (lead.side == INDICATOR_ONSCREEN) {
			m_renderer->DrawLines2D(14, vts, c, m_blendState); //draw all
		} else {
			m_renderer->DrawLines2D(8, vts, c, m_blendState); //only crosshair
		}
	} else {
		DrawEdgeMarker(target, c);
	}
}

void WorldView::DrawTargetIndicator(const Indicator &target, const Color &c)
{
	if (target.side == INDICATOR_HIDDEN) return;

	if (target.side == INDICATOR_ONSCREEN) {
		float x1 = target.pos.x + 0.5f, y1 = target.pos.y;
		float xd = 1.0f, yd = 0.0f;
		float inner = 10.0f, outer = 30.0f;
		Gui::Screen::ConvertSizeToUI(inner, outer, inner, outer);

		const vector2f vts[] = {
			// target crosshairs
			vector2f(x1 + inner * xd, y1 + inner * yd),
			vector2f(x1 + outer * xd, y1 + outer * yd),
			vector2f(x1 - inner * xd, y1 - inner * yd),
			vector2f(x1 - outer * xd, y1 - outer * yd),
			vector2f(x1 - inner * yd, y1 + inner * xd),
			vector2f(x1 - outer * yd, y1 + outer * xd),
			vector2f(x1 + inner * yd, y1 - inner * xd),
			vector2f(x1 + outer * yd, y1 - outer * xd),
		};
		m_renderer->SetLineWidth(3.0f);
		m_renderer->DrawLines2D(8, vts, c, m_blendState); //only crosshair
		m_renderer->SetLineWidth(1.0f);
	} else {
		DrawEdgeMarker(target, c);
	}
}

void WorldView::DrawTargetSquare(const Indicator &marker, const Color &c)
{
	if (marker.side == INDICATOR_HIDDEN) return;
	if (marker.side != INDICATOR_ONSCREEN)
		DrawEdgeMarker(marker, c);

	// if the square is off-screen, draw a little square at the edge
	const float sz = (marker.side == INDICATOR_ONSCREEN)
		? float(WorldView::PICK_OBJECT_RECT_SIZE * 0.5) : 3.0f;

	const float x1 = float(marker.pos.x - sz);
	const float x2 = float(marker.pos.x + sz);
	const float y1 = float(marker.pos.y - sz);
	const float y2 = float(marker.pos.y + sz);

	const vector2f vts[] = {
		vector2f(x1, y1),
		vector2f(x2, y1),
		vector2f(x2, y2),
		vector2f(x1, y2)
	};
	m_renderer->DrawLines2D(COUNTOF(vts), vts, c, m_blendState, Graphics::LINE_LOOP);
}

void WorldView::DrawVelocityIndicator(const Indicator &marker, const Color &c)
{
	if (marker.side == INDICATOR_HIDDEN) return;

	const float sz = HUD_CROSSHAIR_SIZE;
	if (marker.side == INDICATOR_ONSCREEN) {
		const float posx = marker.pos.x;
		const float posy = marker.pos.y;
		const vector2f vts[] = {
			vector2f(posx-sz, posy-sz),
			vector2f(posx-0.5f*sz, posy-0.5f*sz),
			vector2f(posx+sz, posy-sz),
			vector2f(posx+0.5f*sz, posy-0.5f*sz),
			vector2f(posx+sz, posy+sz),
			vector2f(posx+0.5f*sz, posy+0.5f*sz),
			vector2f(posx-sz, posy+sz),
			vector2f(posx-0.5f*sz, posy+0.5f*sz)
		};
		m_renderer->DrawLines2D(COUNTOF(vts), vts, c, m_blendState);
	} else
		DrawEdgeMarker(marker, c);

}

void WorldView::DrawImageIndicator(const Indicator &marker, Gui::TexturedQuad *quad, const Color &c)
{
	if (marker.side == INDICATOR_HIDDEN) return;

	if (marker.side == INDICATOR_ONSCREEN) {
		vector2f pos = marker.pos - m_indicatorMousedirSize/2.0f;
		quad->Draw(pos, m_indicatorMousedirSize, c);
	} else
		DrawEdgeMarker(marker, c);
}

void WorldView::DrawIconIndicator(const Indicator &marker, Gui::TexturedQuad* quad, const Color& c, 
	vector2f size_in_px)
{
	if (marker.side == INDICATOR_HIDDEN) return;
	if (marker.side == INDICATOR_ONSCREEN) {
		// Convert to gui dimensions
		float ratio = size_in_px.y / size_in_px.x;
		size_in_px.x = (size_in_px.x / 1920.0f) * static_cast<float>(Gui::Screen::GetWidth());
		size_in_px.y = size_in_px.x * ratio;
		vector2f pos = marker.pos - size_in_px / 2.0f;
		quad->Draw(pos, size_in_px, c);
	} else {
		DrawEdgeMarker(marker, c);
	}
}

void WorldView::DrawBodyIcons()
{
	if (!m_hud2BodyIcons.empty()) {
		for (unsigned int i = 0; i < m_hud2BodyIcons.size(); ++i) {
			DrawBodyIcon(m_hud2BodyIcons[i].type, m_hud2BodyIcons[i].position, 
				m_hud2BodyIcons[i].currentMission);
		}
	}
}

void WorldView::DrawBodyIcon(Object::Type type, vector2f position, bool current_mission) 
{
	vector2f size(20.0f, 20.0f);
	Gui::TexturedQuad* tquad = m_hud2Unknown.get();
	Color color = Color::PARAGON_GREEN;
	switch (type) {
		case Object::Type::SHIP:
			tquad = m_hud2ShipIndicator.get();
			break;

		case Object::Type::CITYONPLANET:
			tquad = m_hud2Settlement.get();
			break;

		case Object::Type::CARGOBODY:
			tquad = m_hud2ShipIndicator.get();
			break;

		case Object::Type::SPACESTATION:
			tquad = m_hud2Station.get();
			break;

		case Object::Type::STAR:
			tquad = m_hud2Star.get();
			break;

		case Object::Type::PLANET:
		case Object::Type::TERRAINBODY:
			tquad = m_hud2Planet.get();
			break;

		case Object::Type::HYPERSPACECLOUD:
			tquad = m_hud2HyperspaceCloud.get();
			break;

		case Object::Type::PROJECTILE:
		case Object::Type::MISSILE:
		case Object::Type::PLAYER:
			tquad = nullptr;
	}
	if (tquad) {
		Gui::Screen::ConvertSizeToUI(size.x, size.y, size.x, size.y);
		vector2f pos = position - size / 2.0f;
		tquad->Draw(pos, size, color);

		if (current_mission) {
			vector2f pos1(pos.x, pos.y - (size.y * 1.1));
			vector2f pos2(pos.x, pos.y + (size.y * 1.1));
			m_hud2CurrentMission1->Draw(pos1, size, Color::PARAGON_BLUE);
			m_hud2CurrentMission2->Draw(pos2, size, Color::PARAGON_BLUE);
		}
	}
}

void WorldView::DrawEdgeMarker(const Indicator &marker, const Color &c)
{
	const float sz = HUD_CROSSHAIR_SIZE;

	const vector2f screenCentre(Gui::Screen::GetWidth()/2.0f, Gui::Screen::GetHeight()/2.0f);
	vector2f dir = screenCentre - marker.pos;
	float len = dir.Length();
	dir *= sz/len;
	const vector2f vts[] = { marker.pos, marker.pos + dir };
	m_renderer->DrawLines2D(2, vts, c, m_blendState);
}

void WorldView::MouseWheel(bool up)
{
	if (this == Pi::GetView())
	{
		if (m_activeCameraController->IsExternal()) {

			if (!up) {	// Zoom out
				m_activeCameraController->ZoomEvent(ZOOM_SPEED * WHEEL_SENSITIVITY);
			} else {
				m_activeCameraController->ZoomEvent(-ZOOM_SPEED * WHEEL_SENSITIVITY);
			}
		}
	}
}

std::string WorldView::PadWithZeroes(int number, int desired_digits)
{
	std::ostringstream ss;
	ss << number;
	int pad_zeroes = std::max<int>(0, desired_digits - ss.str().length());
	if(pad_zeroes > 0) {
		ss.str("");
		ss.clear();
		for(int i = 0; i < pad_zeroes; ++i) {
			ss << "0";
		}
		ss << number;
	}
	return ss.str();
}

double WorldView::Unjitter(double value, double max_value)
{
	// Anything within 1% of the max value will be replaced by maximum value
	if(abs(max_value - value) <= max_value * 0.01) {
		return max_value; 
	} else {
		return value;
	}
}

void WorldView::GetNavIconFile(Object::Type type, std::string &fileLocation)
{
	switch (type) {
	case Object::Type::SHIP:
		fileLocation = "icons/hud2/ship.png";
		break;

	case Object::Type::CITYONPLANET:
		fileLocation = "icons/hud2/planet.png";
		break;

	case Object::Type::CARGOBODY:
		fileLocation = "icons/hud2/planet.png";
		break;

	case Object::Type::SPACESTATION:
		fileLocation = "icons/hud2/station.png";
		break;

	case Object::Type::STAR:
		fileLocation = "icons/hud2/star.png";
		break;

	case Object::Type::PLANET:
		fileLocation = "icons/hud2/planet.png";
		break;
	case Object::Type::TERRAINBODY:
		fileLocation = "icons/hud2/planet.png";
		break;

	case Object::Type::HYPERSPACECLOUD:
		fileLocation = "icons/hud2/unknown.png";
		break;

	case Object::Type::PROJECTILE:
		fileLocation = "icons/hud2/unknown.png";
		break;
	case Object::Type::MISSILE:
		fileLocation = "icons/hud2/unknown.png";
		break;
	case Object::Type::PLAYER:
		fileLocation = "icons/hud2/unknown.png";
		break;
	default:
		fileLocation = "icons/hud2/unknown.png";
		break;
	}
}

void WorldView::CenterAutopilotOptions(){
	m_commsNavOptions->ShowAll();
	m_commsNavOptionsContainer->ShowAll();

	m_commsOptions->ResizeToFitChildren();
	m_commsNavOptions->ResizeToFitChildren();

	float navPos[2], navSize[2], navCSize[2], commSize[2], scrollSize[2], size[2];
	GetSizeRequested(size);
	m_commsNavOptionsContainer->GetSize(navCSize);
	m_commsNavOptions->GetSize(navSize);
	m_commsOptions->GetSize(commSize);
	m_scroll->GetSize(scrollSize);
	m_scroll->SetOffsetY(navCSize[1] - navSize[1]);
	MoveChild(m_commsNavOptionsContainer, 10, 17 * 2 /* see below */ + (size[1] / 2.0f) - ((scrollSize[1] - m_scroll->GetOffsetY()) / 2.0f));
	m_commsNavOptionsContainer->GetAbsolutePosition(navPos);  // This seems faulty, it gives me a position that is wrong by 17 pixels. (might not be this function but the move function, to investigate (seems to be only on the Y axis))
	MoveChild(m_commsOptions, navPos[0] + navCSize[0] + 10, ((m_scroll->GetOffsetY() > 135.0f) ? (-45.0f / 4.0f) : 0.0f) + navPos[1] - 17.0f /* Magical number to remove once I fix the get position function above */ + ((scrollSize[1] - m_scroll->GetOffsetY()) / 2.0f) - (commSize[1] / 2.0f));

	m_commsOptions->ShowAll();
}

void WorldView::UpdateContactList() {
	Gui::Screen::PushFont("HudFont");
    const std::vector<std::string> contactShips = Pi::player->GetNamesOfNearbyShips();

    m_contactListContainer->RemoveChild(m_contactListButton);

    m_contactListContainer->DeleteAllChildren();

    m_contactListContainer->PackStart(m_contactListButton);

    for (size_t i = 0; i < contactShips.size(); i++) {
        Gui::Label* shipLabel = new Gui::Label("  " + contactShips[i]);
        shipLabel->Color(Color::PARAGON_BLUE);
        m_contactListContainer->PackEnd(shipLabel);
        if (!m_longContactList && i == CONTACT_LIST_LIMIT) {
            break;
        }
    }

    float contSize[2], screenSize[2];
    GetSizeRequested(screenSize);
    m_contactListContainer->GetSize(contSize);
    Gui::Label* contactLabel;
    if (CONTACT_LIST_LIMIT + 1 < contactShips.size() && !m_longContactList) {
        contactLabel = new Gui::Label("+ Contact List +");
        MoveChild(m_contactListContainer, screenSize[0] - 110, screenSize[1] - 15);
    }
    else if (CONTACT_LIST_LIMIT + 1 < contactShips.size()) {
        contactLabel = new Gui::Label("- Contact List -");
        MoveChild(m_contactListContainer, screenSize[0] - 110, screenSize[1] - contSize[1] / 2);
    }
    else {
        contactLabel = new Gui::Label("  Contact List  ");
        MoveChild(m_contactListContainer, screenSize[0] - 110, screenSize[1] - 15);
    }
    contactLabel->Color(Color::PARAGON_GREEN);
    m_contactListButton->SetLabel(contactLabel);

    m_contactListContainer->ShowAll();

    m_contactListContainer->ResizeToFitChildren();






    Gui::Screen::PopFont();
}

void WorldView::ToggleContactListSize() {
    //m_longContactList = !m_longContactList;
    UpdateContactList();
}

void WorldView::UpdateFastTravel() {
	if (m_fastTravelTimeout != 0) {
        if (SDL_GetTicks() - m_fastTravelTimeout < 5000) {
            Pi::player->SetFrame(m_fastTravelTarget->GetFrame());
            double radius = m_fastTravelTarget->GetFrame()->GetRadius() / 2.0;
            if (radius > 10000.0)
                radius = 10000.0;
            vector3d offset(m_fastTravelTarget->GetOrient().VectorY() * radius);
            Pi::player->SetPosition(vector3d(m_fastTravelTarget->GetPosition().x + offset.x, m_fastTravelTarget->GetPosition().y + offset.y, m_fastTravelTarget->GetPosition().z + offset.z));
            Pi::player->GetPlayerController()->SetFlightControlState(CONTROL_AUTOPILOT);
            if (isDocking) {
                Pi::player->AIDock(static_cast<SpaceStation*>(m_fastTravelTarget));
            }
            else {
                Pi::player->AIFlyTo(m_fastTravelTarget);
            }

            Pi::player->TmpSetVelocity(vector3d(0.0, 0.0, 0.0));
        }
        else if (SDL_GetTicks() - m_fastTravelTimeout > 10000) {
            m_fastTravelTimeout = 0;
            Pi::game->SetFastTravel(false);
            Pi::player->SetInvulnerable(false);
        }
        else {
            Pi::player->TmpSetVelocity(vector3d(0.0, 0.0, 0.0));
        }
    }
    else if (m_fastTravelTarget && Pi::player->GetFlightState() == Ship::FlightState::HYPERSPACE) {
        m_fastTravelTarget = nullptr;
    }

}

////////////////////////////////////////////////////////////////////////////////////
//                               NAV TUNNEL WIDGET                                //
////////////////////////////////////////////////////////////////////////////////////

NavTunnelWidget::NavTunnelWidget(WorldView *worldview, Graphics::RenderState *rs)
	: Widget()
	, m_worldView(worldview)
	, m_renderState(rs)
{
}

void NavTunnelWidget::Draw() {
	if (!Pi::IsNavTunnelDisplayed()) return;

	Body *navtarget = Pi::player->GetNavTarget();
	if (navtarget) {
		const vector3d navpos = navtarget->GetPositionRelTo(Pi::player);
		const matrix3x3d &rotmat = Pi::player->GetOrient();
		const vector3d eyevec = rotmat * m_worldView->m_activeCameraController->GetOrient().VectorZ();
		if (eyevec.Dot(navpos) >= 0.0) return;

		const Color green = Color(0, 255, 0, 204);

		const double distToDest = Pi::player->GetPositionRelTo(navtarget).Length();

		const int maxSquareHeight = std::max(Gui::Screen::GetWidth(), Gui::Screen::GetHeight()) / 2;
		const double angle = atan(maxSquareHeight / distToDest);
		const vector2f tpos(m_worldView->m_navTargetIndicator.realpos);
		const vector2f distDiff(tpos - vector2f(Gui::Screen::GetWidth() / 2.0f, Gui::Screen::GetHeight() / 2.0f));

		double dist = 0.0;
		const double scalingFactor = 1.6; // scales distance between squares: closer to 1.0, more squares
		for (int squareNum = 1; ; squareNum++) {
			dist = getSquareDistance(10.0, scalingFactor, squareNum);
			if (dist > distToDest)
				break;

			const double sqh = getSquareHeight(dist, angle);
			if (sqh >= 10) {
				const vector2f off = distDiff * (dist / distToDest);
				const vector2f sqpos(tpos-off);
				DrawTargetGuideSquare(sqpos, sqh, green);
			}
		}
	}
}

void NavTunnelWidget::DrawTargetGuideSquare(const vector2f &pos, const float size, const Color &c)
{
	const float x1 = pos.x - size;
	const float x2 = pos.x + size;
	const float y1 = pos.y - size;
	const float y2 = pos.y + size;

	const vector3f vts[] = {
		vector3f(x1,    y1,    0.f),
		vector3f(pos.x, y1,    0.f),
		vector3f(x2,    y1,    0.f),
		vector3f(x2,    pos.y, 0.f),
		vector3f(x2,    y2,    0.f),
		vector3f(pos.x, y2,    0.f),
		vector3f(x1,    y2,    0.f),
		vector3f(x1,    pos.y, 0.f)
	};
	Color black(c);
	black.a = c.a / 6;
	const Color col[] = {
		c,
		black,
		c,
		black,
		c,
		black,
		c,
		black
	};
	assert(COUNTOF(col) == COUNTOF(vts));
	m_worldView->m_renderer->DrawLines(COUNTOF(vts), vts, col, m_renderState, Graphics::LINE_LOOP);
}

void NavTunnelWidget::GetSizeRequested(float size[2]) {
	size[0] = Gui::Screen::GetWidth();
	size[1] = Gui::Screen::GetHeight();
}
