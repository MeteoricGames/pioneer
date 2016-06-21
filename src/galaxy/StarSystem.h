// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _STARSYSTEM_H
#define _STARSYSTEM_H

#include "libs.h"
#include "EquipType.h"
#include "Polit.h"
#include "Serializer.h"
#include <vector>
#include <string>
#include "RefCounted.h"
#include "galaxy/SystemPath.h"
#include "Orbit.h"
#include "IterationProxy.h"
#include "gameconsts.h"
#include <SDL_stdinc.h>

class CustomSystemBody;
class CustomSystem;
class SystemBody;

// doubles - all masses in Kg, all lengths in meters
// fixed - any mad scheme

enum EconType { // <enum name=EconType prefix=ECON_ public>
	ECON_MINING = 1<<0,
	ECON_AGRICULTURE = 1<<1,
	ECON_INDUSTRY = 1<<2,
};

class StarSystem;
class Faction;

struct RingStyle {
	// note: radius values are given as proportions of the planet radius
	// (e.g., 1.6)
	fixed minRadius;
	fixed maxRadius;
	Color baseColor;
};

class SystemBody : public RefCounted {
public:
	SystemBody(const SystemPath& path);
	void PickPlanetType(Random &rand);
	const SystemBody *FindStarAndTrueOrbitalRange(fixed &orbMin, fixed &orbMax);

	enum BodyType { // <enum scope='SystemBody' prefix=TYPE_ public>
		TYPE_GRAVPOINT = 0,
		TYPE_BROWN_DWARF = 1, //  L+T Class Brown Dwarfs
		TYPE_WHITE_DWARF = 2,
		TYPE_STAR_M = 3, //red
		TYPE_STAR_K = 4, //orange
		TYPE_STAR_G = 5, //yellow
		TYPE_STAR_F = 6, //white
		TYPE_STAR_A = 7, //blue/white
		TYPE_STAR_B = 8, //blue
		TYPE_STAR_O = 9,  //blue/purple/white
		TYPE_STAR_M_GIANT = 10,
		TYPE_STAR_K_GIANT = 11,
		TYPE_STAR_G_GIANT = 12,
		TYPE_STAR_F_GIANT = 13,
		TYPE_STAR_A_GIANT = 14,
		TYPE_STAR_B_GIANT = 15,
		TYPE_STAR_O_GIANT = 16,
		TYPE_STAR_M_SUPER_GIANT = 17,
		TYPE_STAR_K_SUPER_GIANT = 18,
		TYPE_STAR_G_SUPER_GIANT = 19,
		TYPE_STAR_F_SUPER_GIANT = 20,
		TYPE_STAR_A_SUPER_GIANT = 21,
		TYPE_STAR_B_SUPER_GIANT = 22,
		TYPE_STAR_O_SUPER_GIANT = 23,
		TYPE_STAR_M_HYPER_GIANT = 24,
		TYPE_STAR_K_HYPER_GIANT = 25,
		TYPE_STAR_G_HYPER_GIANT = 26,
		TYPE_STAR_F_HYPER_GIANT = 27,
		TYPE_STAR_A_HYPER_GIANT = 28,
		TYPE_STAR_B_HYPER_GIANT = 29,
		TYPE_STAR_O_HYPER_GIANT = 30, // these various stars do exist = they are transitional states and are rare
		TYPE_STAR_M_WF = 31,  //Wolf-Rayet star
		TYPE_STAR_B_WF = 32,  // while you do not specifically get class M,B or O WF stars,
		TYPE_STAR_O_WF = 33, //  you do get red = blue and purple from the colour of the gasses = so spectral class is an easy way to define them.
		TYPE_STAR_S_BH = 34, //stellar blackhole
		TYPE_STAR_IM_BH = 35, //Intermediate-mass blackhole
		TYPE_STAR_SM_BH = 36, //Supermassive blackhole
		TYPE_PLANET_GAS_GIANT = 37,
		TYPE_PLANET_ASTEROID = 38,
		TYPE_PLANET_TERRESTRIAL = 39,
		TYPE_STARPORT_ORBITAL = 40,
		TYPE_STARPORT_SURFACE = 41,
		TYPE_HYPERSPACE_CLOUD = 42,
		TYPE_MIN = TYPE_BROWN_DWARF, // <enum skip>
		TYPE_MAX = TYPE_STARPORT_SURFACE, // <enum skip>
		TYPE_STAR_MIN = TYPE_BROWN_DWARF, // <enum skip>
		TYPE_STAR_MAX = TYPE_STAR_SM_BH, // <enum skip>
		// XXX need larger atmosphereless thing
	};

