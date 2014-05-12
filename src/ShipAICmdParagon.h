// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _SHIP_AI_CMD_PARAGON_H
#define _SHIP_AI_CMD_PARAGON_H

#include "ShipAICmd.h"

/// Fly to command is the main travel command, it can take a ship from any one location to another.
/// It always prefers Transit over Maneuver speed and will try to fly to destination
/// as fast as possible.
/// Example: travelling from a space station on planet X to a space station on planet Y
class AIParagonCmdFlyTo : public AICommand {
public:
	AIParagonCmdFlyTo(Ship *ship, Frame *targframe, const vector3d &posoff, double endvel, bool tangent);
	AIParagonCmdFlyTo(Ship *ship, Body *target);
	AIParagonCmdFlyTo(Ship *ship, Body *target, double dist);
	virtual ~AIParagonCmdFlyTo();

	virtual bool TimeStepUpdate();
	virtual void GetStatusText(char *str) {
		if (m_child) {
			m_child->GetStatusText(str);
		} else if (m_target) {
			snprintf(str, 255, "Paragon FlyTo: %s, dist %.1fkm, juice: %.1f",
				m_target->GetLabel().c_str(), m_remainingDistance / 1000.0, m_ship->GetJuice());
		} else {
			snprintf(str, 255, "Paragon FlyTo: %s, dist %.1fkm, endvel %.1fkm/s, state %i, juice: %.1f",
				m_targetFrame->GetLabel().c_str(), m_posOffset.Length() / 1000.0, m_endVelocity / 1000.0, m_state,
				m_ship->GetJuice());
		}
	}
	virtual void Save(Serializer::Writer &wr) {
		if (m_child) { delete m_child; m_child = 0; }
		AICommand::Save(wr);
		wr.Int32(Pi::game->GetSpace()->GetIndexForBody(m_target));
		wr.Vector3d(m_targetPosition);
		wr.Double(m_arrivalRadius);
		wr.Int32(Pi::game->GetSpace()->GetIndexForFrame(m_targetFrame));
		wr.Vector3d(m_posOffset);
		wr.Double(m_endVelocity);
		wr.Bool(m_tangent);
		wr.Int32(m_state);
	}
	AIParagonCmdFlyTo(Serializer::Reader &rd) : AICommand(rd, CMD_PARAGON_FLYTO) {
		m_targetIndex = rd.Int32();
		m_targetPosition = rd.Vector3d();
		m_arrivalRadius = rd.Double();
		m_targframeIndex = rd.Int32();
		m_posOffset = rd.Vector3d();
		m_endVelocity = rd.Double();
		m_tangent = rd.Bool();
		m_state = rd.Int32();
	}
	virtual void PostLoadFixup(Space *space) {
		AICommand::PostLoadFixup(space);
		m_target = space->GetBodyByIndex(m_targetIndex);
		m_targetFrame = space->GetFrameByIndex(m_targframeIndex);
		m_lockhead = true;
	}
	virtual void OnDeleted(const Body *body) {
		AICommand::OnDeleted(body);
		if (m_target == body) m_target = 0;
	}

private:
	struct SFlyToData { // Cache for repetitively calculated data
		vector3d ship_pos;
		double ship_to_target_distance;
		vector3d ship_to_target_dir;
		Body* fbody;
		double ship_to_fbody_distance;
		vector3d ship_to_fbody_dir;
		double fbody_transit_radius;
	} m_data;
	void CacheData();

	bool GoToTransitDistance();

	Body *m_target;		// target for vicinity. Either this or targframe is 0
	vector3d m_targetPosition;
	double m_arrivalRadius; // target radius of arrival
	Frame *m_targetFrame;	// target frame for waypoint
	Object::Type m_targetType; // target/object type
	double m_endVelocity;	// target speed in direction of motion at end of path, positive only
	int m_targetIndex, m_targframeIndex;	// used during deserialisation

	vector3d m_posOffset;	// offset in target frame
	bool m_tangent;		// true if path is to a tangent of the target frame's body
	int m_state;
	bool m_lockhead;
	vector3d m_relDir;	// target direction relative to ship at last frame change
	Frame *m_frame;		// last frame of ship
	double m_remainingDistance; // For status text
};

/// GoTo command is used by FlyTo as a simple travel command for short distances. 
/// GoTo uses maneuver mode only.
/// Example: going from within the atmosphere of a planet outwards to transit gravity bubble distance.
class AIParagonCmdGoTo : public AICommand {
public:
	AIParagonCmdGoTo(Ship *ship, Frame *target_frame, vector3d target_position, bool to_transit_distance = false);
	virtual ~AIParagonCmdGoTo();

	virtual bool TimeStepUpdate();
	virtual void GetStatusText(char *str) {
		if (m_child) {
			m_child->GetStatusText(str);
		} else {
			snprintf(str, 255, "Paragon GoTo: distance: %.1fkm, juice: %.1f",
				m_remainingDistance, m_ship->GetJuice());
		}
	}
	virtual void Save(Serializer::Writer &wr) {
		if (m_child) { delete m_child; m_child = 0; }
		AICommand::Save(wr);
		wr.Int32(Pi::game->GetSpace()->GetIndexForFrame(m_targetFrame));
		wr.Vector3d(m_targetPosition);
		wr.Bool(m_toTransit);
	}
	AIParagonCmdGoTo(Serializer::Reader &rd) : AICommand(rd, CMD_PARAGON_GOTO) {
		m_targetFrameIndex = rd.Int32();
		m_targetPosition = rd.Vector3d();
		m_toTransit = rd.Bool();
	}
	virtual void PostLoadFixup(Space *space) {
		AICommand::PostLoadFixup(space);
		m_targetFrame = space->GetFrameByIndex(m_targetFrameIndex);
	}
	virtual void OnDeleted(const Body *body) {
		AICommand::OnDeleted(body);
	}

private:
	Ship* m_ship;
	Frame* m_targetFrame;
	int m_targetFrameIndex;
	vector3d m_targetPosition;
	double m_remainingDistance; // For debugging/status text
	bool m_toTransit;
};

