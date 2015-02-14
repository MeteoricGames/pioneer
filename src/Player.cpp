// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "VSLog.h"

#include "Player.h"
#include "Frame.h"
#include "Game.h"
#include "KeyBindings.h"
#include "Lang.h"
#include "Pi.h"
#include "SectorView.h"
#include "Serializer.h"
#include "ShipCpanel.h"
#include "Sound.h"
#include "SpaceStation.h"
#include "WorldView.h"
#include "StringF.h"
#include "ThrusterTrail.h"

//Some player specific sounds
static Sound::Event s_soundUndercarriage;
static Sound::Event s_soundHyperdrive;
Random g_rand;

Player::Player(ShipType::Id shipId): Ship(shipId)
{
	SetController(new PlayerShipController());
}

void Player::Save(Serializer::Writer &wr, Space *space)
{
	Ship::Save(wr, space);
}

void Player::Load(Serializer::Reader &rd, Space *space)
{
	Pi::player = this;
	InitFreightTeleporter();
	Ship::Load(rd, space);
}

void Player::Init()
{
	InitCockpit();
	m_sensors.reset(new Sensors(this));
	InitFreightTeleporter();
	Ship::Init();
}

void Player::InitFreightTeleporter()
{
	m_ftTarget = nullptr;
	m_ftState = 0;
	m_ftTargetType = EFT_TT_NONE;
	m_ftStatus = EFT_S_NOT_AVAILABLE;
	m_ftRechargeTime = 0.0f;
}

void Player::InitCockpit()
{
	m_cockpit.release();
	if (!Pi::config->Int("EnableCockpit"))
		return;

	// XXX select a cockpit model. this is all quite skanky because we want a
	// fallback if the name is not found, which means having to actually try to
	// load the model. but ModelBody (on which ShipCockpit is currently based)
	// requires a model name, not a model object. it won't hurt much because it
	// all stays in the model cache anyway, its just awkward. the fix is to fix
	// ShipCockpit so its not a ModelBody and thus does its model work
	// directly, but we're not there yet
	std::string cockpitModelName;
	if (!GetShipType()->cockpitName.empty()) {
		if (Pi::FindModel(GetShipType()->cockpitName, false))
			cockpitModelName = GetShipType()->cockpitName;
	}
	if (cockpitModelName.empty()) {
		if (Pi::FindModel("default_cockpit", false))
			cockpitModelName = "default_cockpit";
	}
	if (!cockpitModelName.empty())
		m_cockpit.reset(new ShipCockpit(cockpitModelName));
}

//XXX perhaps remove this, the sound is very annoying
bool Player::OnDamage(Object *attacker, float kgDamage, const CollisionContact& contactData)
{
	bool r = Ship::OnDamage(attacker, kgDamage, contactData);
	if (!IsDead() && (GetPercentHull() < 25.0f)) {
		Sound::BodyMakeNoise(this, "warning", .5f);
	}
	return r;
}

//XXX handle killcounts in lua
void Player::SetDockedWith(SpaceStation *s, int port)
{
	Ship::SetDockedWith(s, port);
}

//XXX all ships should make this sound
bool Player::SetWheelState(bool down)
{
	bool did = Ship::SetWheelState(down);
	if (did) {
		s_soundUndercarriage.Play(down ? "UC_out" : "UC_in", 1.0f, 1.0f, 0);
	}
	return did;
}

//XXX all ships should make this sound
Missile * Player::SpawnMissile(ShipType::Id missile_type, int power)
{
	Missile * m = Ship::SpawnMissile(missile_type, power);
	if (m)
		Sound::PlaySfx("Missile_launch", 1.0f, 1.0f, 0);
	return m;
}

//XXX do in lua, or use the alert concept for all ships
void Player::SetAlertState(Ship::AlertState as)
{
	Ship::AlertState prev = GetAlertState();

	switch (as) {
		case ALERT_NONE:
			if (prev != ALERT_NONE) {
				//Pi::cpan->MsgLog()->Message("", Lang::ALERT_CANCELLED);
				Pi::game->log->Add(Lang::ALERT_CANCELLED);
			}
			break;

		case ALERT_SHIP_NEARBY:
			if (prev == ALERT_NONE) {
				//Pi::cpan->MsgLog()->ImportantMessage("", Lang::SHIP_DETECTED_NEARBY);
				Pi::game->log->Add(Lang::SHIP_DETECTED_NEARBY);
			} else {
				//Pi::cpan->MsgLog()->ImportantMessage("", Lang::DOWNGRADING_ALERT_STATUS);
				Pi::game->log->Add(Lang::DOWNGRADING_ALERT_STATUS);
			}
			Sound::PlaySfx("OK");
			break;

		case ALERT_SHIP_FIRING:
			//Pi::cpan->MsgLog()->ImportantMessage("", Lang::LASER_FIRE_DETECTED);
			Pi::game->log->Add(Lang::LASER_FIRE_DETECTED);
			Sound::PlaySfx("warning", 0.2f, 0.2f, 0);
			break;
	}

	Pi::cpan->SetAlertState(as);

	Ship::SetAlertState(as);
}

