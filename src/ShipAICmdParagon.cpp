// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt
#include "VSLog.h"

#include "libs.h"
#include "Ship.h"
#include "ShipAICmdParagon.h"
#include "Pi.h"
#include "WorldView.h"
#include "Player.h"
#include "perlin.h"
#include "Frame.h"
#include "Planet.h"
#include "SpaceStation.h"
#include "Space.h"
#include "Sound.h"
#include "Sfx.h"
#include "ShipCpanel.h"

const double VICINITY_DISTANCE = 50000.0;
const double ARRIVAL_DISTANCE = 15000.0;
const double DESTINATION_RADIUS = 2000.0;
const float DOCKING_SPEED = 250.0f;

/*
Body *m_target;				// target for vicinity. Either this or targframe is 0
double m_arrivalRadius;		// target radius of arrival
Frame *m_targetFrame;		// target frame for waypoint
vector3d m_posOffset;		// offset in target frame
double m_endVelocity;		// target speed in direction of motion at end of path, positive only
bool m_tangent;				// true if path is to a tangent of the target frame's body
int m_state;

bool m_lockhead;
int m_targetIndex, m_targframeIndex;	// used during deserialisation
vector3d m_relDir;			// target direction relative to ship at last frame change
Frame *m_frame;				// last frame of ship
*/

static double GetTransitRadius(Body* body)
{
	if (body->IsType(Object::PLANET)) {
		return static_cast<TerrainBody*>(body)->GetMaxFeatureRadius() + 15000.0 + 2500.0;
	} else if (body->IsType(Object::SPACESTATION)) {
		return TRANSIT_GRAVITY_RANGE_1;
	} else {
		return body->GetPhysRadius();
	}
}

static double GetArrivalRadius(Body* body)
{
	if (body->IsType(Object::PLANET)) {
		return static_cast<Planet*>(body)->GetAtmosphereRadius();
	} else {
		return 3500.0;
	}
}

/*static bool FutureCollision(Ship* ship) {
	Frame* frame = ship->GetFrame();
	Body* target = frame->GetBody();
	if (target->IsType(Object::PLANET) || target->IsType(Object::SPACESTATION)) {
		double radius = GetTransitRadius(target);
		double timeStep = Pi::game->GetTimeStep();
		vector3d target_pos = target->GetPositionRelTo(frame);
		vector3d ship_pos = ship->GetPositionRelTo(frame);
		// Find out the closest distance between ship's path and target
		double ship_speed, target_distance;
		vector3d ship_dir = ship->GetVelocityRelTo(frame).Normalized(ship_speed);
		vector3d ship_to_target_dir = (target_pos - ship_pos).Normalized(target_distance);
		double cos = ship_dir.Dot(ship_to_target_dir);
		if (cos > 0.0) {
			// Closest point along path of travel
			vector3d closest_point = ship_pos + (ship_dir * (target_distance / cos));
			double closest_distance;
			vector3d target_to_closest_dir = (closest_point - target_pos).Normalized(closest_distance);
			if (closest_distance < radius) {
				// path passes through target body
				return true;
			} else {
				return false;
			}
		} else {
			return false;
		}
	} else {
		return false;
	}
}*/

// Checks ship path to destination for obstacles.
// clear_angle is angle
// returns true if path is clear, false if there is an obstacle.
static bool CheckClearPath(Ship* ship, Frame* destination_frame, vector3d destination_pos, float clear_angle = 0.0f)
{
	Body* sbody = ship->GetFrame()->GetBody();
	if (sbody == nullptr) {
		return true;
	}
	if (sbody->IsType(Object::PLANET) 
		|| (sbody->IsType(Object::SPACESTATION) && ship->GetFrame() != destination_frame)) 
	{
		vector3d ship_pos = ship->GetPositionRelTo(destination_frame);
		vector3d body_pos = sbody->GetPositionRelTo(destination_frame);
		double radius = GetTransitRadius(sbody);
		if (sbody->IsType(Object::PLANET)) {
			radius += 5000.0;
		}
		double ship_to_body_distance;
		vector3d ship_to_body_dir = (body_pos - ship_pos).Normalized(ship_to_body_distance);
		double ship_to_destination_distance;
		vector3d ship_to_destination_dir = (destination_pos - ship_pos).Normalized(ship_to_destination_distance);
		double cos = ship_to_destination_dir.Dot(ship_to_body_dir);
		if (cos > clear_angle) { // Path might be unclear
			double ship_to_closest_distance = cos * ship_to_body_distance;
			vector3d closest_point = ship_pos + (ship_to_destination_dir * ship_to_closest_distance);
			double closest_distance = (closest_point - body_pos).Length();
			if (closest_distance < radius && ship_to_closest_distance < ship_to_destination_distance) {
				return false;
			} else {
				return true;
			}
		} else {
			return true;
		}
	} else {
		return true;
	}
}

bool LineSphereIntersection(const vector3d& center, double radius, const vector3d& line_start,
	const vector3d& line_end, vector3d& intersection)
{
	vector3d direction = (line_end - line_start).Normalized();
	double start_to_center_distance;
	vector3d start_to_center_dir = (center - line_start).Normalized(start_to_center_distance);
	double closest_to_center_distance = start_to_center_distance * (start_to_center_dir.Dot(direction));
	double closest_intersection_to_center_distance = pow(radius, 2) - pow(start_to_center_distance, 2) +
		pow(closest_to_center_distance, 2);
	if (closest_intersection_to_center_distance < 0.0) {
		return false;
	} else {
		intersection = line_start +
			(direction * (closest_to_center_distance - sqrt(closest_intersection_to_center_distance)));
		return true;
	}
}

/*void FrameCorrectVector(const Frame* from_frame, const Frame* to_frame, vector3d& v)
{
	matrix4x4d frame_transform = matrix4x4d::Identity();
	if (from_frame != to_frame) {
		to_frame->GetRotFrameTransform(from_frame, to_frame, frame_transform);
	}
	v = frame_transform * v;
}*/

void FrameCorrectPosition(const Frame* from_frame, const Frame* to_frame, vector3d& v)
{
	matrix4x4d frame_transform = matrix4x4d::Identity();
	if (from_frame != to_frame) {
		to_frame->GetFrameTransform(from_frame, to_frame, frame_transform);
	}
	v = frame_transform * v;
}

