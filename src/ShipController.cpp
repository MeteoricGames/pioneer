// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "ShipController.h"
#include "Frame.h"
#include "Game.h"
#include "KeyBindings.h"
#include "Pi.h"
#include "Player.h"
#include "Ship.h"
#include "Space.h"
#include "WorldView.h"
#include "OS.h"
#include "MouseCursor.h"

void ShipController::StaticUpdate(float timeStep)
{
	OS::EnableFPE();
	m_ship->AITimeStep(timeStep);
	OS::DisableFPE();
}

PlayerShipController::PlayerShipController() :
	ShipController(),
	m_combatTarget(0),
	m_navTarget(0),
	m_setSpeedTarget(0),
	m_controlsLocked(false),
	m_invertMouse(false),
	m_mouseActive(false),
	m_rotationDamping(true),
	m_mouseX(0.0),
	m_mouseY(0.0),
	m_flightControlState(CONTROL_MANEUVER),
	m_lowThrustPower(0.25), // note: overridden by the default value in GameConfig.cpp (DefaultLowThrustPower setting)
	m_mouseDir(0.0),
	m_mouseFlightToggle(false),
	m_mouseFlightZeroOffset(true),
	m_prevRightMouseButtonState(false)
{
	float deadzone = Pi::config->Float("JoystickDeadzone");
	m_joystickDeadzone = deadzone * deadzone;
	m_fovY = Pi::config->Float("FOVVertical");
	m_lowThrustPower = Pi::config->Float("DefaultLowThrustPower");

	m_connRotationDampingToggleKey = KeyBindings::toggleRotationDamping.onPress.connect(
			sigc::mem_fun(this, &PlayerShipController::ToggleRotationDamping));

	m_fireMissileKey = KeyBindings::fireMissile.onPress.connect(
			sigc::mem_fun(this, &PlayerShipController::FireMissile));

	//m_onPlayerChangeFlightMode = player->OnPlayerChangeFlightModeSignal().connect(
	//	sigc::mem_fun(this, &PlayerShipController::OnPlayerChangeFlightMode));

}

PlayerShipController::~PlayerShipController()
{
	m_connRotationDampingToggleKey.disconnect();
	m_fireMissileKey.disconnect();
}

void PlayerShipController::Save(Serializer::Writer &wr, Space *space)
{
	ShipController::Save(wr, space);
	wr.Int32(static_cast<int>(m_flightControlState));
	wr.Float(m_lowThrustPower);
	wr.Bool(m_rotationDamping);
	wr.Int32(space->GetIndexForBody(m_combatTarget));
	wr.Int32(space->GetIndexForBody(m_navTarget));
	wr.Int32(space->GetIndexForBody(m_setSpeedTarget));
}

void PlayerShipController::Load(Serializer::Reader &rd)
{
	if (Game::s_loadedGameVersion >= 75) { // Save game upgrade 74 -> 75
		ShipController::Load(rd);
	}
	m_flightControlState = static_cast<FlightControlState>(rd.Int32());
	m_lowThrustPower = rd.Float();
	m_rotationDamping = rd.Bool();
	//figure out actual bodies in PostLoadFixup - after Space body index has been built
	m_combatTargetIndex = rd.Int32();
	m_navTargetIndex = rd.Int32();
	m_setSpeedTargetIndex = rd.Int32();
}

void PlayerShipController::PostLoadFixup(Space *space)
{
	m_combatTarget = space->GetBodyByIndex(m_combatTargetIndex);
	m_navTarget = space->GetBodyByIndex(m_navTargetIndex);
	m_setSpeedTarget = space->GetBodyByIndex(m_setSpeedTargetIndex);
}