	enum BodySuperType { // <enum scope='SystemBody' prefix=SUPERTYPE_ public>
		SUPERTYPE_NONE = 0,
		SUPERTYPE_STAR = 1,
		SUPERTYPE_ROCKY_PLANET = 2,
		SUPERTYPE_GAS_GIANT = 3,
		SUPERTYPE_STARPORT = 4,
		SUPERTYPE_HYPERSPACE_CLOUD = 5,
	};

	const SystemPath& GetPath() const { return m_path; }
	SystemBody* GetParent() const { return m_parent; }

	bool HasChildren() const { return !m_children.empty(); }
	unsigned GetNumChildren() const { return m_children.size(); }
	IterationProxy<std::vector<SystemBody*> > GetChildren() { return MakeIterationProxy(m_children); }
	const IterationProxy<const std::vector<SystemBody*> > GetChildren() const { return MakeIterationProxy(m_children); }

	std::string GetName() const { return m_name; }
	std::string GetAstroDescription() const;
	const char *GetIcon() const;
	BodyType GetType() const { return m_type; }
	BodySuperType GetSuperType() const;
	bool IsCustomBody() const { return m_isCustomBody; }
	fixed GetRadiusAsFixed() const { return m_radius; }
	double GetRadius() const { // polar radius
		if (GetSuperType() <= SUPERTYPE_STAR)
			return (m_radius.ToDouble() / m_aspectRatio.ToDouble()) * SOL_RADIUS;
		else
			return m_radius.ToDouble() * EARTH_RADIUS;
	}
	double GetAspectRatio() const { return m_aspectRatio.ToDouble(); }
	fixed GetMassAsFixed() const { return m_mass; }
	double GetMass() const {
		if (GetSuperType() <= SUPERTYPE_STAR)
			return m_mass.ToDouble() * SOL_MASS;
		else
			return m_mass.ToDouble() * EARTH_MASS;
	}
	fixed GetMassInEarths() const {
		if (GetSuperType() <= SUPERTYPE_STAR)
			return m_mass * 332998;
		else
			return m_mass;
	}
	bool IsRotating() const { return m_rotationPeriod != fixed(0); }
	// returned in seconds
	double GetRotationPeriodInDays() const { return m_rotationPeriod.ToDouble(); }
	double GetRotationPeriod() const {
		return m_rotationPeriod.ToDouble()*60*60*24;
	}
	bool HasRotationPhase() const { return m_rotationalPhaseAtStart != fixed(0); }
	double GetRotationPhaseAtStart() const { return m_rotationalPhaseAtStart.ToDouble(); }
	double GetAxialTilt() const { return m_axialTilt.ToDouble(); }

	const Orbit& GetOrbit() const { return m_orbit; }
	double GetEccentricity() const { return m_eccentricity.ToDouble(); }
	double GetOrbMin() const { return m_orbMin.ToDouble(); }
	double GetOrbMax() const { return m_orbMax.ToDouble(); }
	double GetSemiMajorAxis() const { return m_semiMajorAxis.ToDouble(); }
	void SetOrbitPlane(const matrix3x3d &orient) { m_orbit.SetPlane(orient); }

	int GetAverageTemp() const { return m_averageTemp; }
	std::string GetHeightMapFilename() const { return m_heightMapFilename; }
	unsigned int GetHeightMapFractal() const { return m_heightMapFractal; }

	Uint32 GetSeed() const { return m_seed; }

	fixed GetMetallicity() const { return m_metallicity; }
	fixed GetVolatileGas() const { return m_volatileGas; }
	fixed GetVolatileLiquid() const { return m_volatileLiquid; }
	fixed GetVolatileIces() const { return m_volatileIces; }
	fixed GetVolcanicity() const { return m_volcanicity; }
	fixed GetAtmosOxidizing() const { return m_atmosOxidizing; }
	fixed GetLife() const { return m_life; }

	double GetPopulation() const { return m_population.ToDouble(); }

	fixed CalcHillRadius() const;
	static int CalcSurfaceTemp(const SystemBody *primary, fixed distToPrimary, fixed albedo, fixed greenhouse);
	double CalcSurfaceGravity() const;