void Player::NotifyRemoved(const Body* const removedBody)
{
	if (GetNavTarget() == removedBody)
		SetNavTarget(0);

	else if (GetCombatTarget() == removedBody) {
		SetCombatTarget(0);

		if (!GetNavTarget() && removedBody->IsType(Object::SHIP))
			SetNavTarget(static_cast<const Ship*>(removedBody)->GetHyperspaceCloud());
	}

	Ship::NotifyRemoved(removedBody);
}

//XXX ui stuff
void Player::OnEnterHyperspace()
{
	s_soundHyperdrive.Play("Hyperdrive_Jump");
	SetNavTarget(0);
	SetCombatTarget(0);

	Pi::worldView->HideTargetActions(); // hide the comms menu
	m_controller->SetFlightControlState(CONTROL_MANEUVER); //could set CONTROL_HYPERDRIVE
	ClearThrusterState();
	Pi::game->WantHyperspace();
}

void Player::OnEnterSystem()
{
	m_controller->SetFlightControlState(CONTROL_MANEUVER);
	//XXX don't call sectorview from here, use signals instead
	Pi::sectorView->ResetHyperspaceTarget();
}

//temporary targeting stuff
PlayerShipController *Player::GetPlayerController() const
{
	return static_cast<PlayerShipController*>(GetController());
}

Body *Player::GetCombatTarget() const
{
	return static_cast<PlayerShipController*>(m_controller)->GetCombatTarget();
}

Body *Player::GetNavTarget() const
{
	return static_cast<PlayerShipController*>(m_controller)->GetNavTarget();
}

Body *Player::GetSetSpeedTarget() const
{
	return static_cast<PlayerShipController*>(m_controller)->GetSetSpeedTarget();
}

void Player::SetCombatTarget(Body* const target, bool setSpeedTo)
{
	static_cast<PlayerShipController*>(m_controller)->SetCombatTarget(target, setSpeedTo);
	Pi::onPlayerChangeTarget.emit();
}

void Player::SetNavTarget(Body* const target, bool setSpeedTo)
{
	static_cast<PlayerShipController*>(m_controller)->SetNavTarget(target, setSpeedTo);
	Pi::onPlayerChangeTarget.emit();
}
//temporary targeting stuff ends

Ship::HyperjumpStatus Player::InitiateHyperjumpTo(const SystemPath &dest, int warmup_time, double duration, LuaRef checks) {
	HyperjumpStatus status = Ship::InitiateHyperjumpTo(dest, warmup_time, duration, checks);

	if (status == HYPERJUMP_OK)
		s_soundHyperdrive.Play("Hyperdrive_Charge");

	return status;
}

Ship::HyperjumpStatus Player::StartHyperspaceCountdown(const SystemPath &dest)
{
	HyperjumpStatus status = Ship::StartHyperspaceCountdown(dest);

	if (status == HYPERJUMP_OK)
		s_soundHyperdrive.Play("Hyperdrive_Charge");

	return status;
}

void Player::AbortHyperjump()
{
	s_soundHyperdrive.Play("Hyperdrive_Abort");
	Ship::AbortHyperjump();
}

void Player::ResetHyperspaceCountdown()
{
	s_soundHyperdrive.Play("Hyperdrive_Abort");
	Ship::ResetHyperspaceCountdown();
}

void Player::OnCockpitActivated()
{
	if (m_cockpit)
		m_cockpit->OnActivated();
}

void Player::TimeStepUpdate(const float timeStep)
{
	Ship::TimeStepUpdate(timeStep);
	if (m_sensors) {
		m_sensors->Update(timeStep);
	}
}

void Player::StaticUpdate(const float timeStep)
{
	Ship::StaticUpdate(timeStep);

	// XXX even when not on screen. hacky, but really cockpit shouldn't be here
	// anyway so this will do for now
	if (m_cockpit) {
		m_cockpit->Update(timeStep);
		if (GetTransitState() == TRANSIT_DRIVE_ON) {
			double percentage = log(std::max<double>(GetVelocity().Length(), 0.0)) / log(TRANSIT_DRIVE_2_SPEED);
			double rx = ((rand() % 10) - 5) * 0.001 * percentage * 0.75;
			double ry = ((rand() % 10) - 5) * 0.001 * percentage * 0.75;
			m_cockpit->Shake(rx, rx);
		}
	}
	if(m_ftRechargeTime > 0.0f) {
		m_ftRechargeTime -= timeStep;
	}
	if(GetFlightState() != FlightState::HYPERSPACE) {
		UpdateFreightTeleporter();
	}
}