void PlayerShipController::StaticUpdate(const float timeStep)
{
	vector3d v;
	matrix4x4d m;
	bool any_linthrust_keydown = IsAnyLinearThrusterKeyDown();
	bool any_axis = IsAnyAxisDown();
	bool any_angthrust_keydown = IsAnyAngularThrusterKeyDown() || !GetMouseFlightZeroOffset() 
		|| KeyBindings::decreaseSpeed.IsActive() || any_axis;

	// Get altitude from WorldView (that's where altitude is being calculated for UI display
	// and since it's a bit expensive retrieving it is better than calculating it twice per frame)
	double altitude = -1.0;
	if(Pi::worldView && Pi::worldView->IsAltitudeAvailable()) {
		altitude = Pi::worldView->GetAltitude();
	}

	if (m_ship->GetFlightState() == Ship::FLYING) {

		switch (m_flightControlState) {
			case CONTROL_MANEUVER:
				PollControls(timeStep, true);
				if(m_ship->GetLaunchLockTimeout() <= 0.0f) {
					if (any_linthrust_keydown) break;
					m_ship->ManeuverVelocity();
				}
				break;

			case CONTROL_TRANSIT:
				PollControls(timeStep, true);
				if(m_ship->GetLaunchLockTimeout() <= 0.0f) {
					m_ship->TransitVelocity(timeStep, altitude, any_angthrust_keydown);
				}
				if (m_ship->GetTransitState() == TRANSIT_DRIVE_OFF
					&& m_ship->GetFlightMode() == Ship::EFM_MANEUVER) {
					SetFlightControlState(CONTROL_MANEUVER);
				}
				break;

			/*case CONTROL_MANUAL:
				PollControls(timeStep, false);
				break;*/

			case CONTROL_AUTOPILOT:
				if (m_ship->AIIsActive()) break;
				Pi::game->RequestTimeAccel(Game::TIMEACCEL_1X);
	//			AIMatchVel(vector3d(0.0));			// just in case autopilot doesn't...
							// actually this breaks last timestep slightly in non-relative target cases
				m_ship->AIMatchAngVelObjSpace(vector3d(0.0));
				SetFlightControlState(CONTROL_MANEUVER);
				m_setSpeed = 0.0;
				break;

			default: 
				assert(0); 
				break;
		}
	} else {
		SetFlightControlState(CONTROL_MANEUVER);
	}

	//call autopilot AI, if active (also applies to set speed and heading lock modes)
	//OS::EnableFPE();
	m_ship->AITimeStep(timeStep);
	//OS::DisableFPE();
}

void PlayerShipController::CheckControlsLock()
{
	m_controlsLocked = (Pi::game->IsPaused())
		|| Pi::player->IsDead()
		|| (m_ship->GetFlightState() != Ship::FLYING)
		|| Pi::IsConsoleActive()
		|| (Pi::GetView() != Pi::worldView); //to prevent moving the ship in starmap etc.
}

// mouse wraparound control function
static double clipmouse(double cur, double inp)
{
	if (cur*cur > 0.7 && cur*inp > 0) return 0.0;
	if (inp > 0.2) return 0.2;
	if (inp < -0.2) return -0.2;
	return inp;
}

