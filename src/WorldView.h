// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _WORLDVIEW_H
#define _WORLDVIEW_H

#include "libs.h"
#include "gui/Gui.h"
#include "gui/GuiWidget.h"
#include "UIView.h"
#include "Serializer.h"
#include "SpeedLines.h"
#include "Background.h"
#include "EquipType.h"
#include "Camera.h"
#include "CameraController.h"
#include "TweakerSettings.h"

class Body;
class Frame;
class LabelSet;
class Ship;
class NavTunnelWidget;
namespace Gui { class TexturedQuad; }

class WorldView: public UIView {
public:
	friend class NavTunnelWidget;
	WorldView();
	WorldView(Serializer::Reader &reader);
	virtual ~WorldView();
	virtual void ShowAll();
	virtual void Update();
	virtual void Draw3D();
	virtual void Draw();
	static const double PICK_OBJECT_RECT_SIZE;
	virtual void Save(Serializer::Writer &wr);
	enum CamType {
		CAM_INTERNAL,
		CAM_EXTERNAL,
		CAM_SIDEREAL
	};
	void SetCamType(enum CamType);
	enum CamType GetCamType() const { return m_camType; }
	CameraController *GetCameraController() const { return m_activeCameraController; }
	void ToggleTargetActions();
	void ShowTargetActions();
	void HideTargetActions();
	int GetActiveWeapon() const;
	void OnClickBlastoff();
	void OnClickTutorial();
	bool IsAltitudeAvailable() const { return m_bAltitudeAvailable; }
	double GetAltitude() const { return m_altitude; }

	sigc::signal<void> onChangeCamType;
	const TS_HypercloudVisual& GetHyperCloudTweak() const; 

protected:
	virtual void OnSwitchTo();
	virtual void OnSwitchFrom();
private:
	void InitObject();

	void DefineTweaks();
	void RemoveTweaks();

	void RefreshHyperspaceButton();
	void RefreshButtonStateAndVisibility();
	void RefreshFreightTeleporterState();
	void RefreshLocationInfo();
	void RefreshOverlayHud();
	void UpdateCommsOptions();

	void ChangeInternalCameraMode(InternalCameraController::Mode m);
	
	void CenterAutopilotOptions();
    void UpdateContactList();

	enum IndicatorSide {
		INDICATOR_HIDDEN,
		INDICATOR_ONSCREEN,
		INDICATOR_LEFT,
		INDICATOR_RIGHT,
		INDICATOR_TOP,
		INDICATOR_BOTTOM
	};

	struct Indicator {
		vector2f pos;
		vector2f realpos;
		IndicatorSide side;
		Gui::Label *label;
		Gui::Label *label2;
		Indicator(): pos(0.0f, 0.0f), realpos(0.0f, 0.0f), side(INDICATOR_HIDDEN), label(0), label2(0) {}
	};

	void UpdateProjectedObjects();
	void UpdateIndicator(Indicator &indicator, const vector3d &direction);
	void HideIndicator(Indicator &indicator);
	void SeparateLabels(Gui::Label *a, Gui::Label *b);

	void OnToggleLabels();

	void DrawCrosshair(float px, float py, float sz, const Color &c);
	void DrawCombatTargetIndicator(const Indicator &target, const Indicator &lead, const Color &c);
	void DrawTargetIndicator(const Indicator &target, const Color &c);
	void DrawTargetSquare(const Indicator &marker, const Color &c);
	void DrawVelocityIndicator(const Indicator &marker, const Color &c);
	void DrawImageIndicator(const Indicator &marker, Gui::TexturedQuad *quad, const Color &c); 
	// Size is in 1920x1080 display
	void DrawIconIndicator(const Indicator &marker, Gui::TexturedQuad* quad, const Color& c, 
		vector2f size_in_px);
	void DrawBodyIcons();
	void DrawBodyIcon(Object::Type type, vector2f position, bool current_mission = false);
	void DrawEdgeMarker(const Indicator &marker, const Color &c);
	void DrawThrusterTrails();

	Gui::Button *AddCommsOption(const std::string msg, int ypos, int optnum);
	void AddCommsNavOption(const std::string msg, Body *target);
	void OnClickCommsNavOption(Body *target, Gui::ClickableLabel *button);
	void BuildCommsNavOptions();

	void HideLowThrustPowerOptions();
	void ShowLowThrustPowerOptions();
	void OnSelectLowThrustPower(float power);

	// Paragon flight system
	void OnClickAutopilotButton(Gui::MultiStateImageButton *b);
	void OnClickManeuverButton(Gui::MultiStateImageButton *b);
	void OnClickTransitButton(Gui::MultiStateImageButton *b);
	void OnClickJumpButton(Gui::MultiStateImageButton *b);
	void ShowParagonFlightButtons();
	void HideParagonFlightButtons();

	void OnClickHyperspace();
	void OnHyperspaceTargetChanged();
	void OnPlayerDockOrUndock();
	void OnPlayerChangeTarget();
	void OnPlayerChangeFlightControlState();
	void SelectBody(Body *, bool reselectIsDeselect);
	void DeselectAll();
	Body* PickBody(const double screenX, const double screenY) const;
	void MouseWheel(bool up);

	std::string PadWithZeroes(int number, int desired_digits);
	double Unjitter(double speed, double max_speed);
	bool CheckPhaseJumpMode();

	void GetNavIconFile(Object::Type type, std::string &fileLocation);
    void ToggleContactListSize();
    void UpdateFastTravel();

	NavTunnelWidget *m_navTunnel;
	std::unique_ptr<SpeedLines> m_speedLines;

	Gui::ImageButton *m_hyperspaceButton;