/// SteerAround is specialized in the situation of steering around obstacles at high speed.
/// It is instantiated by AIParagonCmdFlyTo.
///
/// Example: traveling at Transit 2 a planet is in the way, the ship will steer around 
/// the planet from a distance so it doesn't have to get close then fly around it.
/// Example2: ship wants to fly from station 1 to station 2 on the same planet.
/// SteerAround takes it around the planet to a point directly above station 2.
///
/// Caution: SteerAround fails if ship is not in Transit range, this is by design so FlyTo
/// is the only command responsible for that.
class AIParagonCmdSteerAround : public AICommand {
public:
	AIParagonCmdSteerAround(Ship *ship, Frame* target_frame, vector3d target_pos);
	virtual ~AIParagonCmdSteerAround();

	virtual bool TimeStepUpdate();
	virtual void GetStatusText(char *str) {
		if (m_child) {
			m_child->GetStatusText(str);
		} else {
			snprintf(str, 255, "Paragon SteerAround: distance: %.1fkm, steer stage: %s, transit state: %i, juice: %.1f, action: %s",
				m_remainingDistance / 1000.0,
				m_stage == ESS_ENTER ? "ENTER" : m_stage == ESS_AROUND ? "AROUND" : m_stage == ESS_EXIT? "EXIT" : "END",
				m_ship->GetTransitState(), m_ship->GetJuice(),
				m_actionDesc.c_str());
		}
	}
	virtual void Save(Serializer::Writer &wr) {
		if (m_child) { delete m_child; m_child = 0; }
		AICommand::Save(wr);
		wr.Int32(Pi::game->GetSpace()->GetIndexForFrame(m_targetFrame));
		wr.Vector3d(m_targetPosition);
		wr.Vector3d(m_entryPoint);
		wr.Vector3d(m_exitPoint);
		wr.Int32(static_cast<int>(m_stage));
		wr.Bool(m_heatDisposalMode);
	}
	AIParagonCmdSteerAround(Serializer::Reader &rd) : AICommand(rd, CMD_PARAGON_TRANSIT) {
		m_targetFrameIndex = rd.Int32();
		m_targetPosition = rd.Vector3d();
		m_entryPoint = rd.Vector3d();
		m_exitPoint = rd.Vector3d();
		m_stage = static_cast<ESteerStage>(rd.Int32());
		m_heatDisposalMode = rd.Bool();
	}
	virtual void PostLoadFixup(Space *space) {
		AICommand::PostLoadFixup(space);
		m_targetFrame = space->GetFrameByIndex(m_targetFrameIndex);
		CacheData();
	}

private:
	struct SSteerAroundData { // Cache for repetitively calculated data
		vector3d ship_pos;
		double ship_to_target_distance;
		vector3d ship_to_target_dir;

		Frame* sframe;
		vector3d target_pos;
		Body* sbody;
		Object::Type sbody_type;
		vector3d sbody_pos;
		double ship_to_sbody_distance;
		vector3d ship_to_sbody_dir;
		double sbody_radius;
		double sbody_transit_radius;
		double target_to_sbody_distance;
		vector3d target_to_sbody_dir;
		vector3d target_to_transit;
		vector3d ship_to_exit_dir;
		double ship_to_exit_distance;
	} m_data;
	void CacheData();

	enum ESteerStage {
		ESS_ENTER,
		ESS_AROUND,
		ESS_EXIT,
		ESS_END,
	};

	void CalculateSteerPoints();
	void CalculateEntryPoint();
	void CalculateExitPoint();
	bool ApplyMotion(const vector3d& target_destination);

	Frame* m_targetFrame;
	int m_targetFrameIndex;
	vector3d m_targetPosition;
	vector3d m_entryPoint;
	vector3d m_exitPoint;
	ESteerStage m_stage;
	double m_remainingDistance; // For debugging/status text
	bool m_heatDisposalMode;
	std::string m_actionDesc;
};

class AIParagonCmdTransit : public AICommand {
public:
	AIParagonCmdTransit(Ship *ship, Frame *target_frame, vector3d target_location);
	virtual ~AIParagonCmdTransit();

	virtual bool TimeStepUpdate();
	virtual void GetStatusText(char *str) {
		if (m_child) {
			m_child->GetStatusText(str);
		} else {
			snprintf(str, 255, "TransitTo Paragon: %.1fkm, transit state: %i, juice: %.1f",
				m_distance / 1000.0, m_ship->GetTransitState(), m_ship->GetJuice());
		}
	}
	virtual void Save(Serializer::Writer &wr) {
		if (m_child) { delete m_child; m_child = 0; }
		AICommand::Save(wr);
		wr.Double(m_distance);
		wr.Int32(Pi::game->GetSpace()->GetIndexForFrame(m_targetFrame));
		wr.Vector3d(m_targetLocation);
	}
	AIParagonCmdTransit(Serializer::Reader &rd) : AICommand(rd, CMD_PARAGON_TRANSIT) {
		m_distance = rd.Double();
		m_targframeIndex = rd.Int32();
		m_targetLocation = rd.Vector3d();
	}
	virtual void PostLoadFixup(Space *space) {
		AICommand::PostLoadFixup(space);
		m_targetFrame = space->GetFrameByIndex(m_targframeIndex);
	}

private:
	Frame *m_targetFrame;	// target frame
	vector3d m_targetLocation; // target location in target frame
	int m_targframeIndex;	// used during deserialisation
	double m_distance;
};

#endif
