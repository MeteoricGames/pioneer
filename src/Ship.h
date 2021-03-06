// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SHIP_H
#define _SHIP_H

#include "libs.h"
#include "Camera.h"
#include "DynamicBody.h"
#include "EquipSet.h"
#include "galaxy/SystemPath.h"
#include "ThrusterTrail.h"
#include "NavLights.h"
#include "Planet.h"
#include "Sensors.h"
#include "Serializer.h"
#include "ShipType.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/ModelSkin.h"
#include <list>
#include <unordered_map>
#include "graphics/gl2/Program.h"

class SpaceStation;
class HyperspaceCloud;
class AICommand;
class ShipController;
class CargoBody;
class Missile;
class ThrusterTrail;
namespace Graphics { class Renderer; }

struct shipstats_t {
	int used_capacity;
	int used_cargo;
	int free_capacity;
	int total_mass; // cargo, equipment + hull
	//int hydrogen_tank_left;
	float hull_mass_left; // effectively hitpoints
	float hyperspace_range;
	float hyperspace_range_max;
	float shield_mass;
	float shield_mass_left;
	float fuel_tank_mass_left;
};

class SerializableEquipSet: public EquipSet {
public:
	void Save(Serializer::Writer &wr);
	void Load(Serializer::Reader &rd);
};

// Transit State
enum TransitState {
	TRANSIT_DRIVE_OFF,
	TRANSIT_DRIVE_READY,
	TRANSIT_DRIVE_START,
	TRANSIT_DRIVE_ON,
	TRANSIT_DRIVE_STOP,
	TRANSIT_DRIVE_FINISHED,
};

// Transit Constants
static const double TRANSIT_GRAVITY_RANGE_1 = 15000.0;
static const double TRANSIT_GRAVITY_RANGE_2 = 1000000.0;
static const double TRANSIT_DRIVE_1_SPEED = 299000.0;
static const double TRANSIT_DRIVE_2_SPEED = 99999999999.0;
static const float TRANSIT_START_TIME = 2.0; // Allows sound to play first then the drive kicks in
static const double TRANSIT_STATIONCATCH_DISTANCE = 20000.0;
static const double TAKEOFF_THRUSTER_POWER = 0.1;
static const char* DEFAULT_SHIP_LABEL = "UNLABELED_SHIP";
// (Deprecated)
static const double TRANSIT_START_SPEED = 50000.0;


class Ship: public DynamicBody {
	friend class ShipController; //only controllers need access to AITimeStep
	friend class PlayerShipController;
public:
	OBJDEF(Ship, DynamicBody, SHIP);
	Ship(ShipType::Id shipId);
	Ship() {} //default constructor used before Load
	virtual ~Ship();

	virtual void SetFrame(Frame *f) override;

	void SetController(ShipController *c); //deletes existing
	ShipController *GetController() const { return m_controller; }
	virtual bool IsPlayerShip() const { return false; } //XXX to be replaced with an owner check

	virtual void SetDockedWith(SpaceStation *, int port);
	/** Use GetDockedWith() to determine if docked */
	SpaceStation *GetDockedWith() const { return m_dockedWith; }
	int GetDockingPort() const { return m_dockedWithPort; }

	virtual void SetLandedOn(Planet *p, float latitude, float longitude);

	virtual void Render(Graphics::Renderer *r, const Camera *camera, const vector3d &viewCoords, const matrix4x4d &viewTransform);

	void SetThrusterState(int axis, double level);
	void SetThrusterState(const vector3d &levels);
	vector3d GetThrusterState() const { return m_thrusters; }
	void SetAngThrusterState(int axis, double level) { m_angThrusters[axis] = Clamp(level, -1.0, 1.0); }
	void SetAngThrusterState(const vector3d &levels);
	vector3d GetAngThrusterState() const { return m_angThrusters; }
	void ClearThrusterState();
	float GetLaunchLockTimeout() const { return m_launchLockTimeout; }