/// Calculates acceleration due to gravity at given position
static double GetGravityAtPos(Frame *target_frame, const vector3d &position)
{
	Body *body = target_frame->GetBody();
	if (!body || body->IsType(Object::SPACESTATION)) {
		return 0.0;
	}
	return G * body->GetMass() / position.LengthSqr(); // inverse is: sqrt(G * m1m2 / thrust)	
}

/// Converts position to another frame
static inline vector3d GetPosInFrame(Frame *current_frame, Frame *target_frame, const vector3d &position)
{
	return target_frame->GetOrientRelTo(current_frame) * position +
		target_frame->GetPositionRelTo(current_frame);
}

/// Converts unity direction to another frame (dir must be a normal)
static inline vector3d GetDirInFrame(Frame *current_frame, Frame *target_frame, const vector3d &dir)
{
	return target_frame->GetOrientRelTo(current_frame) * dir;
}

//------------------------------- Command: Paragon Fly To
AIParagonCmdFlyTo::AIParagonCmdFlyTo(Ship *ship, Frame *target_frame, const vector3d &pos_offset, 
	double end_velocity, bool tangent) : AICommand(ship, CMD_PARAGON_FLYTO)
{
	m_target = nullptr;
	m_targetPosition = pos_offset;
	m_targetFrame = target_frame;
	m_endVelocity = end_velocity;
	//m_arrivalRadius = pos_offset.Length();
	m_arrivalRadius = 3500.0;
	m_targetType = target_frame->GetBody()->GetType();
	CacheData();
	GoToTransitDistance();
}

AIParagonCmdFlyTo::AIParagonCmdFlyTo(Ship *ship, Body *target)
	: AICommand(ship, CMD_PARAGON_FLYTO)
{
	m_target = target;
	m_targetFrame = target->GetFrame();
	m_arrivalRadius = GetArrivalRadius(target);
	m_endVelocity = 0.0;
	m_targetType = target->GetType();
	m_targetPosition = target->GetPosition();
	if (m_targetPosition.LengthSqr() <= 1.0) {
		m_targetPosition =
			(m_ship->GetPositionRelTo(m_targetFrame) - m_target->GetPosition()).Normalized() * m_arrivalRadius;
	}
	CacheData();
	GoToTransitDistance();
}

AIParagonCmdFlyTo::AIParagonCmdFlyTo(Ship *ship, Body *target, double distance)
	: AICommand(ship, CMD_PARAGON_FLYTO)
{
	m_target = target;
	m_targetFrame = target->GetFrame();
	m_targetPosition = target->GetPositionRelTo(m_targetFrame);
	m_endVelocity = 0.0;
	m_arrivalRadius = distance;
	m_targetType = target->GetType();
	CacheData();
	GoToTransitDistance();
}

AIParagonCmdFlyTo::~AIParagonCmdFlyTo()
{
}

void AIParagonCmdFlyTo::CacheData()
{
	m_data.ship_pos = m_ship->GetPositionRelTo(m_targetFrame);
	m_data.ship_to_target_dir = (m_targetPosition - m_data.ship_pos).Normalized(m_data.ship_to_target_distance);
	m_data.fbody = m_targetFrame->GetBody();
	m_data.ship_to_fbody_dir = (m_data.fbody->GetPosition() - m_data.ship_pos).Normalized(
		m_data.ship_to_fbody_distance);
	m_data.fbody_transit_radius = GetTransitRadius(m_data.fbody);
}

// Creates a child AI command to fly the ship to nearest transit location if it wasn't within transit location
// Returns false if ship is already in transit location, true if it wasn't and a new AI command is created to go there
bool AIParagonCmdFlyTo::GoToTransitDistance()
{
	m_remainingDistance = m_data.ship_to_target_distance;
	// is target far enough for transit and transit is not possible right now?
	if (m_remainingDistance > VICINITY_DISTANCE && !m_ship->IsTransitPossible()) {
		Frame* sframe = m_ship->GetFrame();
		Body* sbody = sframe->GetBody();
		assert(sframe && sbody);
		double transit_radius = GetTransitRadius(sbody);
		// if transit is not possible (IsTransitPossible) then ship is within planet or spacestation so no need to check for that
		vector3d transit_position = (m_ship->GetPosition() - sbody->GetPosition()).Normalized() * transit_radius;
		m_child = new AIParagonCmdGoTo(m_ship, sframe, transit_position);
		return true;
	} else {
		return false;
	}
}

bool AIParagonCmdFlyTo::TimeStepUpdate()
{
	if (!ProcessChild()) return false;
	if (m_target) {
		m_targetPosition = m_target->GetPosition();
		if (m_targetType == Object::PLANET && m_targetPosition.LengthSqr() <= 1.0) {
			m_targetPosition =
				(m_ship->GetPositionRelTo(m_targetFrame) - m_target->GetPosition()).Normalized() * TRANSIT_GRAVITY_RANGE_1;
		}
	}
	CacheData();

	if (GoToTransitDistance()) return false;

	// Calculate direction and distance of travel
	bool clear_path = CheckClearPath(m_ship, m_targetFrame, m_targetPosition);
	if (!clear_path) {
		m_child = new AIParagonCmdSteerAround(m_ship, m_targetFrame, m_targetPosition);
		return false;
	}

	double distance = m_data.ship_to_target_distance - m_arrivalRadius;
	m_remainingDistance = distance;

	// If Transit drive 2 is active monitor the distance to slightly lower it when approaching the planet
	if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT
		&& m_ship->GetController()->GetSpeedLimit() > TRANSIT_DRIVE_1_SPEED
		&& distance < TRANSIT_DRIVE_2_SPEED) 
	{
		m_ship->GetController()->SetSpeedLimit(TRANSIT_DRIVE_1_SPEED);
		m_ship->SetVelocity(-m_ship->GetOrient().VectorZ() * std::max<double>(distance, TRANSIT_DRIVE_1_SPEED));
	}
	// Transit/Maneuver mode depends on how far the target location is
	bool closer = m_targetPosition.LengthSqr() > m_data.ship_pos.LengthSqr();
	if (!closer) {
		if (distance > ARRIVAL_DISTANCE) {
			if (m_ship->GetFlightMode() != Ship::EFM_TRANSIT && m_ship->IsTransitPossible()) {
				m_ship->StartTransitDrive();
			}
		} else if (distance > DESTINATION_RADIUS) {
			if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
				m_ship->StopTransitDrive();
			}
		} else {
			m_ship->GetController()->SetSpeedLimit(m_ship->GetMaxManeuverSpeed());
			m_ship->SetVelocity(-m_ship->GetOrient().VectorZ() * std::min<double>(distance, m_ship->GetMaxManeuverSpeed())); // Creates a crash.
			return true;
		}
	} else {
		if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
			m_ship->StopTransitDrive();
		}
		return true;
	}
	m_ship->AIFaceDirection(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * m_data.ship_to_target_dir);
	m_ship->CalculateVelocity(false);

	return false;
}

