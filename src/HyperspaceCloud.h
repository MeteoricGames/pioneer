// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _HYPERSPACECLOUD_H
#define _HYPERSPACECLOUD_H

#include "Body.h"

class Frame;
class Ship;
class Space;
namespace Graphics {
	class Material;
	class Renderer;
	class VertexArray;
	class RenderState;
}

static const double HYPERCLOUD_PHASE_RANGE = 5000.0;
static const double HYPERCLOUD_PHASE_RANGE_SQR = HYPERCLOUD_PHASE_RANGE * HYPERCLOUD_PHASE_RANGE;
static const double HYPERCLOUD_PERMA_MULTIPLIER = 2.0; // 200% range -> 50% of jump cost

/** XXX TODO XXX Not applied to yet... */
#define HYPERCLOUD_DURATION (60.0*60.0*24.0*2.0)

enum HyperspaceCloudType {
    EHCT_ARRIVAL = 0,
    EHCT_DEPARTURE,
    EHCT_PERMANENT,
};

class HyperspaceCloud: public Body {
    friend class Space;
public:
	OBJDEF(HyperspaceCloud, Body, HYPERSPACECLOUD);
	virtual ~HyperspaceCloud();
	virtual void SetVelocity(const vector3d &v) { m_vel = v; }
	virtual vector3d GetVelocity() const { return m_vel; }
	virtual void Render(Graphics::Renderer *r, const Camera *camera, const vector3d &viewCoords, 
        const matrix4x4d &viewTransform);
	virtual void PostLoadFixup(Space *space);
	virtual void TimeStepUpdate(const float timeStep);
	bool ReceiveShip(Ship* ship, double due);
	Ship *GetShip() { return m_ship; }
	bool HasShip() const { return m_ship != nullptr; }
	Ship *EvictShip();
	double GetDueDate() const { return m_due; }
    void SetCloudType(HyperspaceCloudType cloud_type);
    HyperspaceCloudType GetCloudType() const { return m_cloudType; }
    const char* GetCloudTypeString() const;
    const char* GetCloudDirString() const;
    bool SupportsArrival() const { return m_cloudType != EHCT_DEPARTURE; }
    bool SupportsDeparture() const { return m_cloudType != EHCT_ARRIVAL; }
	bool IsPermanent() const { return m_cloudType == EHCT_PERMANENT; }
	virtual void UpdateInterpTransform(double alpha);
    virtual const SystemBody *GetSystemBody() const override { return m_sbody; }

protected:
    HyperspaceCloud(Ship *, double dateDue, HyperspaceCloudType cloud_type);
    HyperspaceCloud();
    HyperspaceCloud(SystemBody* system_body, HyperspaceCloudType cloud_type);

	virtual void Save(Serializer::Writer &wr, Space *space);
	virtual void Load(Serializer::Reader &rd, Space *space);

private:
	void InitGraphics();

	Ship *m_ship;
    SystemBody *m_sbody;
	vector3d m_vel;
	double m_birthdate;
	double m_due;
    HyperspaceCloudType m_cloudType;

	struct Graphic {
		std::unique_ptr<Graphics::VertexArray> vertices;
		std::unique_ptr<Graphics::Material> material;
		Graphics::RenderState *renderState;
	};
	Graphic m_graphic;
};

#endif /* _HYPERSPACECLOUD_H */
