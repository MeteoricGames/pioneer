// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "StarSystem.h"
#include "Sector.h"
#include "SectorCache.h"
#include "Factions.h"

#include "Serializer.h"
#include "Pi.h"
#include "LuaNameGen.h"
#include "enum_table.h"
#include <map>
#include <string>
#include <algorithm>
#include "utils.h"
#include "Orbit.h"
#include "Lang.h"
#include "StringF.h"
#include <SDL_stdinc.h>

using namespace StarSystemConstants;

StarSystemCache::SystemCacheMap StarSystemCache::s_cachedSystems;

// indexed by enum type turd
const Uint8 StarSystem::starColors[][3] = {
    {0, 0, 0}, // gravpoint
    {128, 0, 0}, // brown dwarf
    {102, 102, 204}, // white dwarf
    {255, 51, 0}, // M
    {255, 153, 26}, // K
    {255, 255, 102}, // G
    {255, 255, 204}, // F
    {255, 255, 255}, // A
    {178, 178, 255}, // B
    {255, 178, 255}, // O
    {255, 51, 0}, // M Giant
    {255, 153, 26}, // K Giant
    {255, 255, 102}, // G Giant
    {255, 255, 204}, // F Giant
    {255, 255, 255}, // A Giant
    {178, 178, 255}, // B Giant
    {255, 178, 255}, // O Giant
    {255, 51, 0}, // M Super Giant
    {255, 153, 26}, // K Super Giant
    {255, 255, 102}, // G Super Giant
    {255, 255, 204}, // F Super Giant
    {255, 255, 255}, // A Super Giant
    {178, 178, 255}, // B Super Giant
    {255, 178, 255}, // O Super Giant
    {255, 51, 0}, // M Hyper Giant
    {255, 153, 26}, // K Hyper Giant
    {255, 255, 102}, // G Hyper Giant
    {255, 255, 204}, // F Hyper Giant
    {255, 255, 255}, // A Hyper Giant
    {178, 178, 255}, // B Hyper Giant
    {255, 178, 255}, // O Hyper Giant
    {255, 51, 0}, // Red/M Wolf Rayet Star
    {178, 178, 255}, // Blue/B Wolf Rayet Star
    {255, 178, 255}, // Purple-Blue/O Wolf Rayet Star
    {76, 178, 76}, // Stellar Blackhole
    {51, 230, 51}, // Intermediate mass Black-hole
    {0, 255, 0}, // Super massive black hole
};

// indexed by enum type turd
const Uint8 StarSystem::starRealColors[][3] = {
    {0, 0, 0}, // gravpoint
    {128, 0, 0}, // brown dwarf
    {255, 255, 255}, // white dwarf
    {255, 128, 51}, // M
    {255, 255, 102}, // K
    {255, 255, 242}, // G
    {255, 255, 255}, // F
    {255, 255, 255}, // A
    {204, 204, 255}, // B
    {255, 204, 255},  // O
    {255, 128, 51}, // M Giant
    {255, 255, 102}, // K Giant
    {255, 255, 242}, // G Giant
    {255, 255, 255}, // F Giant
    {255, 255, 255}, // A Giant
    {204, 204, 255}, // B Giant
    {255, 204, 255},  // O Giant
    {255, 128, 51}, // M Super Giant
    {255, 255, 102}, // K Super Giant
    {255, 255, 242}, // G Super Giant
    {255, 255, 255}, // F Super Giant
    {255, 255, 255}, // A Super Giant
    {204, 204, 255}, // B Super Giant
    {255, 204, 255},  // O Super Giant
    {255, 128, 51}, // M Hyper Giant
    {255, 255, 102}, // K Hyper Giant
    {255, 255, 242}, // G Hyper Giant
    {255, 255, 255}, // F Hyper Giant
    {255, 255, 255}, // A Hyper Giant
    {204, 204, 255}, // B Hyper Giant
    {255, 204, 255},  // O Hyper Giant
    {255, 153, 153}, // M WF
    {204, 204, 255}, // B WF
    {255, 204, 255},  // O WF
    {255, 255, 255},  // small Black hole
    {16, 0, 20}, // med BH
    {10, 0, 16}, // massive BH
};

const double StarSystem::starLuminosities[] = {
    0,
    0.0003, // brown dwarf
    0.1, // white dwarf
    0.08, // M0
    0.38, // K0
    1.2, // G0
    5.1, // F0
    24.0, // A0
    100.0, // B0
    200.0, // O5
    1000.0, // M0 Giant
    2000.0, // K0 Giant
    4000.0, // G0 Giant
    6000.0, // F0 Giant
    8000.0, // A0 Giant
    9000.0, // B0 Giant
    12000.0, // O5 Giant
    12000.0, // M0 Super Giant
    14000.0, // K0 Super Giant
    18000.0, // G0 Super Giant
    24000.0, // F0 Super Giant
    30000.0, // A0 Super Giant
    50000.0, // B0 Super Giant
    100000.0, // O5 Super Giant
    125000.0, // M0 Hyper Giant
    150000.0, // K0 Hyper Giant
    175000.0, // G0 Hyper Giant
    200000.0, // F0 Hyper Giant
    200000.0, // A0 Hyper Giant
    200000.0, // B0 Hyper Giant
    200000.0, // O5 Hyper Giant
    50000.0, // M WF
    100000.0, // B WF
    200000.0, // O WF
    0.0003, // Stellar Black hole
    0.00003, // IM Black hole
    0.000003, // Supermassive Black hole
};

const float StarSystem::starScale[] = {  // Used in sector view
    0,
    0.6f, // brown dwarf
    0.5f, // white dwarf
    0.7f, // M
    0.8f, // K
    0.8f, // G
    0.9f, // F
    1.0f, // A
    1.1f, // B
    1.1f, // O
    1.3f, // M Giant
    1.2f, // K G
    1.2f, // G G
    1.2f, // F G
    1.1f, // A G
    1.1f, // B G
    1.2f, // O G
    1.8f, // M Super Giant
    1.6f, // K SG
    1.5f, // G SG
    1.5f, // F SG
    1.4f, // A SG
    1.3f, // B SG
    1.3f, // O SG
    2.5f, // M Hyper Giant
    2.2f, // K HG
    2.2f, // G HG
    2.1f, // F HG
    2.1f, // A HG
    2.0f, // B HG
    1.9f, // O HG
    1.1f, // M WF
    1.3f, // B WF
    1.6f, // O WF
    1.0f, // Black hole
    2.5f, // Intermediate-mass blackhole
    4.0f  // Supermassive blackhole
};

const fixed StarSystem::starMetallicities[] = {
    fixed(1, 1), // GRAVPOINT - for planets that orbit them
    fixed(9, 10), // brown dwarf
    fixed(5, 10), // white dwarf
    fixed(7, 10), // M0
    fixed(6, 10), // K0
    fixed(5, 10), // G0
    fixed(4, 10), // F0
    fixed(3, 10), // A0
    fixed(2, 10), // B0
    fixed(1, 10), // O5
    fixed(8, 10), // M0 Giant
    fixed(65, 100), // K0 Giant
    fixed(55, 100), // G0 Giant
    fixed(4, 10), // F0 Giant
    fixed(3, 10), // A0 Giant
    fixed(2, 10), // B0 Giant
    fixed(1, 10), // O5 Giant
    fixed(9, 10), // M0 Super Giant
    fixed(7, 10), // K0 Super Giant
    fixed(6, 10), // G0 Super Giant
    fixed(4, 10), // F0 Super Giant
    fixed(3, 10), // A0 Super Giant
    fixed(2, 10), // B0 Super Giant
    fixed(1, 10), // O5 Super Giant
    fixed(1, 1), // M0 Hyper Giant
    fixed(7, 10), // K0 Hyper Giant
    fixed(6, 10), // G0 Hyper Giant
    fixed(4, 10), // F0 Hyper Giant
    fixed(3, 10), // A0 Hyper Giant
    fixed(2, 10), // B0 Hyper Giant
    fixed(1, 10), // O5 Hyper Giant
    fixed(1, 1), // M WF
    fixed(8, 10), // B WF
    fixed(6, 10), // O WF
    fixed(1, 1), //  S BH	Blackholes, give them high metallicity,
    fixed(1, 1), // IM BH	so any rocks that happen to be there
    fixed(1, 1)  // SM BH	may be mining hotspots. FUN :)
};