void Player::SetFrame(Frame *f)
{
	Ship::SetFrame(f);
}

void Player::SetRelations(Body *other, Uint8 percent)
{
	Ship::SetRelations(other, percent);
	if (m_sensors) {
		m_sensors->UpdateIFF(other);
	}
}

void Player::StartTransitDrive()
{
	Ship::StartTransitDrive();
	if(Pi::GetView() && Pi::GetView() == Pi::worldView && Pi::worldView->GetCameraController() &&
		Pi::worldView->GetCamType() == WorldView::CamType::CAM_INTERNAL)
	{
		InternalCameraController* cam = static_cast<InternalCameraController*>(
			Pi::worldView->GetCameraController());
		if (cam->GetMode() == InternalCameraController::Mode::MODE_FRONT) {
			cam->Reset();
			cam->ResetFreelook();
		}
	}
}

void Player::StopTransitDrive()
{
	Ship::StopTransitDrive();
}

SystemPath* Player::GetCurrentMissionPath() const
{
	return m_currentMissionPath? m_currentMissionPath.get() : nullptr;
}

void Player::SetCurrentMissionPath(SystemPath* sp)
{
	if(!sp) {
		m_currentMissionPath.reset(nullptr);
	} else {
		m_currentMissionPath.reset(new SystemPath(sp));
	}
}

int Player::GetFreightTeleporterLevel() const
{
	switch(m_equipment.Get(Equip::SLOT_FREIGHTTELEPORTER)) {
		case Equip::BASIC_FREIGHT_TELEPORTER:
			return 1;
		case Equip::ADVANCED_FREIGHT_TELEPORTER:
			return 2;
		default:
			return 0;
	}
	return 0;
}

Body* Player::GetFreightTeleporterTarget() const
{
	return m_ftTarget;
}

void Player::UpdateFreightTeleporter()
{
	int ft_level = GetFreightTeleporterLevel();
	if(ft_level > 0) {
		m_ftStatus = EFT_S_NO_TGT;
		m_ftTargetType = EFT_TT_NONE;
		m_ftTarget = nullptr;

		if(GetStats().free_capacity < 1) {
			m_ftStatus = EFT_S_FREIGHT_FULL;
			m_ftState = 0;
			return;
		}

		// TODO:
		// - I noticed that all ships show as "shielded" so perhaps I should check for equipment first.
		const SFreightTeleporterSpecs& ft_specs = FreightTeleporterSpecs[ft_level - 1];
		Body* combat_tgt = GetCombatTarget();
		double combat_tgt_dist = combat_tgt != nullptr? combat_tgt->GetPositionRelTo(this).Length() : 0.0;
		EFreightTeleporterStatus combat_tgt_status = EFT_S_NO_TGT;
		Body* nav_tgt = GetNavTarget();
		double nav_tgt_dist = nav_tgt != nullptr? nav_tgt->GetPositionRelTo(this).Length() : 0.0;
		EFreightTeleporterStatus nav_tgt_status = EFT_S_NO_TGT;

		if(combat_tgt) {
			if(combat_tgt->GetType() != Body::Type::SHIP) {
				combat_tgt = nullptr;
			} else if(combat_tgt_dist > ft_specs.range) {
				combat_tgt = nullptr;
				combat_tgt_status = EFT_S_TGT_OUT_OF_RANGE;
			} else {
				Ship* tgt = dynamic_cast<Ship*>(combat_tgt);
				if(tgt->m_equipment.Get(Equip::SLOT_SHIELD) != Equip::NONE &&
					tgt->GetPercentShields() > ft_specs.shield_override * 100.0f)
				{
					combat_tgt = nullptr;
					combat_tgt_status = EFT_S_TGT_SHIELDED;
				} else {
					combat_tgt_status = EFT_S_ACTIVE;
				}
			}
		}
		if(nav_tgt) {
			if(nav_tgt->GetType() != Body::Type::SHIP && nav_tgt->GetType() != Body::Type::CARGOBODY) {
				nav_tgt = nullptr;
			} else if(nav_tgt_dist > ft_specs.range) {
				nav_tgt = nullptr;
				nav_tgt_status = EFT_S_TGT_OUT_OF_RANGE;
			} else {
				nav_tgt_status = EFT_S_ACTIVE;
			}
		}

		// Because there could be two selected valid bodies at the same time, freight teleporter prioritizes
		// the body closest to the player regardless of its type (combat or nav target).
		if (combat_tgt && nav_tgt) {
			m_ftStatus = EFT_S_ACTIVE;
			if(nav_tgt_dist < combat_tgt_dist) {
				m_ftTarget = nav_tgt;
				m_ftTargetType = EFT_TT_NAV_TGT;
			} else {
				m_ftTarget = combat_tgt;
				m_ftTargetType = EFT_TT_COMBAT_TGT;
			}
		} else if(combat_tgt) {
			m_ftStatus = EFT_S_ACTIVE;
			m_ftTarget = combat_tgt;
			m_ftTargetType = EFT_TT_COMBAT_TGT;
		} else if (nav_tgt) {
			m_ftStatus = EFT_S_ACTIVE;
			m_ftTarget = nav_tgt;
			m_ftTargetType = EFT_TT_NAV_TGT;
		} else if(nav_tgt_status == EFT_S_TGT_OUT_OF_RANGE || combat_tgt_status == EFT_S_TGT_OUT_OF_RANGE) {
			m_ftStatus = EFT_S_TGT_OUT_OF_RANGE;
		} else if(combat_tgt_status == EFT_S_TGT_SHIELDED) {
			m_ftStatus = EFT_S_TGT_SHIELDED;
		} else {
			m_ftStatus = EFT_S_NO_TGT;
		}

		if(m_ftTarget && m_ftState > 0) {
			FireFreightTeleporter();
			m_ftState = 0;
		}
	} else {
		m_ftTarget = nullptr;
		m_ftState = 0;
		m_ftTargetType = EFT_TT_NONE;
		m_ftStatus = EFT_S_NOT_AVAILABLE;
	}
}