	double GetMaxChildOrbitalDistance() const;
	void PositionSettlementOnPlanet();
	void PopulateStage1(StarSystem *system, fixed &outTotalPop);
	void PopulateAddStations(StarSystem *system);

	bool HasRings() const { return bool(m_rings.maxRadius.v); }
	const RingStyle& GetRings() const { return m_rings; }
	void PickRings(bool forceRings = false);


	// XXX merge all this atmosphere stuff
	bool HasAtmosphere() const;

	void PickAtmosphere();
	void GetAtmosphereFlavor(Color *outColor, double *outDensity) const {
		*outColor = m_atmosColor;
		*outDensity = m_atmosDensity;
	}

	struct AtmosphereParameters {
		float atmosRadius;
		float atmosInvScaleHeight;
		float atmosDensity;
		float planetRadius;
		Color atmosCol;
		vector3d center;
		float scale;
	};

	AtmosphereParameters CalcAtmosphereParams() const;


	bool IsScoopable() const;

private:
	friend class StarSystem;
	friend class ObjectViewerView;

	void ClearParentAndChildPointers();
	void RemoveChild(SystemBody* child);

	SystemBody *m_parent;                // these are only valid if the StarSystem
	std::vector<SystemBody*> m_children; // that create them still exists

	Uint32 m_id; // index into starsystem->m_bodies
	SystemPath m_path;
	Orbit m_orbit;
	Uint32 m_seed; // Planet.cpp can use to generate terrain
	std::string m_name;
	fixed m_radius; // in earth radii for planets, sol radii for stars. equatorial radius in case of bodies which are flattened at the poles
	fixed m_aspectRatio; // ratio between equatorial and polar radius for bodies with eqatorial bulges
	fixed m_mass; // earth masses if planet, solar masses if star
	fixed m_orbMin, m_orbMax; // periapsism, apoapsis in AUs
	fixed m_rotationPeriod; // in days
	fixed m_rotationalPhaseAtStart; // 0 to 2 pi
	fixed m_humanActivity; // 0 - 1
	fixed m_semiMajorAxis; // in AUs
	fixed m_eccentricity;
	fixed m_orbitalOffset;
	fixed m_orbitalPhaseAtStart; // 0 to 2 pi
	fixed m_axialTilt; // in radians
	fixed m_inclination; // in radians, for surface bodies = latitude
	int m_averageTemp;
	BodyType m_type;
	bool m_isCustomBody;

	/* composition */
	fixed m_metallicity; // (crust) 0.0 = light (Al, SiO2, etc), 1.0 = heavy (Fe, heavy metals)
	fixed m_volatileGas; // 1.0 = earth atmosphere density
	fixed m_volatileLiquid; // 1.0 = 100% ocean cover (earth = 70%)
	fixed m_volatileIces; // 1.0 = 100% ice cover (earth = 3%)
	fixed m_volcanicity; // 0 = none, 1.0 = fucking volcanic
	fixed m_atmosOxidizing; // 0.0 = reducing (H2, NH3, etc), 1.0 = oxidising (CO2, O2, etc)
	fixed m_life; // 0.0 = dead, 1.0 = teeming

	RingStyle m_rings;

	/* economy type stuff */
	fixed m_population;
	fixed m_agricultural;

	std::string m_heightMapFilename;
	unsigned int m_heightMapFractal;

	Color m_atmosColor;
	double m_atmosDensity;
};

class StarSystem : public RefCounted {
public:
	friend class SystemBody;
	friend class StarSystemCache;

	void ExportToLua(const char *filename);

	const std::string &GetName() const { return m_name; }
	SystemPath GetPathOf(const SystemBody *sbody) const;
	SystemBody *GetBodyByPath(const SystemPath &path) const;
	static void Serialize(Serializer::Writer &wr, StarSystem *);
	static RefCountedPtr<StarSystem> Unserialize(Serializer::Reader &rd);
	void Dump();
	const SystemPath &GetPath() const { return m_path; }
	const char *GetShortDescription() const { return m_shortDesc.c_str(); }
	const char *GetLongDescription() const { return m_longDesc.c_str(); }
	int GetNumStars() const { return m_numStars; }
	const SysPolit &GetSysPolit() const { return m_polit; }

	static const Uint8 starColors[][3];
	static const Uint8 starRealColors[][3];
	static const double starLuminosities[];
	static const float starScale[];
	static const fixed starMetallicities[];