/*
* As my excellent comrades have pointed out, choices that depend on floating
* point crap will result in different universes on different platforms.
*
* We must be sneaky and avoid floating point in these places.
*/
StarSystem::StarSystem(const SystemPath &path) : m_path(path), m_nextBodyIndex(0)
{
    PROFILE_SCOPED()
        assert(path.IsSystemPath());
    memset(m_tradeLevel, 0, sizeof(m_tradeLevel));

    RefCountedPtr<const Sector> s = Sector::cache.GetCached(m_path);
    assert(m_path.systemIndex >= 0 && m_path.systemIndex < s->m_systems.size());

    m_seed = s->m_systems[m_path.systemIndex].seed;
    m_name = s->m_systems[m_path.systemIndex].name;
    m_faction = Faction::GetNearestFaction(s, m_path.systemIndex);

    Uint32 _init[6] = {m_path.systemIndex, Uint32(m_path.sectorX), Uint32(m_path.sectorY), Uint32(m_path.sectorZ), UNIVERSE_SEED, Uint32(m_seed)};
    Random rand(_init, 6);

    m_unexplored = !s->m_systems[m_path.systemIndex].explored;

    m_isCustom = m_hasCustomBodies = false;
    if (s->m_systems[m_path.systemIndex].customSys) {
        m_isCustom = true;
        const CustomSystem *custom = s->m_systems[m_path.systemIndex].customSys;
        m_numStars = custom->numStars;
        if (custom->shortDesc.length() > 0) m_shortDesc = custom->shortDesc;
        if (custom->longDesc.length() > 0) m_longDesc = custom->longDesc;
        if (!custom->IsRandom()) {
            m_hasCustomBodies = true;
            GenerateFromCustom(s->m_systems[m_path.systemIndex].customSys, rand);
            return;
        }
    }

    SystemBody *star[4];
    SystemBody *centGrav1(0), *centGrav2(0);

    const int numStars = s->m_systems[m_path.systemIndex].numStars;
    assert((numStars >= 1) && (numStars <= 4));
    if (numStars == 1) {
        SystemBody::BodyType type = s->m_systems[m_path.systemIndex].starType[0];
        star[0] = NewBody();
        star[0]->m_parent = 0;
        star[0]->m_name = s->m_systems[m_path.systemIndex].name;
        star[0]->m_orbMin = fixed(0);
        star[0]->m_orbMax = fixed(0);

        MakeStarOfType(star[0], type, rand);
        m_rootBody.Reset(star[0]);
        m_numStars = 1;
    } else {
        centGrav1 = NewBody();
        centGrav1->m_type = SystemBody::TYPE_GRAVPOINT;
        centGrav1->m_parent = 0;
        centGrav1->m_name = s->m_systems[m_path.systemIndex].name + " A,B";
        m_rootBody.Reset(centGrav1);

        SystemBody::BodyType type = s->m_systems[m_path.systemIndex].starType[0];
        star[0] = NewBody();
        star[0]->m_name = s->m_systems[m_path.systemIndex].name + " A";
        star[0]->m_parent = centGrav1;
        MakeStarOfType(star[0], type, rand);

        star[1] = NewBody();
        star[1]->m_name = s->m_systems[m_path.systemIndex].name + " B";
        star[1]->m_parent = centGrav1;
        MakeStarOfTypeLighterThan(star[1], s->m_systems[m_path.systemIndex].starType[1],
            star[0]->GetMassAsFixed(), rand);

        centGrav1->m_mass = star[0]->GetMassAsFixed() + star[1]->GetMassAsFixed();
        centGrav1->m_children.push_back(star[0]);
        centGrav1->m_children.push_back(star[1]);
        // Separate stars by 0.2 radii for each, so that their planets don't bump into the other star
        const fixed minDist1 = (fixed(12, 10) * star[0]->GetRadiusAsFixed() + fixed(12, 10) * star[1]->GetRadiusAsFixed()) * AU_SOL_RADIUS;
    try_that_again_guvnah:
        MakeBinaryPair(star[0], star[1], minDist1, rand);

        m_numStars = 2;

        if (numStars > 2) {
            if (star[0]->m_orbMax > fixed(100, 1)) {
                // reduce to < 100 AU...
                goto try_that_again_guvnah;
            }
            // 3rd and maybe 4th star
            if (numStars == 3) {
                star[2] = NewBody();
                star[2]->m_name = s->m_systems[m_path.systemIndex].name + " C";
                star[2]->m_orbMin = 0;
                star[2]->m_orbMax = 0;
                MakeStarOfTypeLighterThan(star[2], s->m_systems[m_path.systemIndex].starType[2],
                    star[0]->GetMassAsFixed(), rand);
                centGrav2 = star[2];
                m_numStars = 3;
            } else {
                centGrav2 = NewBody();
                centGrav2->m_type = SystemBody::TYPE_GRAVPOINT;
                centGrav2->m_name = s->m_systems[m_path.systemIndex].name + " C,D";
                centGrav2->m_orbMax = 0;

                star[2] = NewBody();
                star[2]->m_name = s->m_systems[m_path.systemIndex].name + " C";
                star[2]->m_parent = centGrav2;
                MakeStarOfTypeLighterThan(star[2], s->m_systems[m_path.systemIndex].starType[2],
                    star[0]->GetMassAsFixed(), rand);

                star[3] = NewBody();
                star[3]->m_name = s->m_systems[m_path.systemIndex].name + " D";
                star[3]->m_parent = centGrav2;
                MakeStarOfTypeLighterThan(star[3], s->m_systems[m_path.systemIndex].starType[3],
                    star[2]->GetMassAsFixed(), rand);

                // Separate stars by 0.2 radii for each, so that their planets don't bump into the other star
                const fixed minDist2 = (fixed(12, 10) * star[2]->GetRadiusAsFixed() + fixed(12, 10) * star[3]->GetRadiusAsFixed()) * AU_SOL_RADIUS;
                MakeBinaryPair(star[2], star[3], minDist2, rand);
                centGrav2->m_mass = star[2]->GetMassAsFixed() + star[3]->GetMassAsFixed();
                centGrav2->m_children.push_back(star[2]);
                centGrav2->m_children.push_back(star[3]);
                m_numStars = 4;
            }
            SystemBody *superCentGrav = NewBody();
            superCentGrav->m_type = SystemBody::TYPE_GRAVPOINT;
            superCentGrav->m_parent = 0;
            superCentGrav->m_name = s->m_systems[m_path.systemIndex].name;
            centGrav1->m_parent = superCentGrav;
            centGrav2->m_parent = superCentGrav;
            m_rootBody.Reset(superCentGrav);
            const fixed minDistSuper = star[0]->m_orbMax + star[2]->m_orbMax;
            MakeBinaryPair(centGrav1, centGrav2, 4 * minDistSuper, rand);
            superCentGrav->m_children.push_back(centGrav1);
            superCentGrav->m_children.push_back(centGrav2);

        }
    }

    // used in MakeShortDescription
    // XXX except this does not reflect the actual mining happening in this system
    m_metallicity = starMetallicities[m_rootBody->GetType()];

    m_stars.resize(m_numStars);
    for (int i = 0; i<m_numStars; i++) {
        m_stars[i] = star[i];
        MakePlanetsAround(star[i], rand);
    }

    if (m_numStars > 1) MakePlanetsAround(centGrav1, rand);
    if (m_numStars == 4) MakePlanetsAround(centGrav2, rand);

    Populate(true);
    GeneratePermaHyperclouds();

    // an example export of generated system, can be removed during the merge
    //char filename[500];
    //snprintf(filename, 500, "tmp-sys/%s.lua", GetName().c_str());
    //ExportToLua(filename);

#ifdef DEBUG_DUMP
    Dump();
#endif /* DEBUG_DUMP */
}