void Player::SetFreightTeleporterState(int state)
{
	m_ftState = state;
	if (m_ftState == 0 && m_ftSoundLoop.IsPlaying()) {
		m_ftSoundLoop.Stop();
	}
}

void Player::FireFreightTeleporter()
{
	int ft_level = GetFreightTeleporterLevel();
	if(m_ftTarget == nullptr || ft_level == 0 || m_ftStatus != EFT_S_ACTIVE) {
		assert(false);
		return;
	}
	if (!m_ftSoundLoop.IsPlaying()) {
		m_ftSoundLoop.Play("Freight_Teleport_Loop", 0.5f, 0.5f, Sound::OP_REPEAT);
	}
	const SFreightTeleporterSpecs& ft_specs = FreightTeleporterSpecs[ft_level - 1];
	if(m_ftRechargeTime <= 0.0f) {
		m_ftRechargeTime = g_rand.Double(ft_specs.max_rate - ft_specs.min_rate) + ft_specs.min_rate;
		// - Steal freight!
		if (GetStats().free_capacity) {
			if(m_ftTarget->GetType() == Body::Type::CARGOBODY) {
				Equip::Type cargo_type = dynamic_cast<CargoBody*>(m_ftTarget)->GetCargoType();
				Pi::game->GetSpace()->KillBody(dynamic_cast<Body*>(m_ftTarget));
				m_equipment.Add(cargo_type, 1);
				UpdateEquipStats();
				// Some sort of indication that cargo teleport is successful? (should be in HUD rather than message)
				m_ftTarget = nullptr;
			} else if(m_ftTarget->GetType() == Body::Type::SHIP) {
				// - Select random cargo from freight bay
				Ship* tgt = dynamic_cast<Ship*>(m_ftTarget);
				int cargo_size = tgt->m_equipment.GetSlotSize(Equip::SLOT_CARGO);
				int rc = g_rand.Int32(0, cargo_size - 1);
				//VSLog::stream << "Cargo items count in target: " << cargo_size << std::endl;
				Equip::Type chosen_equip = tgt->m_equipment.Get(Equip::SLOT_CARGO, rc);
				//VSLog::stream << "Random cargo: " << rc << " is " << static_cast<int>(chosen_equip) << std::endl;
				//VSLog::stream << "Ship has: " << tgt->m_equipment.Count(Equip::SLOT_CARGO, chosen_equip)
				//	<< std::endl;
				//VSLog::outputLog();
				// - steal 1 unit of said random cargo
				tgt->m_equipment.Remove(chosen_equip, 1);
				m_equipment.Add(chosen_equip, 1);
				tgt->UpdateEquipStats();
				UpdateEquipStats();
				// Some sort of indication that cargo teleport is successful? (should be in HUD rather than message)
				m_ftTarget = nullptr;

				// - Should there be a notification if someone is stealing from player ship? only player can
				//   do this now so it's too early to add something like that.
				// - Should player be set to enemy the moment he teleports a unit of freight? depends on how
				//   notoriety will work. TBD
			}
			Sound::BodyMakeNoise(this, "Freight_Teleport", 0.5f);
		}
	}
}

EFreightTeleporterTargetType Player::GetFreightTeleporterTargetType() const
{
	return m_ftTargetType;
}

EFreightTeleporterStatus Player::GetFreightTeleporterStatus() const
{
	return m_ftStatus;
}