//------------------------------- Command: Steer Around
AIParagonCmdSteerAround::AIParagonCmdSteerAround(Ship *ship, Frame* target_frame, vector3d target_pos)
: AICommand(ship, CMD_PARAGON_STEERAROUND)
{
	m_ship = ship;
	m_targetFrame = target_frame;
	m_targetPosition = target_pos;
	m_stage = ESS_ENTER;
	m_remainingDistance = 0.0;
	m_heatDisposalMode = false;
	m_actionDesc = "Constructor";
	assert(m_targetFrame && m_targetFrame->GetBody() && ship->GetFrame()->GetBody());
	CacheData(); 
	CalculateEntryPoint();
	CalculateExitPoint();
}

AIParagonCmdSteerAround::~AIParagonCmdSteerAround()
{
}

void AIParagonCmdSteerAround::CacheData()
{
	m_data.sframe = m_ship->GetFrame();
	m_data.target_pos = m_targetPosition;
	FrameCorrectPosition(m_targetFrame, m_data.sframe, m_data.target_pos);

	m_data.ship_pos = m_ship->GetPositionRelTo(m_data.sframe);
	m_data.ship_to_target_dir = (m_data.target_pos - m_data.ship_pos).Normalized(
		m_data.ship_to_target_distance);

	if (m_data.target_pos.LengthSqr() <= 1.0) {
		m_data.target_pos = -m_data.ship_to_target_dir * TRANSIT_GRAVITY_RANGE_1;
	}

	m_data.sbody = m_data.sframe->GetBody();
	m_data.sbody_type = m_data.sbody->GetType();
	m_data.sbody_pos = m_data.sbody->GetPosition();
	m_data.ship_to_sbody_dir = (m_data.sbody_pos - m_data.ship_pos).Normalized(
		m_data.ship_to_sbody_distance);
	m_data.sbody_radius = m_data.sbody->GetPhysRadius();
	m_data.sbody_transit_radius = GetTransitRadius(m_data.sbody);

	if ((m_data.sbody_pos - m_data.target_pos).LengthSqr() > DESTINATION_RADIUS * DESTINATION_RADIUS) {
		m_data.target_to_sbody_dir = (m_data.sbody_pos - m_data.target_pos).Normalized(m_data.target_to_sbody_distance);
		m_data.target_to_transit = m_data.target_pos + (m_data.target_to_sbody_dir *GetTransitRadius(m_targetFrame->GetBody()));
	} else {
		// Avoid floating point exceptions when normalizing zero vector
		m_data.target_to_sbody_dir = m_data.ship_to_sbody_dir;
		m_data.target_to_sbody_distance = m_data.sbody_transit_radius;
		m_data.target_to_transit = -m_data.ship_to_sbody_dir * m_data.sbody_transit_radius;
	}
}

bool AIParagonCmdSteerAround::TimeStepUpdate()
{
	if (!ProcessChild()) {
		return false;
	}
	if(m_ship->GetFrame()->GetBody() == nullptr) {
		return true;
	}
	if (!m_ship->IsTransitPossible()) { // SteerAround must always be in transit range, otherwise flyto can do the job
		//assert(false);
		m_child = new AIParagonCmdGoTo(m_ship, m_targetFrame, m_targetPosition, true);
		return false;
	}

	CacheData();

	if (m_data.target_to_sbody_distance > DESTINATION_RADIUS) {
		m_remainingDistance = SAFEACOS(-m_data.ship_to_sbody_dir.Dot(-m_data.target_to_sbody_dir)) *
			m_data.sbody_transit_radius;
	} else {
		m_remainingDistance = m_data.ship_to_sbody_distance - m_data.sbody_transit_radius;
	}

	if (m_stage == ESS_ENTER) {
		m_actionDesc = "Heading towards Entry point";
		CalculateEntryPoint();
		if (ApplyMotion(m_entryPoint)) {
			m_stage = ESS_AROUND;
		}
		return false;
	} else if (m_stage == ESS_AROUND) {
		m_actionDesc = "Steering around obstacle";
		CalculateExitPoint();
		const double transit_range = 2500.0;
		const double transit_low = m_data.sbody_transit_radius + transit_range;
		const double transit_high = m_data.sbody_transit_radius + (transit_range * 2.0);
		const double transit_altitude = transit_low + ((transit_high - transit_low) / 2.0);
		vector3d up_vector = -m_data.ship_to_sbody_dir;
		vector3d right_vector = m_data.ship_to_exit_dir.Cross(up_vector).Normalized();
		vector3d velocity_vector = up_vector.Cross(right_vector).Normalized();
		const double altitude = m_data.ship_to_sbody_distance;
		double transit_factor = 1.0;
		if (!m_heatDisposalMode && m_ship->GetHullTemperature() > 0.8) {
			m_heatDisposalMode = true;
		} else if (m_heatDisposalMode && m_ship->GetHullTemperature() <= 0.1) {
			m_heatDisposalMode = false;
		}
		if (altitude > transit_altitude) {
			if (altitude > transit_altitude + transit_range) {
				velocity_vector = velocity_vector + (-up_vector * 0.007);
			} else if(altitude > transit_altitude + 500.0) {
				velocity_vector = velocity_vector + (-up_vector * 0.002);
			} else if(altitude > transit_altitude + 100.0) {
				velocity_vector = velocity_vector + (-up_vector * 0.001);
			}
		} else if (altitude <= transit_altitude) {
			if (altitude < transit_altitude - transit_range) {
				velocity_vector = velocity_vector + (up_vector * 0.35);
			} else if(altitude < transit_altitude - 500.0) {
				velocity_vector = velocity_vector + (up_vector * 0.002);
			} else if (altitude < transit_altitude - 100.0) {
				velocity_vector = velocity_vector + (up_vector * 0.001);
			}
		}
		double distance = std::min<double>(m_remainingDistance, m_data.ship_to_exit_distance);
		if (distance > TRANSIT_DRIVE_1_SPEED) {
			if (!m_heatDisposalMode) {
				if (m_ship->GetFlightMode() != Ship::EFM_TRANSIT) {
					m_ship->StartTransitDrive();
				}
			} else {
				if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
					m_ship->StopTransitDrive();
				}
			}
		} else if (distance > DESTINATION_RADIUS) {
			if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
				transit_factor = Clamp(
					//m_remainingDistance / ARRIVAL_DISTANCE, 
					distance / TRANSIT_DRIVE_1_SPEED,
					0.1, 1.0);
			}
		} else {
			if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
				m_ship->StopTransitDrive();
			}
			m_stage = ESS_EXIT;
		}
		m_ship->AIFaceDirection(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * velocity_vector);
		m_ship->AIFaceUpdir(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * up_vector);
		m_ship->CalculateVelocity(true, transit_factor);
		return false;
	} else if (m_stage == ESS_EXIT) {
		m_actionDesc = "Heading towards exit point";
		CalculateExitPoint();
		const double distance = (m_data.ship_pos - m_exitPoint).Length();
		if (distance > DESTINATION_RADIUS) {
			vector3d exit_point = m_exitPoint;
			// Transfer exit point to target frame to pass it to goto
			FrameCorrectPosition(m_data.sframe, m_targetFrame, exit_point);
			m_child = new AIParagonCmdGoTo(m_ship, m_targetFrame, exit_point);
			//m_child = new AIParagonCmdGoTo(m_ship, m_targetFrame, exit_point, 
			//	m_ship->GetOrientRelTo(m_targetFrame), 0.0, m_ship->GetMaxManeuverSpeed());
			m_stage = ESS_END;
			return false;
		}
	}
	return true;
}