#ifdef DEBUG_DUMP
struct thing_t {
    SystemBody* obj;
    vector3d pos;
    vector3d vel;
};
void StarSystem::Dump()
{
    std::vector<SystemBody*> obj_stack;
    std::vector<vector3d> pos_stack;
    std::vector<thing_t> output;

    SystemBody *obj = m_rootBody;
    vector3d pos = vector3d(0.0);

    while (obj) {
        vector3d p2 = pos;
        if (obj->m_parent) {
            p2 = pos + obj->m_orbit.OrbitalPosAtTime(1.0);
            pos = pos + obj->m_orbit.OrbitalPosAtTime(0.0);
        }

        if ((obj->GetType() != SystemBody::TYPE_GRAVPOINT) &&
            (obj->GetSuperType() != SystemBody::SUPERTYPE_STARPORT) &&
			(obj->GetSuperType() != SystemBody::SUPERTYPE_HYPERSPACE_CLOUD) {
            struct thing_t t;
            t.obj = obj;
            t.pos = pos;
            t.vel = (p2 - pos);
            output.push_back(t);
        }
        for (std::vector<SystemBody*>::iterator i = obj->m_children.begin();
            i != obj->m_children.end(); ++i) {
            obj_stack.push_back(*i);
            pos_stack.push_back(pos);
        }
        if (obj_stack.size() == 0) break;
        pos = pos_stack.back();
        obj = obj_stack.back();
        pos_stack.pop_back();
        obj_stack.pop_back();
    }

    FILE *f = fopen("starsystem.dump", "w");
    fprintf(f, "%d bodies\n", output.size());
    fprintf(f, "0 steps\n");
    for (std::vector<thing_t>::iterator i = output.begin();
        i != output.end(); ++i) {
        fprintf(f, "B:%lf,%lf:%lf,%lf,%lf,%lf:%lf:%d:%lf,%lf,%lf\n",
            (*i).pos.x, (*i).pos.y, (*i).pos.z,
            (*i).vel.x, (*i).vel.y, (*i).vel.z,
            (*i).obj->GetMass(), 0,
            1.0, 1.0, 1.0);
    }
    fclose(f);
    Output("Junk dumped to starsystem.dump\n");
}
#endif /* DEBUG_DUMP */

SystemBody *StarSystem::GetBodyByPath(const SystemPath &path) const
{
	PROFILE_SCOPED()
	assert(m_path.IsSameSystem(path));
	assert(path.IsBodyPath());
	assert(path.bodyIndex < m_bodies.size());

	return m_bodies.at(path.bodyIndex).Get();
}

SystemPath StarSystem::GetPathOf(const SystemBody *sbody) const
{
	return sbody->GetPath();
}

void StarSystem::CustomGetKidsOf(SystemBody *parent, const std::vector<CustomSystemBody*> &children, int *outHumanInfestedness, Random &rand)
{
	PROFILE_SCOPED()
	// replaces gravpoint mass by sum of masses of its children
	// the code goes here to cover also planetary gravpoints (gravpoints that are not rootBody)
	if (parent->GetType() == SystemBody::TYPE_GRAVPOINT) {
		fixed mass(0);

		for (std::vector<CustomSystemBody*>::const_iterator i = children.begin(); i != children.end(); ++i) {
			const CustomSystemBody *csbody = *i;

			if (csbody->type >= SystemBody::TYPE_STAR_MIN && csbody->type <= SystemBody::TYPE_STAR_MAX)
				mass += csbody->mass;
			else
				mass += csbody->mass / SUN_MASS_TO_EARTH_MASS;
		}

		parent->m_mass = mass;
	}

	for (std::vector<CustomSystemBody*>::const_iterator i = children.begin(); i != children.end(); ++i) {
		const CustomSystemBody *csbody = *i;

		SystemBody *kid = NewBody();
		kid->m_type = csbody->type;
		kid->m_parent = parent;
		kid->m_seed = csbody->want_rand_seed ? rand.Int32() : csbody->seed;
		kid->m_radius = csbody->radius;
		kid->m_aspectRatio = csbody->aspectRatio;
		kid->m_averageTemp = csbody->averageTemp;
		kid->m_name = csbody->name;
		kid->m_isCustomBody = true;

		kid->m_mass = csbody->mass;
		if (kid->GetType() == SystemBody::TYPE_PLANET_ASTEROID) kid->m_mass /= 100000;

		kid->m_metallicity    = csbody->metallicity;
		//multiple of Earth's surface density
		kid->m_volatileGas    = csbody->volatileGas*fixed(1225,1000);
		kid->m_volatileLiquid = csbody->volatileLiquid;
		kid->m_volatileIces   = csbody->volatileIces;
		kid->m_volcanicity    = csbody->volcanicity;
		kid->m_atmosOxidizing = csbody->atmosOxidizing;
		kid->m_life           = csbody->life;

		kid->m_rotationPeriod = csbody->rotationPeriod;
		kid->m_rotationalPhaseAtStart = csbody->rotationalPhaseAtStart;
		kid->m_eccentricity = csbody->eccentricity;
		kid->m_orbitalOffset = csbody->orbitalOffset;
		kid->m_orbitalPhaseAtStart = csbody->orbitalPhaseAtStart;
		kid->m_axialTilt = csbody->axialTilt;
		kid->m_inclination = fixed(csbody->latitude*10000,10000);
		if(kid->GetType() == SystemBody::TYPE_STARPORT_SURFACE)
			kid->m_orbitalOffset = fixed(csbody->longitude*10000,10000);
		kid->m_semiMajorAxis = csbody->semiMajorAxis;

		if (csbody->heightMapFilename.length() > 0) {
			kid->m_heightMapFilename = csbody->heightMapFilename;
			kid->m_heightMapFractal = csbody->heightMapFractal;
		}

		if(parent->GetType() == SystemBody::TYPE_GRAVPOINT) // generalize Kepler's law to multiple stars
			kid->m_orbit.SetShapeAroundBarycentre(csbody->semiMajorAxis.ToDouble() * AU, parent->GetMass(), kid->GetMass(), csbody->eccentricity.ToDouble());
		else
			kid->m_orbit.SetShapeAroundPrimary(csbody->semiMajorAxis.ToDouble() * AU, parent->GetMass(), csbody->eccentricity.ToDouble());

		kid->m_orbit.SetPhase(csbody->orbitalPhaseAtStart.ToDouble());

		if (kid->GetType() == SystemBody::TYPE_STARPORT_SURFACE) {
			kid->m_orbit.SetPlane(matrix3x3d::RotateY(csbody->longitude) * matrix3x3d::RotateX(-0.5*M_PI + csbody->latitude));
		} else {
			if (kid->m_orbit.GetSemiMajorAxis() < 1.2 * parent->GetRadius()) {
				Error("%s's orbit is too close to its parent", csbody->name.c_str());
			}
			double offset = csbody->want_rand_offset ? rand.Double(2*M_PI) : (csbody->orbitalOffset.ToDouble());
			kid->m_orbit.SetPlane(matrix3x3d::RotateY(offset) * matrix3x3d::RotateX(-0.5*M_PI + csbody->latitude));
		}
		if (kid->GetSuperType() == SystemBody::SUPERTYPE_STARPORT) {
			(*outHumanInfestedness)++;
            m_spaceStations.push_back(kid);
		}
		parent->m_children.push_back(kid);

		// perihelion and aphelion (in AUs)
		kid->m_orbMin = csbody->semiMajorAxis - csbody->eccentricity*csbody->semiMajorAxis;
		kid->m_orbMax = 2*csbody->semiMajorAxis - kid->m_orbMin;

		kid->PickAtmosphere();

		// pick or specify rings
		switch (csbody->ringStatus) {
			case CustomSystemBody::WANT_NO_RINGS:
				kid->m_rings.minRadius = fixed(0);
				kid->m_rings.maxRadius = fixed(0);
				break;
			case CustomSystemBody::WANT_RINGS:
				kid->PickRings(true);
				break;
			case CustomSystemBody::WANT_RANDOM_RINGS:
				kid->PickRings(false);
				break;
			case CustomSystemBody::WANT_CUSTOM_RINGS:
				kid->m_rings.minRadius = csbody->ringInnerRadius;
				kid->m_rings.maxRadius = csbody->ringOuterRadius;
				kid->m_rings.baseColor = csbody->ringColor;
				break;
		}

		CustomGetKidsOf(kid, csbody->children, outHumanInfestedness, rand);
	}

}

void StarSystem::GenerateFromCustom(const CustomSystem *customSys, Random &rand)
{
	PROFILE_SCOPED()
	const CustomSystemBody *csbody = customSys->sBody;

	m_rootBody.Reset(NewBody());
	m_rootBody->m_type = csbody->type;
	m_rootBody->m_parent = 0;
	m_rootBody->m_seed = csbody->want_rand_seed ? rand.Int32() : csbody->seed;
	m_rootBody->m_seed = rand.Int32();
	m_rootBody->m_radius = csbody->radius;
	m_rootBody->m_aspectRatio = csbody->aspectRatio;
	m_rootBody->m_mass = csbody->mass;
	m_rootBody->m_averageTemp = csbody->averageTemp;
	m_rootBody->m_name = csbody->name;
	m_rootBody->m_isCustomBody = true;

	m_rootBody->m_rotationalPhaseAtStart = csbody->rotationalPhaseAtStart;
	m_rootBody->m_orbitalPhaseAtStart = csbody->orbitalPhaseAtStart;

	int humanInfestedness = 0;
	CustomGetKidsOf(m_rootBody.Get(), csbody->children, &humanInfestedness, rand);
	int i = 0;
	m_stars.resize(m_numStars);
	//for (RefCountedPtr<SystemBody> b : m_bodies) {
	for(auto iter = m_bodies.begin(); iter != m_bodies.end(); ++iter) {
		if (iter->second->GetSuperType() == SystemBody::SUPERTYPE_STAR)
			m_stars[i++] = iter->second.Get();
	}
	assert(i == m_numStars);
	Populate(false);
    GeneratePermaHyperclouds();

	// an example re-export of custom system, can be removed during the merge
	//char filename[500];
	//snprintf(filename, 500, "tmp-sys/%s.lua", GetName().c_str());
	//ExportToLua(filename);

}

void StarSystem::MakeStarOfType(SystemBody *sbody, SystemBody::BodyType type, Random &rand)
{
	PROFILE_SCOPED()
	sbody->m_type = type;
	sbody->m_seed = rand.Int32();
	sbody->m_radius = fixed(rand.Int32(starTypeInfo[type].radius[0],
				starTypeInfo[type].radius[1]), 100);

	// Assign aspect ratios caused by equatorial bulges due to rotation. See terrain code for details.
	// XXX to do: determine aspect ratio distributions for dimmer stars. Make aspect ratios consistent with rotation speeds/stability restrictions.
	switch (type) {
		// Assign aspect ratios (roughly) between 1.0 to 1.8 with a bias towards 1 for bright stars F, A, B ,O

		// "A large fraction of hot stars are rapid rotators with surface rotational velocities
		// of more than 100 km/s (6, 7). ." Imaging the Surface of Altair, John D. Monnier, et. al. 2007
		// A reasonable amount of lot of stars will be assigned high aspect ratios.

		// Bright stars whose equatorial to polar radius ratio (the aspect ratio) is known
		// seem to tend to have values between 1.0 and around 1.5 (brief survey).
		// The limiting factor preventing much higher values seems to be stability as they
		// are rotating 80-95% of their breakup velocity.
		case SystemBody::TYPE_STAR_F:
		case SystemBody::TYPE_STAR_F_GIANT:
		case SystemBody::TYPE_STAR_F_HYPER_GIANT:
		case SystemBody::TYPE_STAR_F_SUPER_GIANT:
		case SystemBody::TYPE_STAR_A:
		case SystemBody::TYPE_STAR_A_GIANT:
		case SystemBody::TYPE_STAR_A_HYPER_GIANT:
		case SystemBody::TYPE_STAR_A_SUPER_GIANT:
		case SystemBody::TYPE_STAR_B:
		case SystemBody::TYPE_STAR_B_GIANT:
		case SystemBody::TYPE_STAR_B_SUPER_GIANT:
		case SystemBody::TYPE_STAR_B_WF:
		case SystemBody::TYPE_STAR_O:
		case SystemBody::TYPE_STAR_O_GIANT:
		case SystemBody::TYPE_STAR_O_HYPER_GIANT:
		case SystemBody::TYPE_STAR_O_SUPER_GIANT:
		case SystemBody::TYPE_STAR_O_WF: {
			fixed rnd = rand.Fixed();
			sbody->m_aspectRatio = fixed(1, 1)+fixed(8, 10)*rnd*rnd;
			break;
		}
		// aspect ratio is initialised to 1.0 for other stars currently
		default:
			break;
	}
	sbody->m_mass = fixed(rand.Int32(starTypeInfo[type].mass[0],
				starTypeInfo[type].mass[1]), 100);
	sbody->m_averageTemp = rand.Int32(starTypeInfo[type].tempMin,
				starTypeInfo[type].tempMax);
}

void StarSystem::MakeRandomStar(SystemBody *sbody, Random &rand)
{
	PROFILE_SCOPED()
	SystemBody::BodyType type = SystemBody::BodyType(rand.Int32(SystemBody::TYPE_STAR_MIN, SystemBody::TYPE_STAR_MAX));
	MakeStarOfType(sbody, type, rand);
}

void StarSystem::MakeStarOfTypeLighterThan(SystemBody *sbody, SystemBody::BodyType type, fixed maxMass, Random &rand)
{
	PROFILE_SCOPED()
	int tries = 16;
	do {
		MakeStarOfType(sbody, type, rand);
	} while ((sbody->GetMassAsFixed() > maxMass) && (--tries));
}

void StarSystem::MakeBinaryPair(SystemBody *a, SystemBody *b, fixed minDist, Random &rand)
{
	PROFILE_SCOPED()
	fixed m = a->GetMassAsFixed() + b->GetMassAsFixed();
	fixed a0 = b->GetMassAsFixed() / m;
	fixed a1 = a->GetMassAsFixed() / m;
	a->m_eccentricity = rand.NFixed(3);
	int mul = 1;

	do {
		switch (rand.Int32(3)) {
			case 2: a->m_semiMajorAxis = fixed(rand.Int32(100,10000), 100); break;
			case 1: a->m_semiMajorAxis = fixed(rand.Int32(10,1000), 100); break;
			default:
			case 0: a->m_semiMajorAxis = fixed(rand.Int32(1,100), 100); break;
		}
		a->m_semiMajorAxis *= mul;
		mul *= 2;
	} while (a->m_semiMajorAxis - a->m_eccentricity*a->m_semiMajorAxis < minDist);

	const double total_mass = a->GetMass() + b->GetMass();
	const double e = a->m_eccentricity.ToDouble();

	a->m_orbit.SetShapeAroundBarycentre(AU * (a->m_semiMajorAxis * a0).ToDouble(), total_mass, a->GetMass(), e);
	b->m_orbit.SetShapeAroundBarycentre(AU * (a->m_semiMajorAxis * a1).ToDouble(), total_mass, b->GetMass(), e);

	const float rotX = -0.5f*float(M_PI);//(float)(rand.Double()*M_PI/2.0);
	const float rotY = static_cast<float>(rand.Double(M_PI));
	a->m_orbit.SetPlane(matrix3x3d::RotateY(rotY) * matrix3x3d::RotateX(rotX));
	b->m_orbit.SetPlane(matrix3x3d::RotateY(rotY-M_PI) * matrix3x3d::RotateX(rotX));

	// store orbit parameters for later use to be accesible in other way than by rotMatrix
	b->m_orbitalPhaseAtStart = b->m_orbitalPhaseAtStart + FIXED_PI;
	b->m_orbitalPhaseAtStart = b->m_orbitalPhaseAtStart > 2*FIXED_PI ? b->m_orbitalPhaseAtStart - 2*FIXED_PI : b->m_orbitalPhaseAtStart;
	a->m_orbitalPhaseAtStart = a->m_orbitalPhaseAtStart > 2*FIXED_PI ? a->m_orbitalPhaseAtStart - 2*FIXED_PI : a->m_orbitalPhaseAtStart;
	a->m_orbitalPhaseAtStart = a->m_orbitalPhaseAtStart < 0 ? a->m_orbitalPhaseAtStart + 2*FIXED_PI : a->m_orbitalPhaseAtStart;
	b->m_orbitalOffset = fixed(int(round(rotY*10000)),10000);
	a->m_orbitalOffset = fixed(int(round(rotY*10000)),10000);

	fixed orbMin = a->m_semiMajorAxis - a->m_eccentricity*a->m_semiMajorAxis;
	fixed orbMax = 2*a->m_semiMajorAxis - orbMin;
	a->m_orbMin = orbMin;
	b->m_orbMin = orbMin;
	a->m_orbMax = orbMax;
	b->m_orbMax = orbMax;
}

static fixed mass_from_disk_area(fixed a, fixed b, fixed max)
{
	PROFILE_SCOPED()
	// so, density of the disk with distance from star goes like so: 1 - x/discMax
	//
	// ---
	//    ---
	//       --- <- zero at discMax
	//
	// Which turned into a disc becomes 2*pi*x - (2*pi*x*x)/discMax
	// Integral of which is: pi*x*x - (2/(3*discMax))*pi*x*x*x
	//
	// Because get_disc_density divides total_mass by
	// mass_from_disk_area(0, discMax, discMax) to find density, the
	// constant factors (pi) in this equation drop out.
	//
	b = (b > max ? max : b);
	assert(b>=a);
	assert(a<=max);
	assert(b<=max);
	assert(a>=0);
	fixed one_over_3max = fixed(2,1)/(3*max);
	return (b*b - one_over_3max*b*b*b) -
		(a*a - one_over_3max*a*a*a);
}

static fixed get_disc_density(SystemBody *primary, fixed discMin, fixed discMax, fixed percentOfPrimaryMass)
{
	PROFILE_SCOPED()
	discMax = std::max(discMax, discMin);
	fixed total = mass_from_disk_area(discMin, discMax, discMax);
	return primary->GetMassInEarths() * percentOfPrimaryMass / total;
}

void StarSystem::MakePlanetsAround(SystemBody *primary, Random &rand)
{
	PROFILE_SCOPED()
	fixed discMin = fixed(0);
	fixed discMax = fixed(5000,1);
	fixed discDensity;

	SystemBody::BodySuperType superType = primary->GetSuperType();

	if (superType <= SystemBody::SUPERTYPE_STAR) {
		if (primary->GetType() == SystemBody::TYPE_GRAVPOINT) {
			/* around a binary */
			discMin = primary->m_children[0]->m_orbMax * SAFE_DIST_FROM_BINARY;
		} else {
			/* correct thing is roche limit, but lets ignore that because
			 * it depends on body densities and gives some strange results */
			discMin = 4 * primary->GetRadiusAsFixed() * AU_SOL_RADIUS;
		}
		if (primary->GetType() == SystemBody::TYPE_WHITE_DWARF) {
			// white dwarfs will have started as stars < 8 solar
			// masses or so, so pick discMax according to that
			// We give it a larger discMin because it used to be a much larger star
			discMin = 1000 * primary->GetRadiusAsFixed() * AU_SOL_RADIUS;
			discMax = 100 * rand.NFixed(2);		// rand-splitting again
			discMax *= fixed::SqrtOf(fixed(1,2) + fixed(8,1)*rand.Fixed());
		} else {
			discMax = 100 * rand.NFixed(2)*fixed::SqrtOf(primary->GetMassAsFixed());
		}
		// having limited discMin by bin-separation/fake roche, and
		// discMax by some relation to star mass, we can now compute
		// disc density
		discDensity = rand.Fixed() * get_disc_density(primary, discMin, discMax, fixed(2,100));

		if ((superType == SystemBody::SUPERTYPE_STAR) && (primary->m_parent)) {
			// limit planets out to 10% distance to star's binary companion
			discMax = std::min(discMax, primary->m_orbMin * fixed(1,10));
		}

		/* in trinary and quaternary systems don't bump into other pair... */
		if (m_numStars >= 3) {
			discMax = std::min(discMax, fixed(5,100)*m_rootBody->m_children[0]->m_orbMin);
		}
	} else {
		fixed primary_rad = primary->GetRadiusAsFixed() * AU_EARTH_RADIUS;
		discMin = 4 * primary_rad;
		/* use hill radius to find max size of moon system. for stars botch it.
		   And use planets orbit around its primary as a scaler to a moon's orbit*/
		discMax = std::min(discMax, fixed(1,20)*
			primary->CalcHillRadius()*primary->m_orbMin*fixed(1,10));

		discDensity = rand.Fixed() * get_disc_density(primary, discMin, discMax, fixed(1,500));
	}

	//fixed discDensity = 20*rand.NFixed(4);

	//Output("Around %s: Range %f -> %f AU\n", primary->GetName().c_str(), discMin.ToDouble(), discMax.ToDouble());

	fixed initialJump = rand.NFixed(5);
	fixed pos = (fixed(1,1) - initialJump)*discMin + (initialJump*discMax);

	while (pos < discMax) {
		// periapsis, apoapsis = closest, farthest distance in orbit
		fixed periapsis = pos + pos*fixed(1,2)*rand.NFixed(2);/* + jump */;
		fixed ecc = rand.NFixed(3);
		fixed semiMajorAxis = periapsis / (fixed(1,1) - ecc);
		fixed apoapsis = 2*semiMajorAxis - periapsis;
		if (apoapsis > discMax) break;

		fixed mass;

		{
			const fixed a = pos;
			const fixed b = fixed(135,100)*apoapsis;
			mass = mass_from_disk_area(a, b, discMax);
			mass *= rand.Fixed() * discDensity;
		}
		if (mass < 0) {// hack around overflow
			Output("WARNING: planetary mass has overflowed! (child of %s)\n", primary->GetName().c_str());
			mass = fixed(Sint64(0x7fFFffFFffFFffFFull));
		}
		assert(mass >= 0);

		SystemBody *planet = NewBody();
		planet->m_eccentricity = ecc;
		planet->m_axialTilt = fixed(100,157)*rand.NFixed(2);
		planet->m_semiMajorAxis = semiMajorAxis;
		planet->m_type = SystemBody::TYPE_PLANET_TERRESTRIAL;
		planet->m_seed = rand.Int32();
		planet->m_parent = primary;
		planet->m_mass = mass;
		planet->m_rotationPeriod = fixed(rand.Int32(1,200), 24);

		const double e = ecc.ToDouble();

		if(primary->m_type == SystemBody::TYPE_GRAVPOINT)
			planet->m_orbit.SetShapeAroundBarycentre(semiMajorAxis.ToDouble() * AU, primary->GetMass(), planet->GetMass(), e);
		else
			planet->m_orbit.SetShapeAroundPrimary(semiMajorAxis.ToDouble() * AU, primary->GetMass(), e);

		double r1 = rand.Double(2*M_PI);		// function parameter evaluation order is implementation-dependent
		double r2 = rand.NDouble(5);			// can't put two rands in the same expression
		planet->m_orbit.SetPlane(matrix3x3d::RotateY(r1) * matrix3x3d::RotateX(-0.5*M_PI + r2*M_PI/2.0));

		planet->m_inclination = FIXED_PI;
		planet->m_inclination *= r2/2.0;
		planet->m_orbMin = periapsis;
		planet->m_orbMax = apoapsis;
		primary->m_children.push_back(planet);

		/* minimum separation between planets of 1.35 */
		pos = apoapsis * fixed(135,100);
	}

	int idx=0;
	bool make_moons = superType <= SystemBody::SUPERTYPE_STAR;

	for (std::vector<SystemBody*>::iterator i = primary->m_children.begin(); i != primary->m_children.end(); ++i) {
		// planets around a binary pair [gravpoint] -- ignore the stars...
		if ((*i)->GetSuperType() == SystemBody::SUPERTYPE_STAR) continue;
		// Turn them into something!!!!!!!
		char buf[8];
		if (superType <= SystemBody::SUPERTYPE_STAR) {
			// planet naming scheme
			snprintf(buf, sizeof(buf), " %c", 'a'+idx);
		} else {
			// moon naming scheme
			snprintf(buf, sizeof(buf), " %d", 1+idx);
		}
		(*i)->m_name = primary->GetName()+buf;
		(*i)->PickPlanetType(rand);
		if (make_moons) MakePlanetsAround(*i, rand);
		idx++;
	}
}

void StarSystem::MakeShortDescription(Random &rand)
{
	PROFILE_SCOPED()
	m_econType = 0;
	if ((m_industrial > m_metallicity) && (m_industrial > m_agricultural)) {
		m_econType = ECON_INDUSTRY;
	} else if (m_metallicity > m_agricultural) {
		m_econType = ECON_MINING;
	} else {
		m_econType = ECON_AGRICULTURE;
	}

	if (m_unexplored) {
		m_shortDesc = Lang::UNEXPLORED_SYSTEM_NO_DATA;
	}

	/* Total population is in billions */
	else if(m_totalPop == 0) {
		m_shortDesc = Lang::SMALL_SCALE_PROSPECTING_NO_SETTLEMENTS;
	} else if (m_totalPop < fixed(1,10)) {
		switch (m_econType) {
			case ECON_INDUSTRY: m_shortDesc = Lang::SMALL_INDUSTRIAL_OUTPOST; break;
			case ECON_MINING: m_shortDesc = Lang::SOME_ESTABLISHED_MINING; break;
			case ECON_AGRICULTURE: m_shortDesc = Lang::YOUNG_FARMING_COLONY; break;
		}
	} else if (m_totalPop < fixed(1,2)) {
		switch (m_econType) {
			case ECON_INDUSTRY: m_shortDesc = Lang::INDUSTRIAL_COLONY; break;
			case ECON_MINING: m_shortDesc = Lang::MINING_COLONY; break;
			case ECON_AGRICULTURE: m_shortDesc = Lang::OUTDOOR_AGRICULTURAL_WORLD; break;
		}
	} else if (m_totalPop < fixed(5,1)) {
		switch (m_econType) {
			case ECON_INDUSTRY: m_shortDesc = Lang::HEAVY_INDUSTRY; break;
			case ECON_MINING: m_shortDesc = Lang::EXTENSIVE_MINING; break;
			case ECON_AGRICULTURE: m_shortDesc = Lang::THRIVING_OUTDOOR_WORLD; break;
		}
	} else {
		switch (m_econType) {
			case ECON_INDUSTRY: m_shortDesc = Lang::INDUSTRIAL_HUB_SYSTEM; break;
			case ECON_MINING: m_shortDesc = Lang::VAST_STRIP_MINE; break;
			case ECON_AGRICULTURE: m_shortDesc = Lang::HIGH_POPULATION_OUTDOOR_WORLD; break;
		}
	}
}

/* percent */
#define MAX_COMMODITY_BASE_PRICE_ADJUSTMENT 25

void StarSystem::Populate(bool addSpaceStations)
{
	PROFILE_SCOPED()
	Uint32 _init[5] = { m_path.systemIndex, Uint32(m_path.sectorX), Uint32(m_path.sectorY), Uint32(m_path.sectorZ), UNIVERSE_SEED };
	Random rand;
	rand.seed(_init, 5);

	/* Various system-wide characteristics */
	// This is 1 in sector (0,0,0) and approaches 0 farther out
	// (1,0,0) ~ .688, (1,1,0) ~ .557, (1,1,1) ~ .48
	m_humanProx = Faction::IsHomeSystem(m_path) ? fixed(2,3): fixed(3,1) / isqrt(9 + 10*(m_path.sectorX*m_path.sectorX + m_path.sectorY*m_path.sectorY + m_path.sectorZ*m_path.sectorZ));
	m_econType = ECON_INDUSTRY;
	m_industrial = rand.Fixed();
	m_agricultural = 0;

	/* system attributes */
	m_totalPop = fixed(0);
	m_rootBody->PopulateStage1(this, m_totalPop);

//	Output("Trading rates:\n");
	// So now we have balances of trade of various commodities.
	// Lets use black magic to turn these into percentage base price
	// alterations
	int maximum = 0;
	for (int i=Equip::FIRST_COMMODITY; i<=Equip::LAST_COMMODITY; i++) {
		maximum = std::max(abs(m_tradeLevel[i]), maximum);
	}
	if (maximum) {
		for (int i=Equip::FIRST_COMMODITY; i<=Equip::LAST_COMMODITY; i++) {
			m_tradeLevel[i] = (m_tradeLevel[i] * MAX_COMMODITY_BASE_PRICE_ADJUSTMENT) / maximum;
			m_tradeLevel[i] += rand.Int32(-5, 5);
		}
	}

// Unused?
//	for (int i=(int)Equip::FIRST_COMMODITY; i<=(int)Equip::LAST_COMMODITY; i++) {
//		Equip::Type t = (Equip::Type)i;
//		const EquipType &type = Equip::types[t];
//		Output("%s: %d%%\n", type.name, m_tradeLevel[t]);
//	}
//	Output("System total population %.3f billion\n", m_totalPop.ToFloat());
	Polit::GetSysPolitStarSystem(this, m_totalPop, m_polit);

	if (addSpaceStations) {
		m_rootBody->PopulateAddStations(this);
	}

	if (!m_shortDesc.size())
		MakeShortDescription(rand);
}


SystemBody* StarSystem::CreateHyperspaceCloudSBody(fixed orb_min, fixed orb_max, double orbit_phase)
{
    Uint32 _init[6] = {m_path.systemIndex, Uint32(m_path.sectorX),
        Uint32(m_path.sectorY), Uint32(m_path.sectorZ), Uint32(this->m_seed), UNIVERSE_SEED};

    Random rand;
    rand.seed(_init, 6);

    SystemBody* root_body = m_bodies.begin()->second.Get();
    /*fixed orbMaxS = fixed(1, 4) * root_body->CalcHillRadius();
    fixed orbMinS = 4 * root_body->m_radius * AU_EARTH_RADIUS;*/
    fixed orbMaxS = orb_max;
    fixed orbMinS = orb_min;

    SystemBody* sb = NewBody();
    sb->m_type = SystemBody::TYPE_HYPERSPACE_CLOUD;
    sb->m_seed = rand.Int32();
    sb->m_parent = root_body;
    sb->m_rotationPeriod = fixed(200, 24);
    sb->m_rotationalPhaseAtStart = fixed(0);
    sb->m_mass = fixed(100000);
    //sb->m_orbMin = orbMinS;
    //sb->m_orbMax = orbMaxS;
    sb->m_semiMajorAxis = orbMinS;
    sb->m_orbMin = sb->m_semiMajorAxis;
    sb->m_orbMax = sb->m_semiMajorAxis;
    sb->m_eccentricity = fixed(0);
    sb->m_axialTilt = fixed(0);
    sb->m_inclination = fixed(0);
    sb->m_radius = fixed(20000);

    sb->m_name = stringf("Perma-hypercloud %0", static_cast<int>(orbit_phase) + 1);
    /*if(root_body->m_type == SystemBody::TYPE_GRAVPOINT) {
        sb->m_orbit.SetShapeAroundBarycentre(sb->m_semiMajorAxis.ToDouble() * AU, root_body->GetMass(),
                                                 root_body->GetMass() * EARTH_MASS, 0.0);
    } else {
        sb->m_orbit.SetShapeAroundPrimary(sb->m_semiMajorAxis.ToDouble() * AU,
                                          root_body->GetMassAsFixed().ToDouble() * EARTH_MASS, 0.0);
    }*/
    sb->m_orbit.SetShapeAroundBarycentre(sb->m_semiMajorAxis.ToDouble() * AU, root_body->GetMass(),
                                         sb->GetMass() /*root_body->GetMass() * EARTH_MASS*/, 0.0);
    sb->m_orbit.SetPhase((orbit_phase / 2.0) * M_PI);

    //sb->m_orbit.SetPlane(matrix3x3d::Identity());
    double r1 = rand.Double(2 * M_PI);		// function parameter evaluation order is implementation-dependent
    double r2 = rand.NDouble(5);			// can't put two rands in the same expression
    sb->m_orbit.SetPlane(matrix3x3d::RotateY(r1) * matrix3x3d::RotateX(-0.5*M_PI + r2*M_PI / 2.0));

    sb->m_orbMin = sb->m_semiMajorAxis;
    sb->m_orbMax = sb->m_semiMajorAxis;

    root_body->m_children.insert(root_body->m_children.begin(), sb);
    m_hyperspaceClouds.push_back(sb);

    return sb;
}


void StarSystem::GeneratePermaHyperclouds()
{
    fixed orb_min(0), orb_max(0);
    for (auto iter = m_bodies.begin(); iter != m_bodies.end(); ++iter) {
        if(iter->second->m_orbMax > orb_max) {
            orb_max = iter->second->m_orbMax;
        }
        if(iter->second->m_orbMin > orb_min) {
            orb_min = iter->second->m_orbMin;
        }
    }

    for(double i = 0.0; i < 4.0; ++i) {
       CreateHyperspaceCloudSBody(orb_min, orb_max, i);
    }
}

StarSystem::~StarSystem()
{
	PROFILE_SCOPED()
	// clear parent and children pointers. someone (Lua) might still have a
	// reference to things that are about to be deleted
	m_rootBody->ClearParentAndChildPointers();
}

void StarSystem::Serialize(Serializer::Writer &wr, StarSystem *s)
{
	if (s) {
		wr.Byte(1);
		wr.Int32(s->m_path.sectorX);
		wr.Int32(s->m_path.sectorY);
		wr.Int32(s->m_path.sectorZ);
		wr.Int32(s->m_path.systemIndex);
	} else {
		wr.Byte(0);
	}
}

RefCountedPtr<StarSystem> StarSystem::Unserialize(Serializer::Reader &rd)
{
	if (rd.Byte()) {
		int sec_x = rd.Int32();
		int sec_y = rd.Int32();
		int sec_z = rd.Int32();
		int sys_idx = rd.Int32();
		sec_z = 0;
		return StarSystemCache::GetCached(SystemPath(sec_x, sec_y, sec_z, sys_idx));
	} else {
		return RefCountedPtr<StarSystem>(0);
	}
}

std::string StarSystem::ExportBodyToLua(FILE *f, SystemBody *body) {
	const int multiplier = 10000;
	int i;

	std::string code_name = body->GetName();
	std::transform(code_name.begin(), code_name.end(), code_name.begin(), ::tolower);
	code_name.erase(remove_if(code_name.begin(), code_name.end(), isspace), code_name.end());
	for(unsigned int j = 0; j < code_name.length(); j++) {
		if(code_name[j] == ',')
			code_name[j] = 'X';
		if(!((code_name[j] >= 'a' && code_name[j] <= 'z') ||
				(code_name[j] >= 'A' && code_name[j] <= 'Z') ||
				(code_name[j] >= '0' && code_name[j] <= '9')))
			code_name[j] = 'Y';
	}

	std::string code_list = code_name;

	for(i = 0; ENUM_BodyType[i].name != 0; i++) {
		if(ENUM_BodyType[i].value == body->GetType())
			break;
	}

	if(body->GetType() == SystemBody::TYPE_STARPORT_SURFACE) {
		fprintf(f,
			"local %s = CustomSystemBody:new(\"%s\", '%s')\n"
				"\t:latitude(math.deg2rad(%.1f))\n"
                "\t:longitude(math.deg2rad(%.1f))\n",

				code_name.c_str(),
				body->GetName().c_str(), ENUM_BodyType[i].name,
				body->m_inclination.ToDouble()*180/M_PI,
				body->m_orbitalOffset.ToDouble()*180/M_PI
				);
	} else {

		fprintf(f,
				"local %s = CustomSystemBody:new(\"%s\", '%s')\n"
				"\t:radius(f(%d,%d))\n"
				"\t:mass(f(%d,%d))\n",
				code_name.c_str(),
				body->GetName().c_str(), ENUM_BodyType[i].name,
				int(round(body->GetRadiusAsFixed().ToDouble()*multiplier)), multiplier,
				int(round(body->GetMassAsFixed().ToDouble()*multiplier)), multiplier
		);

		if(body->GetType() != SystemBody::TYPE_GRAVPOINT)
		fprintf(f,
				"\t:seed(%u)\n"
				"\t:temp(%d)\n"
				"\t:semi_major_axis(f(%d,%d))\n"
				"\t:eccentricity(f(%d,%d))\n"
				"\t:rotation_period(f(%d,%d))\n"
				"\t:axial_tilt(fixed.deg2rad(f(%d,%d)))\n"
				"\t:rotational_phase_at_start(fixed.deg2rad(f(%d,%d)))\n"
				"\t:orbital_phase_at_start(fixed.deg2rad(f(%d,%d)))\n"
				"\t:orbital_offset(fixed.deg2rad(f(%d,%d)))\n",
			body->GetSeed(), body->GetAverageTemp(),
			int(round(body->GetOrbit().GetSemiMajorAxis()/AU*multiplier)), multiplier,
			int(round(body->GetOrbit().GetEccentricity()*multiplier)), multiplier,
			int(round(body->m_rotationPeriod.ToDouble()*multiplier)), multiplier,
			int(round(body->GetAxialTilt()*multiplier)), multiplier,
			int(round(body->m_rotationalPhaseAtStart.ToDouble()*multiplier*180/M_PI)), multiplier,
			int(round(body->m_orbitalPhaseAtStart.ToDouble()*multiplier*180/M_PI)), multiplier,
			int(round(body->m_orbitalOffset.ToDouble()*multiplier*180/M_PI)), multiplier
		);

		if(body->GetType() == SystemBody::TYPE_PLANET_TERRESTRIAL)
			fprintf(f,
					"\t:metallicity(f(%d,%d))\n"
					"\t:volcanicity(f(%d,%d))\n"
					"\t:atmos_density(f(%d,%d))\n"
					"\t:atmos_oxidizing(f(%d,%d))\n"
					"\t:ocean_cover(f(%d,%d))\n"
					"\t:ice_cover(f(%d,%d))\n"
					"\t:life(f(%d,%d))\n",
				int(round(body->GetMetallicity().ToDouble()*multiplier)), multiplier,
				int(round(body->GetVolcanicity().ToDouble()*multiplier)), multiplier,
				int(round(body->GetVolatileGas().ToDouble()*multiplier)), multiplier,
				int(round(body->GetAtmosOxidizing().ToDouble()*multiplier)), multiplier,
				int(round(body->GetVolatileLiquid().ToDouble()*multiplier)), multiplier,
				int(round(body->GetVolatileIces().ToDouble()*multiplier)), multiplier,
				int(round(body->GetLife().ToDouble()*multiplier)), multiplier
			);
	}

	fprintf(f, "\n");

	if(body->m_children.size() > 0) {
		code_list = code_list + ", \n\t{\n";
		for (Uint32 ii = 0; ii < body->m_children.size(); ii++) {
			code_list = code_list + "\t" + ExportBodyToLua(f, body->m_children[ii]) + ", \n";
		}
		code_list = code_list + "\t}";
	}

	return code_list;

}

std::string StarSystem::GetStarTypes(SystemBody *body) {
	int i = 0;
	std::string types = "";

	if(body->GetSuperType() == SystemBody::SUPERTYPE_STAR) {
		for(i = 0; ENUM_BodyType[i].name != 0; i++) {
			if(ENUM_BodyType[i].value == body->GetType())
				break;
		}

		types = types + "'" + ENUM_BodyType[i].name + "', ";
	}

	for (Uint32 ii = 0; ii < body->m_children.size(); ii++) {
		types = types + GetStarTypes(body->m_children[ii]);
	}

	return types;
}

void StarSystem::ExportToLua(const char *filename) {
	FILE *f = fopen(filename,"w");
	int j;

	if(f == 0)
		return;

	fprintf(f,"-- Copyright © 2008-2012 Pioneer Developers. See AUTHORS.txt for details\n");
	fprintf(f,"-- Licensed under the terms of the GPL v3. See licenses/GPL-3.txt\n\n");

	std::string stars_in_system = GetStarTypes(m_rootBody.Get());

	for(j = 0; ENUM_PolitGovType[j].name != 0; j++) {
		if(ENUM_PolitGovType[j].value == GetSysPolit().govType)
			break;
	}

	fprintf(f,"local system = CustomSystem:new('%s', { %s })\n\t:govtype('%s')\n\t:short_desc('%s')\n\t:long_desc([[%s]])\n\n",
			GetName().c_str(), stars_in_system.c_str(), ENUM_PolitGovType[j].name, GetShortDescription(), GetLongDescription());

	fprintf(f, "system:bodies(%s)\n\n", ExportBodyToLua(f, m_rootBody.Get()).c_str());

	RefCountedPtr<const Sector> sec = Sector::cache.GetCached(GetPath());
	SystemPath pa = GetPath();

	fprintf(f, "system:add_to_sector(%d,%d,%d,v(%.4f,%.4f,%.4f))\n",
			pa.sectorX, pa.sectorY, pa.sectorZ,
			sec->m_systems[pa.systemIndex].p.x/Sector::SIZE,
			sec->m_systems[pa.systemIndex].p.y/Sector::SIZE,
			sec->m_systems[pa.systemIndex].p.z/Sector::SIZE);

	fclose(f);
}

RefCountedPtr<StarSystem> StarSystemCache::GetCached(const SystemPath &path)
{
	PROFILE_SCOPED()
	SystemPath sysPath(path.SystemOnly());

	StarSystem *s = 0;
	std::pair<SystemCacheMap::iterator, bool>
		ret = s_cachedSystems.insert(SystemCacheMap::value_type(sysPath, static_cast<StarSystem*>(0)));
	if (ret.second) {
		s = new StarSystem(sysPath);
		ret.first->second = s;
		s->IncRefCount(); // the cache owns one reference
	} else {
		s = ret.first->second;
	}
	return RefCountedPtr<StarSystem>(s);
}

static bool WithinBox(const SystemPath &here, const int Xmin, const int Xmax, const int Ymin, const int Ymax, const int Zmin, const int Zmax) {
	PROFILE_SCOPED()
	if(here.sectorX >= Xmin && here.sectorX <= Xmax) {
		if(here.sectorY >= Ymin && here.sectorY <= Ymax) {
			if(here.sectorZ >= Zmin && here.sectorZ <= Zmax) {
				return true;
			}
		}
	}
	return false;
}

void StarSystemCache::ShrinkCache(const SystemPath &here, const bool clear/*=false*/)
{
	PROFILE_SCOPED()
	// we're going to use these to determine if our StarSystems are within a range that we'll keep for later use
	static const int survivorRadius = 30;	// 3 times the distance used by the SectorCache population method.

	// min/max box limits
	const int xmin = here.sectorX-survivorRadius;
	const int xmax = here.sectorX+survivorRadius;
	const int ymin = here.sectorY-survivorRadius;
	const int ymax = here.sectorY+survivorRadius;
	const int zmin = here.sectorZ-survivorRadius;
	const int zmax = here.sectorZ+survivorRadius;

	std::map<SystemPath,StarSystem*>::iterator i = s_cachedSystems.begin();
	while (i != s_cachedSystems.end()) {
		StarSystem *s = (*i).second;

		const bool outsideVolume = clear || !WithinBox(s->GetPath(), xmin, xmax, ymin, ymax, zmin, zmax);

		assert(s->GetRefCount() >= 1); // sanity check
		// if the cache is the only owner, then delete it
		if (outsideVolume && s->GetRefCount() == 1) {
			delete s;
			s_cachedSystems.erase(i++);
		} else {
			i++;
		}
	}
}

SystemBody* StarSystem::NewBody()
{
	SystemBody* body = new SystemBody(SystemPath(m_path.sectorX, m_path.sectorY, m_path.sectorZ,
		m_path.systemIndex, m_bodies.size()));
	//m_bodies.push_back(RefCountedPtr<SystemBody>(body));
	m_bodies.insert(std::pair<Uint32, RefCountedPtr<SystemBody> >(m_nextBodyIndex, 
		RefCountedPtr<SystemBody>(body)));
	m_nextBodyIndex += 1;
	return body;
}

void StarSystem::DestroyBody(SystemBody* body)
{
	// For simplicity/less crash prone, DestroyBody can only destroy bodies with no children.
	assert(body->m_parent == nullptr);
	assert(body->GetNumChildren() == 0);
	if(body->GetNumChildren() > 0) {
		throw new std::runtime_error("StarSystem::DestroyBody() is given a system body with children.");
	}
	if(body->GetSuperType() == SystemBody::BodySuperType::SUPERTYPE_STARPORT) {
		auto sbody_search = std::find(m_spaceStations.begin(), m_spaceStations.end(), 
			body);
		if(sbody_search != m_spaceStations.end()) {
			m_spaceStations.erase(sbody_search);
		}
	} else if(body->GetSuperType() == SystemBody::BodySuperType::SUPERTYPE_STAR) {
		auto sbody_search = std::find(m_stars.begin(), m_stars.end(),
			body);
		if (sbody_search != m_stars.end()) {
			m_stars.erase(sbody_search);
		}
	} else if(body->GetSuperType() == SystemBody::BodySuperType::SUPERTYPE_HYPERSPACE_CLOUD) {
		auto sbody_search = std::find(m_hyperspaceClouds.begin(), m_hyperspaceClouds.end(),
			body);
		if (sbody_search != m_hyperspaceClouds.end()) {
			m_hyperspaceClouds.erase(sbody_search);
		}
	}
	if(body->m_parent) {
		body->m_parent->RemoveChild(body);
	}
	body->ClearParentAndChildPointers();
}