	RefCountedPtr<const SystemBody> GetRootBody() const { return m_rootBody; }
	RefCountedPtr<SystemBody> GetRootBody() { return m_rootBody; }
	bool HasSpaceStations() const { return !m_spaceStations.empty(); }
	unsigned GetNumSpaceStations() const { return m_spaceStations.size(); }
	IterationProxy<std::vector<SystemBody*> > GetSpaceStations() { return MakeIterationProxy(m_spaceStations); }
	const IterationProxy<const std::vector<SystemBody*> > GetSpaceStations() const { return MakeIterationProxy(m_spaceStations); }
	IterationProxy<std::vector<SystemBody*> > GetStars() { return MakeIterationProxy(m_stars); }
	const IterationProxy<const std::vector<SystemBody*> > GetStars() const { return MakeIterationProxy(m_stars); }
	unsigned GetNumBodies() const { return m_bodies.size(); }
	//IterationProxy<std::map<Uint32, RefCountedPtr<SystemBody> > > GetBodies() { return MakeIterationProxy(m_bodies); }
	//const IterationProxy<const std::map<Uint32, RefCountedPtr<SystemBody> > > GetBodies() const { return MakeIterationProxy(m_bodies); }
	std::map<Uint32, RefCountedPtr<SystemBody> >& GetBodiesMap() { return m_bodies; }
	const std::map<Uint32, RefCountedPtr<SystemBody> >& GetBodiesMap() const { return m_bodies; }

	int GetCommodityBasePriceModPercent(int t) {
		return m_tradeLevel[t];
	}

	Faction* GetFaction() const  { return m_faction; }
	bool GetUnexplored() const { return m_unexplored; }
	fixed GetMetallicity() const { return m_metallicity; }
	fixed GetIndustrial() const { return m_industrial; }
	int GetEconType() const { return m_econType; }
	int GetSeed() const { return m_seed; }
	const int* GetTradeLevel() const { return m_tradeLevel; }
	fixed GetAgricultural() const { return m_agricultural; }
	fixed GetHumanProx() const { return m_humanProx; }
	fixed GetTotalPop() const { return m_totalPop; }
	void DestroyBody(SystemBody* body);
    SystemBody* CreateHyperspaceCloudSBody(fixed orb_min, fixed orb_max, double orbit_phase);

private:
	StarSystem(const SystemPath &path);
	~StarSystem();

	SystemBody *NewBody();

	void MakeShortDescription(Random &rand);
	void MakePlanetsAround(SystemBody *primary, Random &rand);
	void MakeRandomStar(SystemBody *sbody, Random &rand);
	void MakeStarOfType(SystemBody *sbody, SystemBody::BodyType type, Random &rand);
	void MakeStarOfTypeLighterThan(SystemBody *sbody, SystemBody::BodyType type, fixed maxMass, Random &rand);
	void MakeBinaryPair(SystemBody *a, SystemBody *b, fixed minDist, Random &rand);
	void CustomGetKidsOf(SystemBody *parent, const std::vector<CustomSystemBody*> &children, int *outHumanInfestedness, Random &rand);
	void GenerateFromCustom(const CustomSystem *, Random &rand);
	void Populate(bool addSpaceStations);
    void GeneratePermaHyperclouds();

	std::string ExportBodyToLua(FILE *f, SystemBody *body);
	std::string GetStarTypes(SystemBody *body);

	SystemPath m_path;
	int m_numStars;
	std::string m_name;
	std::string m_shortDesc, m_longDesc;
	SysPolit m_polit;

	bool m_isCustom;
	bool m_hasCustomBodies;

	Faction* m_faction;
	bool m_unexplored;
	fixed m_metallicity;
	fixed m_industrial;
	int m_econType;
	int m_seed;

	// percent price alteration
	int m_tradeLevel[Equip::TYPE_MAX];

	fixed m_agricultural;
	fixed m_humanProx;
	fixed m_totalPop;

	RefCountedPtr<SystemBody> m_rootBody;
	// index into this will be the SystemBody ID used by SystemPath
	//std::vector< RefCountedPtr<SystemBody> > m_bodies;
	std::map<Uint32, RefCountedPtr<SystemBody> > m_bodies;
	Uint32 m_nextBodyIndex;
	std::vector<SystemBody*> m_spaceStations;
	std::vector<SystemBody*> m_stars;
	std::list<SystemBody*> m_hyperspaceClouds;
};