void AIParagonCmdSteerAround::CalculateEntryPoint()
{
	// Entry point
	if (m_data.ship_to_sbody_distance > m_data.sbody_transit_radius + 5000.0
		&& m_data.ship_to_sbody_distance <= m_data.sbody_transit_radius + 10000.0)
	{
		// - Ship is in transit range or nearby
		// --	Directly above or below to transit range is the entry point (Default)
		m_entryPoint = -m_data.ship_to_sbody_dir * m_data.sbody_transit_radius;
		m_stage = ESS_ENTER;
	} else if (m_data.ship_to_sbody_distance > m_data.sbody_transit_radius + 10000.0) {
		// - Ship is far from transit range
		vector3d intersection_point;
		bool intersection;
		// --	Calculate entry point
		if (m_data.target_to_sbody_distance > m_data.sbody_transit_radius) {
			// -- Target out of planet (WIP)
			// Ship takes a wide angle turn around the planet
			const double angle = DEG2RAD(45.0);
			const double ship_to_entry_distance = sqrt(2.0 * pow(m_data.ship_to_sbody_distance, 2));
			vector3d ship_up = m_ship->GetOrientRelTo(m_data.sframe).VectorY();
			
			// Get direction from ship to nearest entry point on planet hemisphere circular edge

			const vector3d ship_to_entry_dir =
				m_data.ship_to_sbody_dir * matrix4x4d::RotateMatrix(angle, ship_up.x, ship_up.y, ship_up.z);
			m_entryPoint = m_data.ship_pos + (ship_to_entry_dir * ship_to_entry_distance);

			// Planet could intersect path, in which case intersection point is the entry point
			if (LineSphereIntersection(m_data.sbody_pos, m_data.sbody_transit_radius,
				m_data.ship_pos, m_entryPoint, intersection_point)) {
				m_entryPoint = intersection_point;
			}
			m_stage = ESS_ENTER;
		} else {
			// -- Target in planet
			// is target in front or back hemisphere relative to ship?
			if (m_data.target_to_sbody_distance <= DESTINATION_RADIUS ||				
				-m_data.ship_to_sbody_dir.Dot(-m_data.target_to_sbody_dir) >= 0.0) {
				// Front hemisphere
				// Calculate location at which ship will enter transit-around heading towards target
				intersection = LineSphereIntersection(m_data.sbody_pos, m_data.sbody_transit_radius, 
					m_data.ship_pos, m_data.target_to_transit, intersection_point);
				assert(intersection);	// There should always be an intersection otherwise this code path would not run
										// If this exception is hit, it means there is a problem with the code that determines obstacles
				m_entryPoint = intersection_point;
				m_stage = ESS_ENTER;
			} else {
				// Back hemisphere
				// Enter point is the point that is exactly at 90 degrees planet-transit-radius from ship
				// If ship intersects radius before it reaches entry point then intersection point is the entry point
				const double angle = atan2(m_data.ship_to_sbody_distance, m_data.sbody_transit_radius);
				const double ship_to_entry_distance = sqrt(pow(m_data.ship_to_sbody_distance, 2) +
					pow(m_data.sbody_transit_radius, 2));
				const vector3d ship_up = m_ship->GetOrientRelTo(m_data.sframe).VectorY();

				// Get direction from ship to nearest entry point on planet hemisphere-edge
				// Note that entry point can be set anywhere around the planet's diameter from the ship's perspective
				vector3d sbody_to_target = -m_data.target_to_sbody_dir;
				sbody_to_target = sbody_to_target / ship_up.Dot(sbody_to_target);
				vector3d ship_to_entry_dir = (sbody_to_target - ship_up).Normalized();
				m_entryPoint = m_data.ship_pos + (ship_to_entry_dir * ship_to_entry_distance);

				// Planet could intersect path, in which case intersection point is the entry point
				if (LineSphereIntersection(m_data.sbody_pos, m_data.sbody_transit_radius,
					m_data.ship_pos, m_entryPoint, intersection_point)) {
					m_entryPoint = intersection_point;
				}
				m_stage = ESS_ENTER;
			}
		}
	} else {
		// --	Ship is already at entry point
		m_entryPoint = m_data.ship_pos;
		m_stage = ESS_AROUND;
	}
}

void AIParagonCmdSteerAround::CalculateExitPoint()
{
	// Exit point	
	if (m_data.target_to_sbody_distance <= m_data.sbody_transit_radius + 100.0) { // -100 to eliminate floating point errors
		// - Target is under transit radius (on planet surface)
		// --	Directly above or below to transit range is the exit point
		m_exitPoint = -m_data.target_to_sbody_dir * m_data.sbody_transit_radius;
	} else {
		// - Target is far above transit radius
		// --	Calculate exit point (WIP)
		if (CheckClearPath(m_ship, m_targetFrame, m_targetPosition, -0.1f)) {
			m_exitPoint = m_data.ship_pos +(m_ship->GetOrient().VectorZ() * 0.1);
		} else {
			m_exitPoint = -m_data.target_to_sbody_dir * m_data.sbody_transit_radius;
		}
	}
	m_data.ship_to_exit_dir = (m_exitPoint - m_data.ship_pos).Normalized(m_data.ship_to_exit_distance);
}