void PlayerShipController::PollControls(const float timeStep, const bool force_rotation_damping)
{
	static bool stickySpeedKey = false;

	CheckControlsLock();
	if (m_controlsLocked) return;

	// if flying
	{
		m_ship->ClearThrusterState();
		m_ship->SetGunState(0,0);
		m_ship->SetGunState(1,0);

		vector3d wantAngVel(0.0);
		double angThrustSoftness = 10.0;

		// Manual thruster controls are removed in Paragon
		//const float linearThrustPower = (KeyBindings::thrustLowPower.IsActive() ? m_lowThrustPower : 1.0f);
		const float linearThrustPower = 1.0f;

		// have to use this function. SDL mouse position event is bugged in windows
		int mouseMotion[2];
		SDL_GetRelativeMouseState (mouseMotion+0, mouseMotion+1);	// call to flush
		bool rightMouseButtonState = Pi::MouseButtonState(SDL_BUTTON_RIGHT);
		if(rightMouseButtonState) {
			if(!m_prevRightMouseButtonState) {
				m_mouseFlightToggle = !m_mouseFlightToggle;
			}
		}
		m_prevRightMouseButtonState = rightMouseButtonState;
		
		if (m_mouseFlightToggle)
		{
			int x,y;
			Pi::mouseCursor->GetFlightCursorState(&x, &y);
			
			float window_width = Pi::renderer->GetWindow()->GetWidth();
			float window_height = Pi::renderer->GetWindow()->GetHeight();
			float zone_radius = MouseFlightZoneDiameter * window_width / 2.0f;
			float deadzone_radius = MouseFlightDeadZoneDiameter * window_width / 2.0f;
			float effect_radius = zone_radius - deadzone_radius;

			x += static_cast<int>(window_width / 2.0f);
			y += static_cast<int>(window_height / 2.0f);

			// Limit mouse rotation effect to flight zone (circle)
			vector2f mouse_pos(x - window_width / 2.0f, y - window_height / 2.0f);
			if (mouse_pos.LengthSqr() >= 1.0) {
				vector2f mouse_dir = mouse_pos.Normalized();
				if (mouse_pos.LengthSqr() > zone_radius * zone_radius) {
					mouse_pos = mouse_dir * zone_radius;
				}
			}
			float radius = mouse_pos.Length(); // Distance from center of view to mouse cursor inside zone
			if (radius >= deadzone_radius) {
				x = static_cast<int>(mouse_pos.x) * 2.0f;
				y = static_cast<int>(mouse_pos.y) * 2.0f;
			} else {
				x = 0;
				y = 0;
			}

			if(x == 0 && y == 0) {
				m_mouseFlightZeroOffset = true;
			} else {
				m_mouseFlightZeroOffset = false;
			}

			const matrix3x3d &rot = m_ship->GetOrient();
			if (!m_mouseActive) {
				m_mouseDir = -rot.VectorZ(); // in world space
				// m_mouseX = m_mouseY = 0;
				m_mouseActive = true;
			}
			vector3d objDir = m_mouseDir * rot;

			const double radiansPerPixel = 0.00001 * m_fovY;
			const int maxMotion = std::max(abs(x), abs(y));
			const double accel = Clamp(maxMotion / 100.0, 0.0, 0.1);

			m_mouseX = x * accel * radiansPerPixel;
			m_mouseX = clipmouse(objDir.x, m_mouseX);

			const bool invertY = (Pi::IsMouseYInvert() ? !m_invertMouse : m_invertMouse);

			m_mouseY = y * accel * radiansPerPixel * (invertY ? -1 : 1);
			m_mouseY = clipmouse(objDir.y, m_mouseY);

			if(!is_zero_general(m_mouseX) || !is_zero_general(m_mouseY)) {
				matrix3x3d mrot = matrix3x3d::RotateY(m_mouseX) * matrix3x3d::RotateX(m_mouseY);
				m_mouseDir = (rot * (mrot * objDir)).Normalized();
			}
			if(Pi::mouseCursor) {
				Pi::mouseCursor->SetType(MCT_FLIGHT);
			}
		} else { 
			m_mouseActive = false;
			m_mouseFlightZeroOffset = true;
			if(Pi::mouseCursor) {
				Pi::mouseCursor->SetType(MCT_NORMAL);
			}
		}

		if (m_flightControlState == CONTROL_MANEUVER) {
			double oldSpeed = m_setSpeed;
			if (stickySpeedKey) {
				if (!(KeyBindings::increaseSpeed.IsActive() || KeyBindings::decreaseSpeed.IsActive())) {
					stickySpeedKey = false;
				}
			}

			if (!stickySpeedKey) {
				if (KeyBindings::increaseSpeed.IsActive())
					m_setSpeed += std::max(fabs(m_setSpeed)*0.05, 1.0);
				if (KeyBindings::decreaseSpeed.IsActive())
					m_setSpeed -= std::max(fabs(m_setSpeed)*0.05, 1.0);
				// Limit set speed mode to maximum maneuver speed
				m_setSpeed = Clamp<double>(m_setSpeed, 
					-m_ship->GetShipType()->maxManeuverSpeed, 
					m_ship->GetShipType()->maxManeuverSpeed);
				if ( ((oldSpeed < 0.0) && (m_setSpeed >= 0.0)) ||
						((oldSpeed > 0.0) && (m_setSpeed <= 0.0)) ) {
					// flipped from going forward to backwards. make the speed 'stick' at zero
					// until the player lets go of the key and presses it again
					stickySpeedKey = true;
					m_setSpeed = 0;
				}
			}
		}

		/* Manual thruster controls are removed in Paragon
		if (KeyBindings::thrustForward.IsActive()) m_ship->SetThrusterState(2, -linearThrustPower);
		if (KeyBindings::thrustBackwards.IsActive()) m_ship->SetThrusterState(2, linearThrustPower);
		if (KeyBindings::thrustUp.IsActive()) m_ship->SetThrusterState(1, linearThrustPower);
		if (KeyBindings::thrustDown.IsActive()) m_ship->SetThrusterState(1, -linearThrustPower);
		if (KeyBindings::thrustLeft.IsActive()) m_ship->SetThrusterState(0, -linearThrustPower);
		if (KeyBindings::thrustRight.IsActive()) m_ship->SetThrusterState(0, linearThrustPower);
		*/

		if (KeyBindings::fireLaser.IsActive() || (Pi::MouseButtonState(SDL_BUTTON_LEFT) && m_mouseFlightToggle)) {
				//XXX worldview? madness, ask from ship instead
				m_ship->SetGunState(Pi::worldView->GetActiveWeapon(), 1);
		}

		if(KeyBindings::freightTeleporter.IsActive()) {
			Pi::player->SetFreightTeleporterState(1);
		} else {
			Pi::player->SetFreightTeleporterState(0);
		}

		if (KeyBindings::yawLeft.IsActive()) wantAngVel.y += 1.0;
		if (KeyBindings::yawRight.IsActive()) wantAngVel.y += -1.0;
		if (KeyBindings::pitchDown.IsActive()) wantAngVel.x += -1.0;
		if (KeyBindings::pitchUp.IsActive()) wantAngVel.x += 1.0;
		if (KeyBindings::rollLeft.IsActive()) wantAngVel.z += 1.0;
		if (KeyBindings::rollRight.IsActive()) wantAngVel.z -= 1.0;

		// Manual thruster controls are removed in Paragon
		//if (KeyBindings::thrustLowPower.IsActive())
		//	angThrustSoftness = 50.0;

		vector3d changeVec;
		changeVec.x = KeyBindings::pitchAxis.GetValue();
		changeVec.y = KeyBindings::yawAxis.GetValue();
		changeVec.z = KeyBindings::rollAxis.GetValue();

		// Deadzone more accurate
		for (int axis=0; axis<3; axis++) {
				if (fabs(changeVec[axis]) < m_joystickDeadzone)
					changeVec[axis]=0.0;
				else
					changeVec[axis] = changeVec[axis] * 2.0;
		}
		
		wantAngVel += changeVec;

		if (wantAngVel.Length() >= 0.001 || force_rotation_damping || m_rotationDamping) {
			if (Pi::game->GetTimeAccel()!=Game::TIMEACCEL_1X) {
				for (int axis=0; axis<3; axis++)
					wantAngVel[axis] = wantAngVel[axis] * Pi::game->GetInvTimeAccelRate();
			}

			m_ship->AIModelCoordsMatchAngVel(wantAngVel, angThrustSoftness);
		}

		if (m_mouseActive) m_ship->AIFaceDirection(m_mouseDir);

	}
}