class StarSystemCache
{
public:
	static RefCountedPtr<StarSystem> GetCached(const SystemPath &path);
	static void ShrinkCache(const SystemPath &path, const bool clear=false);

private:
	typedef std::map<SystemPath,StarSystem*> SystemCacheMap;
	static SystemCacheMap s_cachedSystems;
};

namespace StarSystemConstants
{
    static const double CELSIUS = 273.15;
    //#define DEBUG_DUMP

    // minimum moon mass a little under Europa's
    static const fixed MIN_MOON_MASS = fixed(1, 30000); // earth masses
    static const fixed MIN_MOON_DIST = fixed(15, 10000); // AUs
    static const fixed MAX_MOON_DIST = fixed(2, 100); // AUs
    // if binary stars have separation s, planets can have stable
    // orbits at (0.5 * s * SAFE_DIST_FROM_BINARY)
    static const fixed SAFE_DIST_FROM_BINARY = fixed(5, 1);
    static const fixed PLANET_MIN_SEPARATION = fixed(135, 100);

    // very crudely
    static const fixed AU_SOL_RADIUS = fixed(305, 65536);
    static const fixed AU_EARTH_RADIUS = fixed(3, 65536);

    static const fixed SUN_MASS_TO_EARTH_MASS = fixed(332998, 1);

    static const fixed FIXED_PI = fixed(103993, 33102);