bool AIParagonCmdSteerAround::ApplyMotion(const vector3d& target_destination)
{
	vector3d ship_to_target = m_data.target_pos - m_data.ship_pos;
	// Calculate direction and distance of travel
	if (m_ship->IsTransitPossible()) {
		if (m_ship->GetFlightMode() == Ship::EFM_MANEUVER) {
			m_ship->StartTransitDrive();
		} else if (m_data.ship_to_target_distance <= m_ship->GetController()->GetSpeedLimit()) {
			m_ship->GetController()->SetSpeedLimit(
				std::max<double>(m_data.ship_to_target_distance, m_ship->GetMaxManeuverSpeed()));
			if (m_data.ship_to_target_distance <= m_ship->GetMaxManeuverSpeed()) {
				m_ship->StopTransitDrive();
				return true;
			}
		}
	} else {
		if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
			m_ship->StopTransitDrive();
		} else if (m_data.ship_to_target_distance <= m_ship->GetMaxManeuverSpeed()) {
			return true;
		}
		m_ship->GetController()->SetSpeedLimit(m_ship->GetMaxManeuverSpeed());
	}
	if (m_data.ship_to_target_distance >= 1.0) {
		m_ship->AIFaceDirection(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * 
			ship_to_target.Normalized());
	}
	m_ship->CalculateVelocity(false);
	return false;
}

//------------------------------- Command: Go To
// TODO
// - Add low altitude detection: GoTo detects when ship starts flying from a low altitude point towards the planet
//   then it first adds a new GoTo child command to take the ship to a safe altitude then resumes.
//   How to handle ground stations though?

// Simple mode
AIParagonCmdGoTo::AIParagonCmdGoTo(Ship *ship, Frame *target_frame, vector3d target_position, bool to_transit_distance)
	: AICommand(ship, CMD_PARAGON_GOTO), m_remainingDistance(0.0)
{
	m_targetFrame = target_frame;
	m_targetPosition = target_position;
	m_speedLimit = m_ship->GetMaxManeuverSpeed();
	m_arrivalSpeed = 0.0;
	m_toTransit = to_transit_distance;
	m_mode = to_transit_distance? EGTM_TRANSIT : EGTM_SIMPLE;
}

// Precision mode spacestation
AIParagonCmdGoTo::AIParagonCmdGoTo(Ship *ship, Frame *target_frame,
	SpaceStationType::positionOrient_t target_posorient, double arrival_speed = 0.0,
	double speed_limit = DOCKING_SPEED)
	: AICommand(ship, CMD_PARAGON_GOTO), m_remainingDistance(0.0)
{
	m_targetFrame = target_frame;
	m_targetPosition = target_posorient.pos;
	m_targetOrient = matrix3x3d::FromVectors(target_posorient.xaxis, target_posorient.yaxis, 
		target_posorient.zaxis);
	m_speedLimit = speed_limit;
	m_arrivalSpeed = arrival_speed;
	m_mode = EGTM_PRECISION;
}

// Precision mode pos + orient
AIParagonCmdGoTo::AIParagonCmdGoTo(Ship *ship, Frame *target_frame,
	vector3d target_position, matrix3x3d target_orient, double arrival_speed = 0.0,
	double speed_limit = DOCKING_SPEED)
	: AICommand(ship, CMD_PARAGON_GOTO), m_remainingDistance(0.0)
{
	m_targetFrame = target_frame;
	m_targetPosition = target_position;
	m_targetOrient = target_orient;
	m_speedLimit = speed_limit;
	m_arrivalSpeed = arrival_speed;
	m_mode = EGTM_PRECISION;
}

// Docking mode
AIParagonCmdGoTo::AIParagonCmdGoTo(Ship *ship, Frame *target_frame, 
	SpaceStationType::positionOrient_t start_posorient,
	SpaceStationType::positionOrient_t end_posorient)
	: AICommand(ship, CMD_PARAGON_GOTO), m_remainingDistance(0.0)
{
	m_targetFrame = target_frame;
	m_startPO.pos = start_posorient.pos;
	m_startPO.xaxis = start_posorient.xaxis.NormalizedSafe();
	m_startPO.yaxis = start_posorient.yaxis.NormalizedSafe();
	m_startPO.zaxis = start_posorient.zaxis.NormalizedSafe();
	m_endPO = end_posorient;
	m_endPO.xaxis = end_posorient.xaxis.NormalizedSafe();
	m_endPO.yaxis = end_posorient.yaxis.NormalizedSafe();
	m_endPO.zaxis = end_posorient.zaxis.NormalizedSafe();
	m_speedLimit = std::min(DOCKING_SPEED, m_ship->GetMaxManeuverSpeed());
	m_arrivalSpeed = 0.0;
	m_mode = EGTM_DOCK;
}

AIParagonCmdGoTo::~AIParagonCmdGoTo()
{

}


void AIParagonCmdGoTo::GetStatusText(char *str) 
{
	if (m_child) {
		m_child->GetStatusText(str);
	} else {
		std::string mode;
		static const char * mode_labels [] = { "SIMPLE", "TRANSIT", "PRECISION", "DOCK" };

		//snprintf(str, 255, "Paragon GoTo: mode: %s, distance: %.1fkm, juice: %.1f, angle: %.3f",
		//	mode_labels[static_cast<int>(m_mode)], m_remainingDistance, m_ship->GetJuice());

		if(m_ship->GetVelocityRelTo(m_targetFrame).LengthSqr() < 1.0) {
			snprintf(str, 255, "Paragon GoTo: mode: %s, distance: %.1fkm, juice: %.1f, angle: NA",
				mode_labels[static_cast<int>(m_mode)], m_remainingDistance, m_ship->GetJuice());
		} else {
			vector3d ship_to_target_dir = (m_targetPosition -
				m_ship->GetPositionRelTo(m_targetFrame)).Normalized();
			vector3d ship_dir = m_ship->GetVelocityRelTo(m_targetFrame).Normalized();
			float dp = ship_to_target_dir.Dot(ship_dir);
			snprintf(str, 255, "Paragon GoTo: mode: %s, distance: %.1fkm, juice: %.1f, angle: %f",
				mode_labels[static_cast<int>(m_mode)], m_remainingDistance, m_ship->GetJuice(),
				dp);
		}
	}
}