	Gui::Label *m_pauseText;
	Gui::Fixed *m_commsOptions;
	Gui::VBox *m_commsNavOptions;
	Gui::HBox *m_commsNavOptionsContainer;
	Gui::Fixed *m_lowThrustPowerOptions;
	Gui::Label *m_flightStatus, *m_debugText;
	Gui::ImageButton *m_launchButton;
    Gui::ClickableLabel *m_contactListButton;

	Gui::MultiStateImageButton *m_flightAutopilotButton;
	Gui::MultiStateImageButton *m_flightManeuverButton;
	Gui::MultiStateImageButton *m_flightTransitButton;
	Gui::MultiStateImageButton *m_flightJumpButton;

	Gui::Fixed *m_tutorialDialog;

	// Used to cache altitude calculation in WorldView::RefreshButtonStateAndVisibility()
	bool m_bAltitudeAvailable;
	double m_altitude;

	std::string m_overlayBuffer [4];
	std::string m_circleTitleBuffer[3];
	std::string m_circleDataBuffer[3];
	std::string m_devBuffer;

	bool m_labelsOn;
	enum CamType m_camType;
	Uint32 m_showTargetActionsTimeout;
	Uint32 m_showLowThrustPowerTimeout;
	bool m_tutorialSeen;
    bool m_longContactList = false;

#if WITH_DEVKEYS
	Gui::Label *m_debugInfo;
#endif

	Gui::Label *m_hudHyperspaceInfo;
	Gui::Label *m_hudLocationPrimary, *m_hudLocationSecondary;
	std::vector<Gui::Label*> m_vHudTargetInfo;
	Gui::Label *m_hudPlayerShip, *m_hudTargetShip, *m_hudTargetDesc, *m_hudTargetCargo;
	Gui::MeterBar *m_hudHullTemp, *m_hudWeaponTemp, *m_hudHullIntegrity, *m_hudShieldIntegrity;
	Gui::MeterBar *m_hudTargetHullIntegrity, *m_hudTargetShieldIntegrity, *m_hudTargetFuel;
	Gui::MeterBar *m_hudFuelGauge, *m_hudHydrogenGauge;

	sigc::connection m_onHyperspaceTargetChangedCon;
	sigc::connection m_onPlayerChangeTargetCon;
	sigc::connection m_onChangeFlightControlStateCon;
	sigc::connection m_onMouseWheelCon;

	Gui::LabelSet *m_bodyLabels;
	std::map<Body*,vector3d> m_projectedPos;

	RefCountedPtr<CameraContext> m_cameraContext;
	std::unique_ptr<Camera> m_camera;
	std::unique_ptr<InternalCameraController> m_internalCameraController;
	std::unique_ptr<ExternalCameraController> m_externalCameraController;
	std::unique_ptr<SiderealCameraController> m_siderealCameraController;
	CameraController *m_activeCameraController; //one of the above

	Indicator m_velIndicator;
	Indicator m_navVelIndicator;
	Indicator m_navTargetIndicator;
	Indicator m_combatTargetIndicator;
	Indicator m_mouseDirIndicator;

	std::unique_ptr<Gui::TexturedQuad> m_indicatorMousedir;
	vector2f m_indicatorMousedirSize;
	std::unique_ptr<Gui::TexturedQuad> m_reticle;
	std::unique_ptr<Gui::TexturedQuad> m_reticleTarget;
	Graphics::RenderState *m_blendState;

	std::unique_ptr<Gui::TexturedQuad> m_hud2Planet;
	std::unique_ptr<Gui::TexturedQuad> m_hud2Settlement;
	std::unique_ptr<Gui::TexturedQuad> m_hud2Ship;
	std::unique_ptr<Gui::TexturedQuad> m_hud2ShipIndicator;
	std::unique_ptr<Gui::TexturedQuad> m_hud2ShipOffscreen;
	std::unique_ptr<Gui::TexturedQuad> m_hud2Star;
	std::unique_ptr<Gui::TexturedQuad> m_hud2Station;
	std::unique_ptr<Gui::TexturedQuad> m_hud2HyperspaceCloud;
	std::unique_ptr<Gui::TexturedQuad> m_hud2TargetOffscreen;
	std::unique_ptr<Gui::TexturedQuad> m_hud2TargetSelector;
	std::unique_ptr<Gui::TexturedQuad> m_hud2Unknown;
	std::unique_ptr<Gui::TexturedQuad> m_hud2CurrentMission1;
	std::unique_ptr<Gui::TexturedQuad> m_hud2CurrentMission2;

	struct SBodyIcon
	{
		SBodyIcon(float _x, float _y, Object::Type _type, bool _current_mission = false) {
			position.x = _x;
			position.y = _y;
			type = _type;
			currentMission = _current_mission;
		}
		vector2f position;
		Object::Type type;
		bool currentMission;
	};
	std::vector<SBodyIcon> m_hud2BodyIcons;

	std::unique_ptr<Graphics::Material> m_trailDepthMtrl;
	std::unique_ptr<Graphics::RenderTarget> m_trailDepthRT;
	std::unique_ptr<Graphics::Material> m_trailMtrl;
	int m_windowSizeUniformId;
	Graphics::Texture* m_trailGradient;
	Gui::NavVScrollBar* m_scroll;
    Gui::VBox *m_contactListContainer;

};

class NavTunnelWidget: public Gui::Widget {
public:
	NavTunnelWidget(WorldView *worldView, Graphics::RenderState*);
	virtual void Draw();
	virtual void GetSizeRequested(float size[2]);
	void DrawTargetGuideSquare(const vector2f &pos, const float size, const Color &c);

private:
	WorldView *m_worldView;
	Graphics::RenderState *m_renderState;
};

#endif /* _WORLDVIEW_H */