bool PlayerShipController::IsAnyAngularThrusterKeyDown()
{
	return !Pi::IsConsoleActive() && (
		KeyBindings::pitchUp.IsActive()   ||
		KeyBindings::pitchDown.IsActive() ||
		KeyBindings::yawLeft.IsActive()   ||
		KeyBindings::yawRight.IsActive()  ||
		KeyBindings::rollLeft.IsActive()  ||
		KeyBindings::rollRight.IsActive()
	);
}

bool PlayerShipController::IsAnyAxisDown()
{
	float axis_state;
	axis_state = fabs(KeyBindings::pitchAxis.GetValue());
	axis_state += fabs(KeyBindings::yawAxis.GetValue());
	axis_state += fabs(KeyBindings::rollAxis.GetValue());
	if(axis_state > m_joystickDeadzone) {
		return true;
	} else {
		return false;
	}
}

bool PlayerShipController::IsAnyLinearThrusterKeyDown()
{
	return false;
	// Manual thruster controls are removed in Paragon
	/*return !Pi::IsConsoleActive() && (
		KeyBindings::thrustForward.IsActive()	||
		KeyBindings::thrustBackwards.IsActive()	||
		KeyBindings::thrustUp.IsActive()		||
		KeyBindings::thrustDown.IsActive()		||
		KeyBindings::thrustLeft.IsActive()		||
		KeyBindings::thrustRight.IsActive()
	);*/
}