void AIParagonCmdGoTo::Save(Serializer::Writer &wr) 
{
	if (m_child) { 
		delete m_child; m_child = 0; 
	}
	AICommand::Save(wr);
	wr.Int32(Pi::game->GetSpace()->GetIndexForFrame(m_targetFrame));
	wr.Vector3d(m_targetPosition);
	wr.Bool(m_toTransit);
	wr.Vector3d(m_targetOrient.VectorX());
	wr.Vector3d(m_targetOrient.VectorY());
	wr.Vector3d(m_targetOrient.VectorZ());
	wr.Vector3d(m_startPO.pos);
	wr.Vector3d(m_startPO.xaxis);
	wr.Vector3d(m_startPO.yaxis);
	wr.Vector3d(m_startPO.zaxis);
	wr.Vector3d(m_endPO.pos);
	wr.Vector3d(m_endPO.xaxis);
	wr.Vector3d(m_endPO.yaxis);
	wr.Vector3d(m_endPO.zaxis);
	wr.Double(m_speedLimit);
	wr.Double(m_arrivalSpeed);
	wr.Int32(static_cast<int>(m_mode));
}

AIParagonCmdGoTo::AIParagonCmdGoTo(Serializer::Reader &rd) : AICommand(rd, CMD_PARAGON_GOTO) 
{
	m_targetFrameIndex = rd.Int32();
	m_targetPosition = rd.Vector3d();
	m_toTransit = rd.Bool();

	if(Game::s_loadedGameVersion >= 77) { // Save game upgrade: 76 -> 77
		m_targetOrient = matrix3x3d::FromVectors(
			rd.Vector3d(), rd.Vector3d(), rd.Vector3d());
		m_startPO.pos = rd.Vector3d();
		m_startPO.xaxis = rd.Vector3d();
		m_startPO.yaxis = rd.Vector3d();
		m_startPO.zaxis = rd.Vector3d();

		m_endPO.pos = rd.Vector3d();
		m_endPO.xaxis = rd.Vector3d();
		m_endPO.yaxis = rd.Vector3d();
		m_endPO.zaxis = rd.Vector3d();

		m_speedLimit = rd.Double();
		m_arrivalSpeed = rd.Double();

		m_mode = static_cast<EGoToMode>(rd.Int32());
	}
}

void AIParagonCmdGoTo::PostLoadFixup(Space *space)
{
	AICommand::PostLoadFixup(space);
	m_targetFrame = space->GetFrameByIndex(m_targetFrameIndex);
}

void AIParagonCmdGoTo::OnDeleted(const Body *body)
{
	AICommand::OnDeleted(body);
}

bool AIParagonCmdGoTo::TimeStepUpdate()
{
	vector3d ship_position = m_ship->GetPositionRelTo(m_targetFrame);
	double ship_to_target_distance;
	vector3d ship_to_target_dir;

	if(m_mode == EGTM_SIMPLE) {	// Go close to target position (end distance = maneuver speed)
		ship_to_target_dir = (m_targetPosition - ship_position).Normalized(ship_to_target_distance);
		m_remainingDistance = ship_to_target_distance;

		if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
			m_ship->StopTransitDrive();
		}
		if (ship_to_target_distance <= m_ship->GetMaxManeuverSpeed()) {
			return true;
		}

		m_ship->GetController()->SetSpeedLimit(m_ship->GetMaxManeuverSpeed());		
		m_ship->AIFaceDirection(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * ship_to_target_dir);
		m_ship->CalculateVelocity(false);
	} else if(m_mode == EGTM_TRANSIT) { // Go to nearest transitable position
		if (m_ship->IsTransitPossible()) {
			return true;
		}
		ship_to_target_dir = (ship_position - 
			m_ship->GetFrame()->GetBody()->GetPositionRelTo(m_targetFrame)).Normalized(ship_to_target_distance);
		m_remainingDistance = ship_to_target_distance - GetTransitRadius(m_ship->GetFrame()->GetBody());

		if (m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
			m_ship->StopTransitDrive();
		}

		m_ship->GetController()->SetSpeedLimit(m_ship->GetMaxManeuverSpeed());
		m_ship->AIFaceDirection(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * ship_to_target_dir);
		m_ship->CalculateVelocity(false);
	} else if(m_mode == EGTM_PRECISION) { // Go to precise position + orientation
		// TODO: should also check for angle before returning true (for stationary repositioning)
		ship_to_target_dir = (m_targetPosition - ship_position).Normalized(ship_to_target_distance);
		m_remainingDistance = ship_to_target_distance;
		if(m_ship->GetFlightMode() == Ship::EFM_TRANSIT) {
			m_ship->StopTransitDrive();
		}
		if(ship_to_target_distance > m_speedLimit) {
			m_ship->GetController()->SetSpeedLimit(m_speedLimit);
		} else if(ship_to_target_distance > 10.0) {
			m_ship->GetController()->SetSpeedLimit(ship_to_target_distance);
		} else {
			m_ship->GetController()->SetSpeedLimit(0.0);
			m_ship->SetVelocity(vector3d(0.0, 0.0, 0.0));
			m_ship->ClearThrusterState();
			m_ship->SetAngThrusterState(vector3d(0.0, 0.0, 0.0));
			return true;
		}

		m_ship->AIFaceUpdir(m_targetOrient.VectorY());
		m_ship->AIFaceDirection(m_targetFrame->GetOrientRelTo(m_ship->GetFrame()) * ship_to_target_dir);
		m_ship->CalculateVelocity(false);
	} else if(m_mode ==EGTM_DOCK) { // First orient according to end point, then move to position
		matrix3x3d ship_orient = m_ship->GetOrientRelTo(m_targetFrame);		
		const matrix3x3d ai_orient = m_targetFrame->GetOrientRelTo(m_ship->GetFrame());
		bool move_ship = true;

		float yangle = m_endPO.yaxis.Dot(ship_orient.VectorY());
		if (yangle <= 0.99) {
			m_ship->AIFaceUpdir(ai_orient * m_endPO.yaxis);
			move_ship = false;
		}

		float zangle = m_endPO.zaxis.Dot(ship_orient.VectorZ());
		if(zangle <= 0.99) {
			m_ship->AIFaceDirection(ai_orient * (-m_endPO.zaxis));
			move_ship = false;
		}

		if(move_ship) {
			// Fix ship orientation
			m_ship->SetOrient(matrix3x3d::FromVectors(
				m_endPO.yaxis.Cross(m_endPO.zaxis).Normalized(), m_endPO.yaxis, m_endPO.zaxis));
			m_ship->SetAngThrusterState(vector3d(0.0, 0.0, 0.0));

			// Move ship to end position
			double dist;
			vector3d dir = (m_endPO.pos - ship_position).Normalized(dist);
			if(dist > 0.1) {
				double speed = dist < m_speedLimit? std::max(dist, 10.0) : m_speedLimit;
				m_ship->AIMatchVel(ai_orient * (dir * speed), true);
			} else {
				m_ship->ClearThrusterState();
				m_ship->SetVelocity(vector3d(0.0, 0.0, 0.0));
				return true;
			}
		}
	}
	return false;
}