	vector3d GetMaxThrust(const vector3d &dir) const;
	float GetMaxManeuverSpeed() const;
	double GetAccelFwd() const ;
	double GetAccelRev() const ;
	double GetAccelUp() const ;
	double GetAccelMin() const;

	void UpdateEquipStats();
	void UpdateFuelStats();
	void UpdateStats();
	const shipstats_t &GetStats() const { return m_stats; }

	void Explode();
	void SetGunState(int idx, int state);
	void UpdateMass();
	virtual bool SetWheelState(bool down); // returns success of state change, NOT state itself
	void Blastoff();
	bool Undock();
	virtual void TimeStepUpdate(const float timeStep);
	virtual void StaticUpdate(const float timeStep);

	// Ship automatically calculates velocity based on current flight mode
	void CalculateVelocity(bool force_drive_1, double transit_factor = 1.0);
	// Ship calculates velocity for maneuver flight mode
	void ManeuverVelocity();
	// Ship calculates velocity for transit flight mode
	void TransitVelocity(float timeStep, double altitude, bool only_drive_1 = false, double transit_factor = 1.0);
	bool IsTransitPossible();
	void TransitTunnelingTest(float timeStep);
	void TransitStationCatch(float timeStep);

	enum EFlightMode {
		EFM_MANEUVER,
		EFM_TRANSIT
	};
	EFlightMode GetFlightMode() const { return m_flightMode; }
	void SetFlightMode(EFlightMode m);

	void TimeAccelAdjust(const float timeStep);
	void SetDecelerating(bool decel) { m_decelerating = decel; }
	bool IsDecelerating() const { return m_decelerating; }

	virtual void NotifyRemoved(const Body* const removedBody);
	virtual bool OnCollision(Object *o, Uint32 flags, double relVel);
	virtual bool OnDamage(Object *attacker, float kgDamage, const CollisionContact& contactData);

	enum FlightState { // <enum scope='Ship' name=ShipFlightState public>
		FLYING,     // open flight (includes autopilot)
		DOCKING,    // in docking animation
		DOCKED,     // docked with station
		LANDED,     // rough landed (not docked)
		JUMPING,    // between space and hyperspace ;)
		HYPERSPACE, // in hyperspace
	};

	FlightState GetFlightState() const { return m_flightState; }
	void SetFlightState(FlightState s);
	float GetWheelState() const { return m_wheelState; }
	float GetJuice() const { return m_juice; }
	TransitState GetTransitState() const { return m_transitstate; }
	int GetWheelTransition() const { return m_wheelTransition; }
	bool SpawnCargo(CargoBody * c_body) const;

	virtual bool IsInSpace() const { return (m_flightState != HYPERSPACE); }

	void SetJuice(const double &juice) { m_juice = juice; }
	void SetTransitState(TransitState transitstate);
	void SetHyperspaceDest(const SystemPath &dest) { m_hyperspace.dest = dest; }
	const SystemPath &GetHyperspaceDest() const { return m_hyperspace.dest; }
	double GetHyperspaceDuration() const { return m_hyperspace.duration; }

	// Transit Drive
	virtual void StartTransitDrive();
	virtual void StopTransitDrive();
	
	enum HyperjumpStatus { // <enum scope='Ship' name=ShipJumpStatus prefix=HYPERJUMP_ public>
		HYPERJUMP_OK,
		HYPERJUMP_CURRENT_SYSTEM,
		HYPERJUMP_NO_DRIVE,
		HYPERJUMP_INITIATED,
		HYPERJUMP_DRIVE_ACTIVE,
		HYPERJUMP_OUT_OF_RANGE,
		HYPERJUMP_INSUFFICIENT_FUEL,
		HYPERJUMP_SAFETY_LOCKOUT
	};

