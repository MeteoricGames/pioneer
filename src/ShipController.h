// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SHIPCONTROLLER_H
#define _SHIPCONTROLLER_H
/*
 * Ship movement controller class
 * Controls thrusters, autopilot according to player input or AI
 */
#include "libs.h"
#include "Serializer.h"

class Ship;
class Space;

enum FlightControlState {
	CONTROL_MANEUVER,
	//CONTROL_MANUAL,
	CONTROL_AUTOPILOT,
	CONTROL_TRANSIT,

	CONTROL_STATE_COUNT
};

enum FlightButtonStatus {
	FLIGHT_BUTTON_UNAVAILABLE	= 0,
	FLIGHT_BUTTON_OFF			= 1,
	FLIGHT_BUTTON_ON			= 2,
};

// Mouse flight deadzone is in pixels offset from center of screen
static const int MOUSE_FLIGHT_DEADZONE_X = 20;
static const int MOUSE_FLIGHT_DEADZONE_Y = 15;

// only AI
class ShipController
{
	friend class Ship;
public:
	//needed for serialization
	enum Type {
		AI = 0,
		PLAYER = 1
	};
	ShipController() : m_setSpeed(0.0) { }
	virtual ~ShipController() { }
	virtual Type GetType() { return AI; }
	virtual void Save(Serializer::Writer &wr, Space *s) {
		wr.Double(m_setSpeed);
	}
	virtual void Load(Serializer::Reader &rd) { 
		m_setSpeed = rd.Double();
	}
	virtual void PostLoadFixup(Space *) { }
	virtual void StaticUpdate(float timeStep);
	virtual void SetFlightControlState(FlightControlState s) { }
	double GetSpeedLimit() const { return m_setSpeed; }
	void SetSpeedLimit(double set_speed) { m_setSpeed = set_speed; }

protected:
	Ship *m_ship;
	double m_setSpeed;
};

// autopilot AI + input
class PlayerShipController : public ShipController
{
public:
	PlayerShipController();
	~PlayerShipController();
	virtual Type GetType() { return PLAYER; }
	void Save(Serializer::Writer &wr, Space *s);
	void Load(Serializer::Reader &rd);
	void PostLoadFixup(Space *s);
	void StaticUpdate(float timeStep);
	// Poll controls, set thruster states, gun states and target velocity
	void PollControls(float timeStep, const bool force_rotation_damping);
	bool IsMouseActive() const { return m_mouseActive; }
	FlightControlState GetFlightControlState() const { return m_flightControlState; }
	vector3d GetMouseDir() const { return m_mouseDir; }
	void SetMouseForRearView(bool enable) { m_invertMouse = enable; }
	void SetFlightControlState(FlightControlState s);
	float GetLowThrustPower() const { return m_lowThrustPower; }
	void SetLowThrustPower(float power);

	bool GetRotationDamping() const { return m_rotationDamping; }
	void SetRotationDamping(bool enabled);
	void ToggleRotationDamping();
	void FireMissile();

	//targeting
	//XXX AI should utilize one or more of these
	Body *GetCombatTarget() const;
	Body *GetNavTarget() const;
	Body *GetSetSpeedTarget() const;
	void SetCombatTarget(Body* const target, bool setSpeedTo = false);
	void SetNavTarget(Body* const target, bool setSpeedTo = false);

	sigc::signal<void> onRotationDampingChanged;

	bool GetMouseFlightMode() const { return m_mouseFlightToggle; }
	bool GetMouseFlightZeroOffset() const { return m_mouseFlightZeroOffset; }

private:
	bool IsAnyAngularThrusterKeyDown();
	bool IsAnyAxisDown();
	bool IsAnyLinearThrusterKeyDown();
	//do a variety of checks to see if input is allowed
	void CheckControlsLock();
	void OnPlayerChangeFlightMode();

	Body* m_combatTarget;
	Body* m_navTarget;
	Body* m_setSpeedTarget;
	bool m_controlsLocked;
	bool m_invertMouse; // used for rear view, *not* for invert Y-axis option (which is Pi::IsMouseYInvert)
	bool m_mouseActive;
	bool m_rotationDamping;
	double m_mouseX;
	double m_mouseY;
	FlightControlState m_flightControlState;
	float m_fovY; //for mouse acceleration adjustment
	float m_joystickDeadzone;
	float m_lowThrustPower;
	int m_combatTargetIndex; //for PostLoadFixUp
	int m_navTargetIndex;
	int m_setSpeedTargetIndex;
	vector3d m_mouseDir;
	bool m_mouseFlightToggle;		// when true mouse flight is active
	bool m_mouseFlightZeroOffset;	// when true mouse flight is active but mouse is in deadzone (zero offset)
	bool m_prevRightMouseButtonState;

	sigc::connection m_connRotationDampingToggleKey;
	sigc::connection m_fireMissileKey;
	//sigc::connection m_onPlayerChangeFlightMode;
};

#endif