//------------------------------- Command: Paragon Transit
AIParagonCmdTransit::AIParagonCmdTransit(Ship *ship, Frame *target_frame, vector3d target_location)
	: AICommand(ship, CMD_PARAGON_TRANSIT)
{
	m_targetFrame = target_frame;
	m_targetLocation = target_location;
	m_ship->SetTransitState(TransitState::TRANSIT_DRIVE_READY);
}

AIParagonCmdTransit::~AIParagonCmdTransit()
{

}

bool AIParagonCmdTransit::TimeStepUpdate()
{
	return false;
}

//-------------------------------- Command: Paragon Orbit
AIParagonCmdOrbit::AIParagonCmdOrbit(Ship *ship, Frame *target_frame, double altitude_multiplier)
	: AICommand(ship, CMD_PARAGON_ORBIT)
{
	assert(target_frame && target_frame->GetBody() && target_frame->GetBody()->IsType(Object::TERRAINBODY));
	m_targetFrame = target_frame;
	m_altitudeMultiplier = altitude_multiplier;
}

AIParagonCmdOrbit::~AIParagonCmdOrbit()
{

}

bool AIParagonCmdOrbit::TimeStepUpdate()
{
	if (!ProcessChild()) {
		if (m_child->GetCommandName() == CmdName::CMD_PARAGON_FLYTO) {
			if (m_child->GetChildCommand() &&
				m_child->GetChildCommand()->GetCommandName() == CmdName::CMD_PARAGON_STEERAROUND)
			{
				AIParagonCmdSteerAround* cmd = reinterpret_cast<AIParagonCmdSteerAround*>(
					m_child->GetChildCommand());
				if (cmd->GetRemainingDistance() > 25000.0 && cmd->GetRemainingDistance() < 110000.0) {
					delete m_child;
					m_child = nullptr;
				}
			}
		}
		return false;
	}

	TerrainBody* planet = reinterpret_cast<TerrainBody*>(m_targetFrame->GetBody());
	assert(planet);

	// Orbit position is adjusted to avoid perfectly oposite direction (which might cause
	// vector math to go berserk with NaNs)
	vector3d ship_pos = m_ship->GetPositionRelTo(m_targetFrame) - (m_ship->GetOrientRelTo(m_targetFrame).VectorZ() * 5000.0);
	vector3d ship_to_planet_dir = (planet->GetPosition() - ship_pos).Normalized();
	vector3d orbit_position = planet->GetPosition() + (ship_to_planet_dir * GetTransitRadius(planet));
	m_child = new AIParagonCmdFlyTo(m_ship, m_targetFrame, orbit_position, 0, true);
	return false;
}

//---------------------------------------------------------------------------
//----------------------------------------------------- Command: Paragon Dock
AIParagonCmdDock::AIParagonCmdDock(Ship* ship, SpaceStation* station) 
	: AICommand(ship, CmdName::CMD_PARAGON_DOCK)
{
	assert(ship);
	m_station = station;
	m_state = EDS_ZERO;
	m_waitingForSignalMessage = false;
}

AIParagonCmdDock::~AIParagonCmdDock()
{

}

void AIParagonCmdDock::Save(Serializer::Writer &wr)
{
	if (m_child) { delete m_child; m_child = 0; }
	AICommand::Save(wr);
	Space* space = Pi::game->GetSpace();
	wr.Int32(space->GetIndexForBody(m_station));
	wr.Int32(static_cast<int>(m_state));
	wr.Int32(m_approachPoints.size());
	for(auto iter : m_approachPoints) {
		wr.Vector3d(iter.pos);
		wr.Vector3d(iter.xaxis);
		wr.Vector3d(iter.yaxis);
		wr.Vector3d(iter.zaxis);
	}
	wr.Int32(m_nextApproachPoint);
}

AIParagonCmdDock::AIParagonCmdDock(Serializer::Reader &rd)
	: AICommand(rd, CMD_PARAGON_DOCK)
{
	// an older save should never load a dock comand object since it didn't exist initially
	// but still, better be safe.
	if(Game::s_loadedGameVersion >= 77) { // Save game upgrade: 76 -> 77
		m_stationIndex = rd.Int32();
		m_state = static_cast<EDockingState>(rd.Int32());
		int ap_count = rd.Int32();
		m_approachPoints.clear();
		for (int i = 0; i < ap_count; ++i) {
			SpaceStationType::positionOrient_t po;
			po.pos = rd.Vector3d();
			po.xaxis = rd.Vector3d();
			po.yaxis = rd.Vector3d();
			po.zaxis = rd.Vector3d();
			m_approachPoints.push_back(po);
		}
		m_nextApproachPoint = rd.Int32();
	} 
}

void AIParagonCmdDock::PostLoadFixup(Space *space)
{
	AICommand::PostLoadFixup(space);
	m_station = static_cast<SpaceStation*>(space->GetBodyByIndex(m_stationIndex));
}

void AIParagonCmdDock::OnDeleted(const Body *body)
{
	AICommand::OnDeleted(body);	
	if (static_cast<Body*>(m_station) == body) {
		m_station = nullptr;
	}
}

void AIParagonCmdDock::GetStatusText(char *str)
{
	if (m_child) {
		m_child->GetStatusText(str);
	} else {
		static const char* docking_states [] = {
				"ZERO",
				"START",
				"WAIT FOR GO",
				"WAYPOINTS",
				"DOCKING",
				"DOCK"
			};
		snprintf(str, 255, "Paragon Dock, STATE: %s", docking_states[static_cast<int>(m_state)]);
	}
}