	HyperjumpStatus GetHyperspaceDetails(const SystemPath &src, const SystemPath &dest, 
		int &outFuelRequired, double &outDurationSecs, bool phase_jump);
	HyperjumpStatus GetHyperspaceDetails(const SystemPath &dest, int &outFuelRequired, 
		double &outDurationSecs, bool phase_jump);
	HyperjumpStatus CheckHyperspaceTo(const SystemPath &dest, int &outFuelRequired, 
		double &outDurationSecs, bool phase_jump);
	HyperjumpStatus CheckHyperspaceTo(const SystemPath &dest, bool phase_jump);
	bool CanHyperspaceTo(const SystemPath &dest, HyperjumpStatus &status, 
 		bool phase_jump);
	bool CanHyperspaceTo(const SystemPath &dest, bool phase_jump);

	Ship::HyperjumpStatus CheckHyperjumpCapability() const;
	virtual Ship::HyperjumpStatus InitiateHyperjumpTo(const SystemPath &dest, 
		int warmup_time, double duration, LuaRef checks, bool phase_mode);
	virtual void AbortHyperjump();
	virtual Ship::HyperjumpStatus StartHyperspaceCountdown(const SystemPath &dest,
		bool phase_mode);
	float GetHyperspaceCountdown() const { return m_hyperspace.countdown; }
	bool IsHyperspaceActive() const { return (m_hyperspace.countdown > 0.0); }
	virtual void ResetHyperspaceCountdown();

	Equip::Type GetHyperdriveFuelType() const;
	// 0 to 1.0 is alive, > 1.0 = death
	double GetHullTemperature() const;
	void UseECM();
	virtual Missile * SpawnMissile(ShipType::Id missile_type, int power=-1);

	enum AlertState { // <enum scope='Ship' name=ShipAlertStatus prefix=ALERT_ public>
		ALERT_NONE,
		ALERT_SHIP_NEARBY,
		ALERT_SHIP_FIRING,
	};
	AlertState GetAlertState() { return m_alertState; }

	bool AIMatchVel(const vector3d &vel, bool isDocking = false);
	bool AIChangeVelBy(const vector3d &diffvel, bool isDocking = false);		// acts in obj space
	vector3d AIChangeVelDir(const vector3d &diffvel);	// world space, maintain direction
	void AIMatchAngVelObjSpace(const vector3d &angvel);
	double AIFaceUpdir(const vector3d &updir, double av=0);
	double AIFaceDirection(const vector3d &dir, double av=0);
	vector3d AIGetLeadDir(const Body *target, const vector3d& targaccel, int gunindex=0);
	double AITravelTime(const vector3d &reldir, double targdist, const vector3d &relvel, double endspeed, double maxdecel);

	// old stuff, deprecated
	void AIAccelToModelRelativeVelocity(const vector3d v);
	void AIModelCoordsMatchAngVel(vector3d desiredAngVel, double softness);
	void AIModelCoordsMatchSpeedRelTo(const vector3d v, const Ship *);

	void AIClearInstructions();
	bool AIIsActive() { return m_curAICmd ? true : false; }
	void AIGetStatusText(char *str);

	enum AIError { // <enum scope='Ship' name=ShipAIError prefix=AIERROR_ public>
		AIERROR_NONE=0,
		AIERROR_GRAV_TOO_HIGH,
		AIERROR_REFUSED_PERM,
		AIERROR_ORBIT_IMPOSSIBLE,
		AIERROR_REMOTE_DOCKING
	};
	AIError AIMessage(AIError msg=AIERROR_NONE) { AIError tmp = m_aiMessage; m_aiMessage = msg; return tmp; }

	void AIKamikaze(Body *target);
	void AIKill(Ship *target);
	void AIFire();
	//void AIJourney(SystemBodyPath &dest);
	void AIDock(SpaceStation *target);
	void AIFlyTo(Body *target);
	void AIFlyTo(Body *target, vector3d posoff);
	void AIFlyToClose(Body *target, double dist);
	void AIFlyToPermaCloud();
	void AIOrbit(Body *target, double alt);
	void AIHoldPosition();

	void AIBodyDeleted(const Body* const body) {};		// todo: signals

