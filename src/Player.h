// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _PLAYER_H
#define _PLAYER_H

#include "libs.h"
#include <list>
#include "HyperspaceCloud.h"
#include "Ship.h"
#include "ShipController.h"
#include "ShipCockpit.h"
#include "galaxy/StarSystem.h"
#include "Sound.h"

namespace Graphics { class Renderer; }

struct SFreightTeleporterSpecs
{
	double range;
	double shield_override;
	double min_rate;
	double max_rate;
};

static const SFreightTeleporterSpecs FreightTeleporterSpecs [2] = {
	{ 1000.0, 0.0,  1.0, 2.0 }, // BASIC FREIGHT TELEPORTER
	{ 2500.0, 0.25, 2.0, 4.0 }, // ADVANCED FREIGHT TELEPORTER
};

enum EFreightTeleporterTargetType {
	EFT_TT_NONE = 0,
	EFT_TT_COMBAT_TGT,
	EFT_TT_NAV_TGT,
};

enum EFreightTeleporterStatus {
	EFT_S_NOT_AVAILABLE = 0,
	EFT_S_NO_TGT,
	EFT_S_TGT_OUT_OF_RANGE,
	EFT_S_FREIGHT_FULL,
	EFT_S_TGT_SHIELDED,
	EFT_S_ACTIVE,
};

class Player: public Ship {
public:
	OBJDEF(Player, Ship, PLAYER);
	Player(ShipType::Id shipId);
	Player() {}; //default constructor used before Load

	virtual void SetFrame(Frame *f) override;

	virtual void SetDockedWith(SpaceStation *, int port);
	virtual bool OnDamage(Object *attacker, float kgDamage, const CollisionContact& contactData);
	virtual bool SetWheelState(bool down); // returns success of state change, NOT state itself
	virtual Missile * SpawnMissile(ShipType::Id missile_type, int power=-1);
	virtual void SetAlertState(Ship::AlertState as);
	virtual void NotifyRemoved(const Body* const removedBody);

	PlayerShipController *GetPlayerController() const;
	//XXX temporary things to avoid causing too many changes right now
	Body *GetCombatTarget() const;
	Body *GetNavTarget() const;
	Body *GetSetSpeedTarget() const;
	void SetCombatTarget(Body* const target, bool setSpeedTo = false);
	void SetNavTarget(Body* const target, bool setSpeedTo = false);

	virtual Ship::HyperjumpStatus InitiateHyperjumpTo(const SystemPath &dest, int warmup_time, double duration, LuaRef checks);
	virtual void AbortHyperjump();
	virtual Ship::HyperjumpStatus StartHyperspaceCountdown(const SystemPath &dest);
	virtual void ResetHyperspaceCountdown();

	// XXX cockpit is here for now because it has a physics component
	void InitCockpit();
	ShipCockpit* GetCockpit() const {return m_cockpit.get();}
	void OnCockpitActivated();

	virtual void SetRelations(Body *other, Uint8 percent);

	virtual void TimeStepUpdate(const float timeStep);
	virtual void StaticUpdate(const float timeStep);
	virtual bool IsPlayerShip() const override { return true; }

	Sensors *GetSensors() const { return m_sensors.get(); }

	virtual void StartTransitDrive() override;
	virtual void StopTransitDrive() override;

	SystemPath* GetCurrentMissionPath() const;
	void SetCurrentMissionPath(SystemPath* sp);

	// Freight Teleporter: No need to load/save any of these properties as they are recalculated every frame 
	// only when needed.
	//
	/// FT Level: 0 -> no freight teleporter on ship, 1 -> basic freight teleporter, 2 -> advanced freight teleporter
	int GetFreightTeleporterLevel() const;
	/// FT Target: the valid target body of freight teleporter. Null if no valid target is available.
	Body* GetFreightTeleporterTarget() const;
	/// FT State: The firing state of the teleporter. 0 means no firing. 1 means fire
	void SetFreightTeleporterState(int state);
	/// FT Target type: none, nav or combat.
	EFreightTeleporterTargetType GetFreightTeleporterTargetType() const;
	/// FT Status: not available, no target, target out of range, or FT is active (Ready to fire!)
	EFreightTeleporterStatus GetFreightTeleporterStatus() const;

protected:
	virtual void Save(Serializer::Writer &wr, Space *space);
	virtual void Load(Serializer::Reader &rd, Space *space);
	virtual void Init() override;
	void InitFreightTeleporter();

	virtual void OnEnterSystem();
	virtual void OnEnterHyperspace();

	void UpdateFreightTeleporter();
	void FireFreightTeleporter();

private:
	std::unique_ptr<ShipCockpit> m_cockpit;
	std::unique_ptr<Sensors> m_sensors;
	std::unique_ptr<SystemPath> m_currentMissionPath;
	Body* m_ftTarget;
	int m_ftState;
	EFreightTeleporterTargetType m_ftTargetType;
	EFreightTeleporterStatus m_ftStatus;
	float m_ftRechargeTime;
	Sound::Event m_ftSound;
	Sound::Event m_ftSoundLoop;
};

#endif /* _PLAYER_H */