    static const struct StarTypeInfo {
        SystemBody::BodySuperType supertype;
        int mass[2]; // min,max % sol for stars, unused for planets
        int radius[2]; // min,max % sol radii for stars, % earth radii for planets
        int tempMin, tempMax;
    } starTypeInfo[] = {
        {
            SystemBody::SUPERTYPE_NONE, {}, {},
            0, 0
        }, {
            SystemBody::SUPERTYPE_STAR, //Brown Dwarf
            {2, 8}, {10, 30},
            1000, 2000
        }, {
            SystemBody::SUPERTYPE_STAR,  //white dwarf
            {20, 100}, {1, 2},
            4000, 40000
        }, {
            SystemBody::SUPERTYPE_STAR, //M
            {10, 47}, {30, 60},
            2000, 3500
        }, {
            SystemBody::SUPERTYPE_STAR, //K
            {50, 78}, {60, 100},
            3500, 5000
        }, {
            SystemBody::SUPERTYPE_STAR, //G
            {80, 110}, {80, 120},
            5000, 6000
        }, {
            SystemBody::SUPERTYPE_STAR, //F
            {115, 170}, {110, 150},
            6000, 7500
        }, {
            SystemBody::SUPERTYPE_STAR, //A
            {180, 320}, {120, 220},
            7500, 10000
        }, {
            SystemBody::SUPERTYPE_STAR,  //B
            {200, 300}, {120, 290},
            10000, 30000
        }, {
            SystemBody::SUPERTYPE_STAR, //O
            {300, 400}, {200, 310},
            30000, 60000
        }, {
            SystemBody::SUPERTYPE_STAR, //M Giant
            {60, 357}, {2000, 5000},
            2500, 3500
        }, {
            SystemBody::SUPERTYPE_STAR, //K Giant
            {125, 500}, {1500, 3000},
            3500, 5000
        }, {
            SystemBody::SUPERTYPE_STAR, //G Giant
            {200, 800}, {1000, 2000},
            5000, 6000
        }, {
            SystemBody::SUPERTYPE_STAR, //F Giant
            {250, 900}, {800, 1500},
            6000, 7500
        }, {
            SystemBody::SUPERTYPE_STAR, //A Giant
            {400, 1000}, {600, 1000},
            7500, 10000
        }, {
            SystemBody::SUPERTYPE_STAR,  //B Giant
            {500, 1000}, {600, 1000},
            10000, 30000
        }, {
            SystemBody::SUPERTYPE_STAR, //O Giant
            {600, 1200}, {600, 1000},
            30000, 60000
        }, {
            SystemBody::SUPERTYPE_STAR, //M Super Giant
            {1050, 5000}, {7000, 15000},
            2500, 3500
        }, {
            SystemBody::SUPERTYPE_STAR, //K Super Giant
            {1100, 5000}, {5000, 9000},
            3500, 5000
        }, {
            SystemBody::SUPERTYPE_STAR, //G Super Giant
            {1200, 5000}, {4000, 8000},
            5000, 6000
        }, {
            SystemBody::SUPERTYPE_STAR, //F Super Giant
            {1500, 6000}, {3500, 7000},
            6000, 7500
        }, {
            SystemBody::SUPERTYPE_STAR, //A Super Giant
            {2000, 8000}, {3000, 6000},
            7500, 10000
        }, {
            SystemBody::SUPERTYPE_STAR,  //B Super Giant
            {3000, 9000}, {2500, 5000},
            10000, 30000
        }, {
            SystemBody::SUPERTYPE_STAR, //O Super Giant
            {5000, 10000}, {2000, 4000},
            30000, 60000
        }, {
            SystemBody::SUPERTYPE_STAR, //M Hyper Giant
            {5000, 15000}, {20000, 40000},
            2500, 3500
        }, {
            SystemBody::SUPERTYPE_STAR, //K Hyper Giant
            {5000, 17000}, {17000, 25000},
            3500, 5000
        }, {
            SystemBody::SUPERTYPE_STAR, //G Hyper Giant
            {5000, 18000}, {14000, 20000},
            5000, 6000
        }, {
            SystemBody::SUPERTYPE_STAR, //F Hyper Giant
            {5000, 19000}, {12000, 17500},
            6000, 7500
        }, {
            SystemBody::SUPERTYPE_STAR, //A Hyper Giant
            {5000, 20000}, {10000, 15000},
            7500, 10000
        }, {
            SystemBody::SUPERTYPE_STAR,  //B Hyper Giant
            {5000, 23000}, {6000, 10000},
            10000, 30000
        }, {
            SystemBody::SUPERTYPE_STAR, //O Hyper Giant
            {10000, 30000}, {4000, 7000},
            30000, 60000
        }, {
            SystemBody::SUPERTYPE_STAR,  // M WF
            {2000, 5000}, {2500, 5000},
            25000, 35000
        }, {
            SystemBody::SUPERTYPE_STAR,  // B WF
            {2000, 7500}, {2500, 5000},
            35000, 45000
        }, {
            SystemBody::SUPERTYPE_STAR,  // O WF
            {2000, 10000}, {2500, 5000},
            45000, 60000
        }, {
            SystemBody::SUPERTYPE_STAR,  // S BH
            {20, 2000}, {0, 0},	// XXX black holes are < 1 Sol radii big; this is clamped to a non-zero value later
            10, 24
        }, {
            SystemBody::SUPERTYPE_STAR,  // IM BH
            {900000, 1000000}, {100, 500},
            1, 10
        }, {
            SystemBody::SUPERTYPE_STAR,  // SM BH
            {2000000, 5000000}, {10000, 20000},
            10, 24
        }
        /*	}, {
        SystemBody::SUPERTYPE_GAS_GIANT,
        {}, 950, Lang::MEDIUM_GAS_GIANT,
        }, {
        SystemBody::SUPERTYPE_GAS_GIANT,
        {}, 1110, Lang::LARGE_GAS_GIANT,
        }, {
        SystemBody::SUPERTYPE_GAS_GIANT,
        {}, 1500, Lang::VERY_LARGE_GAS_GIANT,
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 1, Lang::ASTEROID,
        "icons/object_planet_asteroid.png"
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 2, "Large asteroid",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 26, "Small, rocky dwarf planet", // moon radius
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 26, "Small, rocky dwarf planet", // dwarf2 for moon-like colours
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 52, "Small, rocky planet with a thin atmosphere", // mars radius
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Rocky frozen planet with a thin nitrogen atmosphere", // earth radius
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Dead world that once housed it's own intricate ecosystem.", // earth radius
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Rocky planet with a carbon dioxide atmosphere",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Rocky planet with a methane atmosphere",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Water world with vast oceans and a thick nitrogen atmosphere",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Rocky planet with a thick carbon dioxide atmosphere",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Rocky planet with a thick methane atmosphere",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "Highly volcanic world",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 100, "World with indigenous life and an oxygen atmosphere",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 60, "Marginal terraformed world with minimal plant life",
        }, {
        SystemBody::SUPERTYPE_ROCKY_PLANET,
        {}, 90, "Fully terraformed world with introduced species from numerous successful colonies",
        }, {
        SystemBody::SUPERTYPE_STARPORT,
        {}, 0, Lang::ORBITAL_STARPORT,
        }, {
        SystemBody::SUPERTYPE_STARPORT,
        {}, 0, Lang::STARPORT,
        }*/
    };
}

#endif /* _STARSYSTEM_H */