	SerializableEquipSet m_equipment;			// shouldn't be public?...

	virtual void PostLoadFixup(Space *space);

	const ShipType *GetShipType() const { return m_type; }
	void SetShipType(const ShipType::Id &shipId);

	const SceneGraph::ModelSkin &GetSkin() const { return m_skin; }
	void SetSkin(const SceneGraph::ModelSkin &skin);

	void SetLabel(const std::string &label);

	float GetPercentShields() const;
	float GetPercentHull() const;
	void SetPercentHull(float);
	float GetGunTemperature(int idx) const { return m_gun[idx].temperature; }

	enum FuelState { // <enum scope='Ship' name=ShipFuelStatus prefix=FUEL_ public>
		FUEL_OK,
		FUEL_WARNING,
		FUEL_EMPTY,
	};
	FuelState GetFuelState() { return m_thrusterFuel > 0.05f ? FUEL_OK : m_thrusterFuel > 0.0f ? FUEL_WARNING : FUEL_EMPTY; }

	// fuel left, 0.0-1.0
	double GetFuel() const { return m_thrusterFuel;	}
	void SetFuel(const double f);
	double GetFuelReserve() const { return m_reserveFuel; }
	void SetFuelReserve(const double f) { m_reserveFuel = Clamp(f, 0.0, 1.0); }

	int GetHydrogenCapacity() const;
	int GetHydrogen() const;
	int GetHydrogenFree() const;
	double GetHydrogenPercentage() const;
	int AddHydrogenUnits(int num);
	int RemoveHydrogenUnits(int num);
	void SetHydrogenUnits(int hydrogen);

	// available delta-V given the ship's current fuel state
	double GetSpeedReachedWithFuel() const;

	void EnterSystem();

	HyperspaceCloud *GetHyperspaceCloud() const { return m_hyperspaceCloud; }

	sigc::signal<void> onDock;				// JJ: check what these are for
	sigc::signal<void> onUndock;

	// mutable because asking to know when state changes is not the same as
	// actually changing state
	mutable sigc::signal<void> onFlavourChanged;

	bool IsInvulnerable() const { return m_invulnerable; }
	void SetInvulnerable(bool b) { m_invulnerable = b; }

	Uint8 GetRelations(Body *other) const; //0=hostile, 50=neutral, 100=ally
	void SetRelations(Body *other, Uint8 percent);

	bool TargetInSight() const { return m_targetInSight; }

	virtual Body *GetCombatTarget() const { return 0; }
	virtual Body *GetNavTarget() const { return 0; }

	double GetLandingPosOffset() const { return m_landingMinOffset; }

	unsigned int GetThrusterTrailsNum() const { return m_thrusterTrails.size(); }
	ThrusterTrail* GetThrusterTrail(unsigned int index) const { return m_thrusterTrails[index]; }
	void UpdateThrusterTrails(float time);
	void ClearThrusterTrails();
	bool IsPhaseJumpRange(bool recalc = false);
	bool IsPhaseJumpMode(bool recalc = false);

	sigc::signal<void>& OnPlayerChangeFlightModeSignal() {
		return m_onPlayerChangeFlightModeSignal;
	}

	const std::string& GetModuleStatus() const { return m_moduleStatus; }
	void SetModuleStatus(const std::string& status) { m_moduleStatus = status; }
	const std::string& GetModuleName() const { return m_moduleName; }
	void SetModuleName(const std::string& name) { m_moduleName = name; }

	bool IsVisible() const { return m_visible; }
	void SetVisible(bool visible) { m_visible = visible; }

	float GetMaxHullTemp() const { return m_maxHullTemp; }

	bool IsRemotlyDocked() const { return m_isRemotlyDocked; }
	void SetRemotlyDocked(bool state) { m_isRemotlyDocked = state; }
	virtual void CalcExternalForce() override;
	virtual void SetVelocity(const vector3d &v) override {};
	virtual void TmpSetVelocity(const vector3d &v) { DynamicBody::SetVelocity(v); }
    const std::vector<std::string> GetNamesOfNearbyShips() { return m_namesOfNearbyShips; }

protected:
	virtual void Save(Serializer::Writer &wr, Space *space);
	virtual void Load(Serializer::Reader &rd, Space *space);

