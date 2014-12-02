// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef CAMERACONTROLLER_H
#define CAMERACONTROLLER_H

#include "vector3.h"
#include "matrix4x4.h"
#include "Lang.h"
#include "Serializer.h"
#include "Camera.h"
#include "Easing.h"

class Ship;
namespace Easing { 
	template<typename T> class NumericTweener; 
}

class CameraController
{
public:
	enum Type { //can be used for serialization & identification
		INTERNAL,
		EXTERNAL,
		SIDEREAL
	};

	CameraController(RefCountedPtr<CameraContext> camera, const Ship *ship);
	virtual ~CameraController() {}

	virtual void Reset();

	virtual Type GetType() const = 0;
	virtual const char *GetName() const { return ""; }
	virtual void Save(Serializer::Writer &wr);
	virtual void Load(Serializer::Reader &rd);
	virtual bool IsExternal() const { return false; }

	// camera position relative to the body
	void SetPosition(const vector3d &pos) { m_pos = pos; }
	vector3d GetPosition() const { return m_pos; }

	// camera orientation relative to the body
	void SetOrient(const matrix3x3d &orient) { m_orient = orient; }
	const matrix3x3d &GetOrient() const { return m_orient; }

	virtual void Update();

	const Ship *GetShip() const { return m_ship; }
	const CameraContext *GetCameraContext() const { assert(m_camera.Valid()); return m_camera.Get(); }

	virtual void RollLeft(float frameTime) { }
	virtual void RollRight(float frameTime) { }
	virtual void RotateDown(float frameTime) { }
	virtual void RotateLeft(float frameTime) { }
	virtual void RotateRight(float frameTime) { }
	virtual void RotateUp(float frameTime) { }
	/// Zooming with this method will interrupt any animation launched by ZoomEvent().
	virtual void ZoomIn(float frameTime) { }
	/// Zooming with this method will interrupt any animation launched by ZoomEvent().
	virtual void ZoomOut(float frameTime) { }
	/// Animated zoom trigger (on each event), primarily designed for mouse wheel.
	///\param amount The zoom delta to add or substract (>0: zoom out, <0: zoom in), indirectly controling the zoom animation speed.
	virtual void ZoomEvent(float amount) { }
	/// Animated zoom update (on each frame), primarily designed for mouse wheel.
	virtual void ZoomEventUpdate(float frameTime) { }

	virtual bool IsFreelooking() const { return false; }

private:
	RefCountedPtr<CameraContext> m_camera;
	const Ship *m_ship;
	vector3d m_pos;
	matrix3x3d m_orient;
};


class InternalCameraController : public CameraController {
public:
	enum Mode {
		MODE_FRONT,
		MODE_REAR,
		MODE_LEFT,
		MODE_RIGHT,
		MODE_TOP,
		MODE_BOTTOM
	};

	InternalCameraController(RefCountedPtr<CameraContext> camera, const Ship *ship);
	virtual void Reset();

	Type GetType() const { return INTERNAL; }
	const char *GetName() const { return m_name; }
	void SetMode(Mode m);
	Mode GetMode() const { return m_mode; }
	virtual void Update() override;
	void Save(Serializer::Writer &wr);
	void Load(Serializer::Reader &rd);

	virtual void RotateDown(float frameTime) override;
	virtual void RotateLeft(float frameTime) override;
	virtual void RotateRight(float frameTime) override;
	virtual void RotateUp(float frameTime) override;
	virtual void ZoomIn(float frameTime) override;
	virtual void ZoomOut(float frameTime) override;

	virtual bool IsFreelooking() const override;
	virtual const matrix3x3d& GetExtOrient() const { return m_extOrient; }

	void ResetFreelook();

private:

	void ApplySmoothFreelook(vector2d& dir, float td);

	Mode m_mode;
	const char *m_name;

	vector3d m_frontPos;  matrix3x3d m_frontOrient;
	vector3d m_rearPos;   matrix3x3d m_rearOrient;
	vector3d m_leftPos;   matrix3x3d m_leftOrient;
	vector3d m_rightPos;  matrix3x3d m_rightOrient;
	vector3d m_topPos;    matrix3x3d m_topOrient;
	vector3d m_bottomPos; matrix3x3d m_bottomOrient;

	double m_dist, m_distTo;
	double m_rotX; //vertical rot
	double m_rotY; //horizontal rot
	matrix3x3d m_extOrient;

	std::unique_ptr<Easing::NumericTweener<double> > m_tweenRotX;
	std::unique_ptr<Easing::NumericTweener<double> > m_tweenRotY;
	std::unique_ptr<Easing::NumericTweener<double> > m_tweenSpeed;

	// Freelook
	vector2d m_flDir; // Freelook direction
	vector2d m_flLastDir; // Freelook last direction for smooth stop
	float m_flResetTimer; // Freelook reset timer
};

// Zoomable, rotatable orbit camera, always looks at the ship
class ExternalCameraController : public CameraController {
public:
	ExternalCameraController(RefCountedPtr<CameraContext> camera, const Ship *ship);

	Type GetType() const { return EXTERNAL; }
	const char *GetName() const { return Lang::EXTERNAL_VIEW; }

	void RotateDown(float frameTime);
	void RotateLeft(float frameTime);
	void RotateRight(float frameTime);
	void RotateUp(float frameTime);
	void ZoomIn(float frameTime);
	void ZoomOut(float frameTime);
	void ZoomEvent(float amount);
	void ZoomEventUpdate(float frameTime);
	void Reset();
	bool IsExternal() const { return true; }
	void SetRotationAngles(double x, double y) {
		m_rotX = x;
		m_rotY = y;
	}

	void Save(Serializer::Writer &wr);
	void Load(Serializer::Reader &rd);

	void Update();

private:
	double m_dist, m_distTo;
	double m_rotX; //vertical rot
	double m_rotY; //horizontal rot
	matrix3x3d m_extOrient;
};


// Much like external camera, but does not turn when the ship turns
class SiderealCameraController : public CameraController {
public:
	SiderealCameraController(RefCountedPtr<CameraContext> camera, const Ship *ship);

	Type GetType() const { return SIDEREAL; }
	const char *GetName() const { return Lang::SIDEREAL_VIEW; }

	void RollLeft(float frameTime);
	void RollRight(float frameTime);
	void RotateDown(float frameTime);
	void RotateLeft(float frameTime);
	void RotateRight(float frameTime);
	void RotateUp(float frameTime);
	void ZoomIn(float frameTime);
	void ZoomOut(float frameTime);
	void ZoomEvent(float amount);
	void ZoomEventUpdate(float frameTime);
	void Reset();
	bool IsExternal() const { return true; }

	void Save(Serializer::Writer &wr);
	void Load(Serializer::Reader &rd);

	void Update();

private:
	double m_dist, m_distTo;
	matrix3x3d m_sidOrient;
};

#endif