bool AIParagonCmdDock::TimeStepUpdate()
{
	m_ship->SetRemotlyDocked(false);
	if (m_ship->GetFlightState() == Ship::JUMPING || !ProcessChild()) {
		return false;
	}
	if (!m_station) {
		return true;
	}

	// Fly to station
	if (m_station->GetPositionRelTo(m_ship).Length() > GetArrivalRadius(m_station) + 100.0) {
		m_child = new AIParagonCmdFlyTo(m_ship, m_station);
		ProcessChild();
		return false;
	}
	
	if(m_state == EDS_ZERO) {
		if (m_ship->GetVelocity().Length() > 100.0) { // Anything I write here causes weird behavior!
			m_ship->SetDecelerating(true);
			m_ship->GetController()->SetSpeedLimit(0.0);
			m_ship->AIMatchVel(vector3d(0.0, 0.0, 0.0));
			return false;
		} else {
			m_ship->SetDecelerating(false);
			m_ship->SetJuice(1.0);
			m_ship->ClearThrusterState();
			m_ship->SetAngThrusterState(vector3d(0.0, 0.0, 0.0));
			m_ship->SetVelocity(vector3d(0.0, 0.0, 0.0));
			m_state = EDS_START;
			return false;
		}
	} else if (m_state == EDS_START) {				// Zero speed
		m_ship->SetVelocity(vector3d(0.0, 0.0, 0.0));
		int port = m_station->GetMyDockingPort(m_ship);
		if (port == -1) {
			std::string msg;
			bool cleared = m_station->GetDockingClearance(m_ship, msg);
			port = m_station->GetMyDockingPort(m_ship);
			if (!cleared || port == -1) {
				m_ship->AIMessage(Ship::AIERROR_REMOTE_DOCKING);
				m_ship->SetRemotlyDocked(true);
				port = m_station->GetRemoteDockingPort();
				m_ship->SetDockedWith(m_station, port);
				return true;
			}
		}
		//m_station->RequestDockingApproach(m_ship);
		m_state = EDS_WAIT_FOR_GO;
		return false;
	} else if (m_state == EDS_WAIT_FOR_GO) {// Get docking signal from station (docking queue)
		// Disabled for now, leaving station overrides the approach queue so it's a bit pointless.
		/*if(m_station->CheckDockingApproachSignal(m_ship)) {
			m_state = EDS_WAYPOINTS;
		} else {
			// Waiting for approach signal
			if(m_ship->IsPlayerShip() && !m_waitingForSignalMessage) {
				Pi::cpan->InfLog()->ImportantMessage(m_station->GetLabel(), "Standby for approach signal");
				m_waitingForSignalMessage = true;
			}
		}*/
		m_state = EDS_WAYPOINTS;
		return false;
	} else if (m_state == EDS_WAYPOINTS) {	// Prepare docking points
		SpaceStationType::positionOrient_t po1, po2;
		vector3d ship_pos = m_ship->GetPositionRelTo(m_station);

		bool got_orient_1 = m_station->GetStationType()->GetShipApproachWaypoints(
			m_station->GetMyDockingPort(m_ship), 1, po1);
		bool got_orient_2 = m_station->GetStationType()->GetShipApproachWaypoints(
			m_station->GetMyDockingPort(m_ship), 2, po2);
		m_approachPoints.push_back(po1); // Used for approach standby distance
		m_approachPoints.push_back(po1);
		m_approachPoints.push_back(po2);

		// Apply frame offset
		if (m_station->GetStationType()->dockMethod != SpaceStationType::ORBITAL) {
			for (unsigned int i = 0; i < m_approachPoints.size(); ++i) {
				/*m_approachPoints[i].pos -= m_approachPoints[i].yaxis.Normalized() * (
				m_ship->GetAabb().min.y + 1.0);*/
				m_approachPoints[i].pos = m_station->GetOrient() * m_approachPoints[i].pos +
					m_station->GetPosition();
				m_approachPoints[i].xaxis = m_station->GetOrient() * m_approachPoints[i].xaxis;
				m_approachPoints[i].yaxis = m_station->GetOrient() * m_approachPoints[i].yaxis;
				m_approachPoints[i].zaxis = m_station->GetOrient() * m_approachPoints[i].zaxis;
			}
			m_nextApproachPoint = 2;
		} else {
			// For orbital stations, point 0 should be standby for approach distance from point 1
			double station_to_point_dist;
			vector3d station_to_point = (m_approachPoints[0].pos - m_station->GetPosition()).Normalized(
				station_to_point_dist);
			m_approachPoints[0].pos = station_to_point * GetArrivalRadius(m_station);
			m_nextApproachPoint = 1;

			vector3d station_to_ship = (ship_pos - m_station->GetPosition()).Normalized();
			double ship_angle = station_to_point.Dot(station_to_ship);
			if(ship_angle < 0.0f) { // Ship needs to go around
				// Calculate perpendicular vector between station to point and station to ship
				vector3d v = -station_to_point;
				v = v * station_to_ship.Dot(v);
				SpaceStationType::positionOrient_t perp_po = m_approachPoints[0];
				perp_po.pos = (station_to_ship - v).NormalizedSafe() * GetArrivalRadius(m_station);
				m_approachPoints.insert(m_approachPoints.begin(),  perp_po);
			}
		}

		m_child = new AIParagonCmdGoTo(m_ship, m_station->GetFrame(), m_approachPoints[0]);
		ProcessChild();
		m_state = EDS_DOCKING;
		return false;
	} else if (m_state == EDS_DOCKING) {	// Proceed to dock
		m_ship->ClearThrusterState();
		m_ship->SetAngThrusterState(vector3d(0.0, 0.0, 0.0));

		if(m_station->GetStationType()->dockMethod == SpaceStationType::ORBITAL) {
			// Update end point (keeps changing due to station local rotation/translation)
			m_station->GetStationType()->GetShipApproachWaypoints(m_station->GetMyDockingPort(m_ship), 2,
				m_approachPoints.back());
			m_child = new AIParagonCmdGoTo(m_ship, m_station->GetFrame(), 
				m_approachPoints[m_nextApproachPoint]);
		} else {
			m_child = new AIParagonCmdGoTo(m_ship, m_station->GetFrame(),
				m_approachPoints[m_nextApproachPoint - 1], m_approachPoints[m_nextApproachPoint]);
		}

		ProcessChild();
		m_nextApproachPoint += 1;
		if(m_nextApproachPoint >= m_approachPoints.size()) {
			m_state = EDS_DOCK;
		}
		return false;
	} else if (m_state == EDS_DOCK) {		// Dock!
		return false;
	} /*else {
		VSLog::stream << "Paragon Docking: Unknown state " << m_state << std::endl;
		VSLog::outputLog();
	}*/

	return false;
}