	virtual void Init();

	void RenderLaserfire();
	void ApplyThrusterLimits();
		
	bool AITimeStep(float timeStep); // Called by controller. Returns true if complete

	virtual void SetAlertState(AlertState as);

	virtual void OnEnterHyperspace();
	virtual void OnEnterSystem();

	SpaceStation *m_dockedWith;
	int m_dockedWithPort;

	struct Gun {
		vector3f pos;
		vector3f dir;
		Uint32 state;
		float recharge;
		float temperature;
	};
	Gun m_gun[ShipType::GUNMOUNT_MAX];

	float m_ecmRecharge;

	ShipController *m_controller;
	std::vector<ThrusterTrail*> m_thrusterTrails;

	struct HyperspacingOut {
		SystemPath dest;
		// > 0 means active
		float countdown;
		bool now;
		bool ignoreFuel; // XXX: To remove once the fuel handling is out of the core
		double duration;
		bool phaseMode;
		LuaRef checks; // A Lua function to check all the conditions before the jump
	} m_hyperspace;

	const float m_maxHullTemp = 0.1f; // The temperature that the hull can take until it starts damaging the ship.
    std::vector<std::string> m_namesOfNearbyShips;

private:
	float GetECMRechargeTime();
	void DoThrusterSounds() const;
	void FireWeapon(int num);
	bool IsFiringLasers();
	void TestLanded();
	void UpdateAlertState();
	void UpdateFuel(float timeStep, const vector3d &thrust);
    void SetShipId(const ShipType::Id &shipId);
	void OnEquipmentChange(Equip::Type e);
	void EnterHyperspace();
	void InitGun(const char *tag, int num);
	void InitMaterials();
	bool UpdatePhaseJumpRange();
	bool UpdatePhaseJumpMode();
	virtual void PrivateSetVelocity(const vector3d &v);

	bool m_invulnerable;

	static const float DEFAULT_SHIELD_COOLDOWN_TIME;
	float m_shieldCooldown;
	shipstats_t m_stats;
	const ShipType *m_type;
	SceneGraph::ModelSkin m_skin;

	FlightState m_flightState;
	bool m_testLanded;
	float m_launchLockTimeout;
	float m_transitStartTimeout;
	float m_wheelState;
	int m_wheelTransition;
	double m_juice;
	TransitState m_transitstate;
	EFlightMode m_flightMode;
	sigc::signal<void> m_onPlayerChangeFlightModeSignal;

	vector3d m_thrusters;
	vector3d m_angThrusters;

	AlertState m_alertState;
	double m_lastFiringAlert;

	HyperspaceCloud *m_hyperspaceCloud;

	AICommand *m_curAICmd;
	AIError m_aiMessage;
	bool m_decelerating;

	double m_thrusterFuel; 	// remaining fuel 0.0-1.0
	double m_reserveFuel;	// 0-1, fuel not to touch for the current AI program

	bool m_targetInSight;
	vector3d m_lastVel;

	double m_landingMinOffset;	// offset from the centre of the ship used during docking

	int m_dockedWithIndex; // deserialisation

	SceneGraph::Animation *m_landingGearAnimation;
	std::unique_ptr<NavLights> m_navLights;

	 std::unordered_map<Body*, Uint8> m_relationsMap;

	static HeatGradientParameters_t s_heatGradientParams;

	bool m_unlabeled;
	bool m_phaseJumpMode;
	bool m_phaseJumpRange;
	HyperspaceCloud* m_phaseModeCloud;

	std::string m_moduleStatus; // Ship status coming from module
	std::string m_moduleName;	// LUA module that create the ship

	bool m_visible;
	bool m_isRemotlyDocked = false;
};



#endif /* _SHIP_H */