void PlayerShipController::SetFlightControlState(FlightControlState s)
{
	if (m_flightControlState != s) {
		// finalizer
		switch(m_ship->GetFlightMode()) {
			case Ship::EFM_TRANSIT:
				m_ship->StopTransitDrive();
				break;
		}
		// new state
		m_flightControlState = s;
		m_ship->AIClearInstructions();
		switch(m_flightControlState) {
			case CONTROL_MANEUVER: {
				//set desired velocity to current actual
				// Speed is set to the projection of the velocity onto the target.
				vector3d shipVel = m_setSpeedTarget ?
					// Ship's velocity with respect to the target, in current frame's coordinates
					-m_setSpeedTarget->GetVelocityRelTo(m_ship) :
					// Ship's velocity with respect to current frame
					m_ship->GetVelocity();
				// A change from Manual to Set Speed never sets a negative speed.
				m_setSpeed = std::max(shipVel.Dot(-m_ship->GetOrient().VectorZ()), 0.0);
				break;
			}

			case CONTROL_TRANSIT:
				m_ship->StartTransitDrive();
				// Set transit speed to default, limit will be raised in update function based on altitude
				m_setSpeed = TRANSIT_START_SPEED;
				// Give it some juice to hit transit speed faster
				m_ship->SetJuice(80.0);
				break;
		}
		//XXX global stuff
		Pi::onPlayerChangeFlightControlState.emit();
	}
}

/*void PlayerShipController::OnPlayerChangeFlightMode()
{
	switch (m_ship->GetFlightMode()) {
		case Ship::EFM_MANEUVER:
			if (m_flightControlState == CONTROL_TRANSIT) {

			}
			break;

		case Ship::EFM_TRANSIT:
			if (m_flightControlState == CONTROL_MANEUVER) {

			}
			break;
	}
}*/

void PlayerShipController::SetLowThrustPower(float power)
{
	assert((power >= 0.0f) && (power <= 1.0f));
	m_lowThrustPower = power;
}

void PlayerShipController::SetRotationDamping(bool enabled)
{
	if (enabled != m_rotationDamping) {
		m_rotationDamping = enabled;
		onRotationDampingChanged.emit();
	}
}

void PlayerShipController::ToggleRotationDamping()
{
	SetRotationDamping(!GetRotationDamping());
}

void PlayerShipController::FireMissile()
{
	if (!Pi::player->GetCombatTarget())
		return;

	lua_State *l = Lua::manager->GetLuaState();
	int pristine_stack = lua_gettop(l);
	LuaObject<Ship>::PushToLua(Pi::player);
	lua_pushstring(l, "FireMissileAt");
	lua_gettable(l, -2);
	lua_pushvalue(l, -2);
	lua_pushstring(l, "any");
	LuaObject<Ship>::PushToLua(static_cast<Ship*>(Pi::player->GetCombatTarget()));
	lua_call(l, 3, 1);
	lua_settop(l, pristine_stack);
}

Body *PlayerShipController::GetCombatTarget() const
{
	return m_combatTarget;
}

Body *PlayerShipController::GetNavTarget() const
{
	return m_navTarget;
}

Body *PlayerShipController::GetSetSpeedTarget() const
{
	return m_setSpeedTarget;
}

void PlayerShipController::SetCombatTarget(Body* const target, bool setSpeedTo)
{
	if (setSpeedTo)
		m_setSpeedTarget = target;
	else if (m_setSpeedTarget == m_combatTarget)
		m_setSpeedTarget = 0;
	m_combatTarget = target;
}

void PlayerShipController::SetNavTarget(Body* const target, bool setSpeedTo)
{
	if (setSpeedTo)
		m_setSpeedTarget = target;
	else if (m_setSpeedTarget == m_navTarget)
		m_setSpeedTarget = 0;
	m_navTarget = target;
}
