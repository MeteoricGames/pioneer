// Copyright � 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#include "StarSystem.h"
#include "LuaNameGen.h"
#include "Pi.h"
#include "Lang.h"

using namespace StarSystemConstants;

static bool check_unique_station_name(const std::string & name, const StarSystem * system) {
    PROFILE_SCOPED()
        bool ret = true;
    for (const SystemBody *station : system->GetSpaceStations())
        if (station->GetName() == name) {
            ret = false;
            break;
        }
    return ret;
}

static std::string gen_unique_station_name(SystemBody *sp, const StarSystem *system, RefCountedPtr<Random> &namerand) {
    PROFILE_SCOPED()
        std::string name;
    do {
        name = Pi::luaNameGen->BodyName(sp, namerand);
    } while (!check_unique_station_name(name, system));
    return name;
}

/*
* These are the nice floating point surface temp calculating turds.
*
static const double boltzman_const = 5.6704e-8;
static double calcEnergyPerUnitAreaAtDist(double star_radius, double star_temp, double object_dist)
{
const double total_solar_emission = boltzman_const *
star_temp*star_temp*star_temp*star_temp*
4*M_PI*star_radius*star_radius;

return total_solar_emission / (4*M_PI*object_dist*object_dist);
}

// bond albedo, not geometric
static double CalcSurfaceTemp(double star_radius, double star_temp, double object_dist, double albedo, double greenhouse)
{
const double energy_per_meter2 = calcEnergyPerUnitAreaAtDist(star_radius, star_temp, object_dist);
const double surface_temp = pow(energy_per_meter2*(1-albedo)/(4*(1-greenhouse)*boltzman_const), 0.25);
return surface_temp;
}
*/
/*
* Instead we use these butt-ugly overflow-prone spat of ejaculate:
*/

/*
* star_radius in sol radii
* star_temp in kelvin,
* object_dist in AU
* return Watts/m^2
*/
static fixed calcEnergyPerUnitAreaAtDist(fixed star_radius, int star_temp, fixed object_dist)
{
    PROFILE_SCOPED()
        fixed temp = star_temp * fixed(1, 10000);
    const fixed total_solar_emission =
        temp*temp*temp*temp*star_radius*star_radius;

    return fixed(1744665451, 100000)*(total_solar_emission / (object_dist*object_dist));
}

//static
int SystemBody::CalcSurfaceTemp(const SystemBody *primary, fixed distToPrimary, fixed albedo, fixed greenhouse)
{
    PROFILE_SCOPED()
        fixed energy_per_meter2;
    if (primary->GetType() == SystemBody::TYPE_GRAVPOINT) {
        // binary. take energies of both stars
        energy_per_meter2 = calcEnergyPerUnitAreaAtDist(primary->m_children[0]->m_radius,
            primary->m_children[0]->m_averageTemp, distToPrimary);
        energy_per_meter2 += calcEnergyPerUnitAreaAtDist(primary->m_children[1]->m_radius,
            primary->m_children[1]->m_averageTemp, distToPrimary);
    } else {
        energy_per_meter2 = calcEnergyPerUnitAreaAtDist(primary->m_radius, primary->m_averageTemp, distToPrimary);
    }
    const fixed surface_temp_pow4 = energy_per_meter2*(1 - albedo) / (1 - greenhouse);
    return int(isqrt(isqrt((surface_temp_pow4.v >> fixed::FRAC) * 4409673)));
}

SystemBody::SystemBody(const SystemPath& path) : m_path(path)
{
    PROFILE_SCOPED()
        m_heightMapFractal = 0;
    m_aspectRatio = fixed(1, 1);
    m_rotationalPhaseAtStart = fixed(0);
    m_orbitalPhaseAtStart = fixed(0);
    m_orbMin = fixed(0);
    m_orbMax = fixed(0);
    m_semiMajorAxis = fixed(0);
    m_eccentricity = fixed(0);
    m_orbitalOffset = fixed(0);
    m_inclination = fixed(0);
    m_axialTilt = fixed(0);
    m_isCustomBody = false;
}

SystemBody::BodySuperType SystemBody::GetSuperType() const
{
    PROFILE_SCOPED()
        switch (m_type) {
        case TYPE_BROWN_DWARF:
        case TYPE_WHITE_DWARF:
        case TYPE_STAR_M:
        case TYPE_STAR_K:
        case TYPE_STAR_G:
        case TYPE_STAR_F:
        case TYPE_STAR_A:
        case TYPE_STAR_B:
        case TYPE_STAR_O:
        case TYPE_STAR_M_GIANT:
        case TYPE_STAR_K_GIANT:
        case TYPE_STAR_G_GIANT:
        case TYPE_STAR_F_GIANT:
        case TYPE_STAR_A_GIANT:
        case TYPE_STAR_B_GIANT:
        case TYPE_STAR_O_GIANT:
        case TYPE_STAR_M_SUPER_GIANT:
        case TYPE_STAR_K_SUPER_GIANT:
        case TYPE_STAR_G_SUPER_GIANT:
        case TYPE_STAR_F_SUPER_GIANT:
        case TYPE_STAR_A_SUPER_GIANT:
        case TYPE_STAR_B_SUPER_GIANT:
        case TYPE_STAR_O_SUPER_GIANT:
        case TYPE_STAR_M_HYPER_GIANT:
        case TYPE_STAR_K_HYPER_GIANT:
        case TYPE_STAR_G_HYPER_GIANT:
        case TYPE_STAR_F_HYPER_GIANT:
        case TYPE_STAR_A_HYPER_GIANT:
        case TYPE_STAR_B_HYPER_GIANT:
        case TYPE_STAR_O_HYPER_GIANT:
        case TYPE_STAR_M_WF:
        case TYPE_STAR_B_WF:
        case TYPE_STAR_O_WF:
        case TYPE_STAR_S_BH:
        case TYPE_STAR_IM_BH:
        case TYPE_STAR_SM_BH:
            return SUPERTYPE_STAR;
        case TYPE_PLANET_GAS_GIANT:
            return SUPERTYPE_GAS_GIANT;
        case TYPE_PLANET_ASTEROID:
        case TYPE_PLANET_TERRESTRIAL:
            return SUPERTYPE_ROCKY_PLANET;
        case TYPE_STARPORT_ORBITAL:
        case TYPE_STARPORT_SURFACE:
            return SUPERTYPE_STARPORT;
        case TYPE_HYPERSPACE_CLOUD:
            return SUPERTYPE_HYPERSPACE_CLOUD;
        case TYPE_GRAVPOINT:
            return SUPERTYPE_NONE;
        default:
            Output("Warning: Invalid SuperBody Type found.\n");
            return SUPERTYPE_NONE;
    }
}

std::string SystemBody::GetAstroDescription() const
{
    PROFILE_SCOPED()
        switch (m_type) {
        case TYPE_BROWN_DWARF: return Lang::BROWN_DWARF;
        case TYPE_WHITE_DWARF: return Lang::WHITE_DWARF;
        case TYPE_STAR_M: return Lang::STAR_M;
        case TYPE_STAR_K: return Lang::STAR_K;
        case TYPE_STAR_G: return Lang::STAR_G;
        case TYPE_STAR_F: return Lang::STAR_F;
        case TYPE_STAR_A: return Lang::STAR_A;
        case TYPE_STAR_B: return Lang::STAR_B;
        case TYPE_STAR_O: return Lang::STAR_O;
        case TYPE_STAR_M_GIANT: return Lang::STAR_M_GIANT;
        case TYPE_STAR_K_GIANT: return Lang::STAR_K_GIANT;
        case TYPE_STAR_G_GIANT: return Lang::STAR_G_GIANT;
        case TYPE_STAR_F_GIANT: return Lang::STAR_AF_GIANT;
        case TYPE_STAR_A_GIANT: return Lang::STAR_AF_GIANT;
        case TYPE_STAR_B_GIANT: return Lang::STAR_B_GIANT;
        case TYPE_STAR_O_GIANT: return Lang::STAR_O_GIANT;
        case TYPE_STAR_M_SUPER_GIANT: return Lang::STAR_M_SUPER_GIANT;
        case TYPE_STAR_K_SUPER_GIANT: return Lang::STAR_K_SUPER_GIANT;
        case TYPE_STAR_G_SUPER_GIANT: return Lang::STAR_G_SUPER_GIANT;
        case TYPE_STAR_F_SUPER_GIANT: return Lang::STAR_AF_SUPER_GIANT;
        case TYPE_STAR_A_SUPER_GIANT: return Lang::STAR_AF_SUPER_GIANT;
        case TYPE_STAR_B_SUPER_GIANT: return Lang::STAR_B_SUPER_GIANT;
        case TYPE_STAR_O_SUPER_GIANT: return Lang::STAR_O_SUPER_GIANT;
        case TYPE_STAR_M_HYPER_GIANT: return Lang::STAR_M_HYPER_GIANT;
        case TYPE_STAR_K_HYPER_GIANT: return Lang::STAR_K_HYPER_GIANT;
        case TYPE_STAR_G_HYPER_GIANT: return Lang::STAR_G_HYPER_GIANT;
        case TYPE_STAR_F_HYPER_GIANT: return Lang::STAR_AF_HYPER_GIANT;
        case TYPE_STAR_A_HYPER_GIANT: return Lang::STAR_AF_HYPER_GIANT;
        case TYPE_STAR_B_HYPER_GIANT: return Lang::STAR_B_HYPER_GIANT;
        case TYPE_STAR_O_HYPER_GIANT: return Lang::STAR_O_HYPER_GIANT;
        case TYPE_STAR_M_WF: return Lang::STAR_M_WF;
        case TYPE_STAR_B_WF: return Lang::STAR_B_WF;
        case TYPE_STAR_O_WF: return Lang::STAR_O_WF;
        case TYPE_STAR_S_BH: return Lang::STAR_S_BH;
        case TYPE_STAR_IM_BH: return Lang::STAR_IM_BH;
        case TYPE_STAR_SM_BH: return Lang::STAR_SM_BH;
        case TYPE_PLANET_GAS_GIANT:
            if (m_mass > 800) return Lang::VERY_LARGE_GAS_GIANT;
            if (m_mass > 300) return Lang::LARGE_GAS_GIANT;
            if (m_mass > 80) return Lang::MEDIUM_GAS_GIANT;
            else return Lang::SMALL_GAS_GIANT;
        case TYPE_PLANET_ASTEROID: return Lang::ASTEROID;
        case TYPE_PLANET_TERRESTRIAL: {
            std::string s;
            if (m_mass > fixed(2, 1)) s = Lang::MASSIVE;
            else if (m_mass > fixed(3, 2)) s = Lang::LARGE;
            else if (m_mass < fixed(1, 10)) s = Lang::TINY;
            else if (m_mass < fixed(1, 5)) s = Lang::SMALL;

            if (m_volcanicity > fixed(7, 10)) {
                if (s.size()) s += Lang::COMMA_HIGHLY_VOLCANIC;
                else s = Lang::HIGHLY_VOLCANIC;
            }

            if (m_volatileIces + m_volatileLiquid > fixed(4, 5)) {
                if (m_volatileIces > m_volatileLiquid) {
                    if (m_averageTemp < fixed(250)) {
                        s += Lang::ICE_WORLD;
                    } else s += Lang::ROCKY_PLANET;
                } else {
                    if (m_averageTemp < fixed(250)) {
                        s += Lang::ICE_WORLD;
                    } else {
                        s += Lang::OCEANICWORLD;
                    }
                }
            } else if (m_volatileLiquid > fixed(2, 5)){
                if (m_averageTemp > fixed(250)) {
                    s += Lang::PLANET_CONTAINING_LIQUID_WATER;
                } else {
                    s += Lang::PLANET_WITH_SOME_ICE;
                }
            } else if (m_volatileLiquid > fixed(1, 5)){
                s += Lang::ROCKY_PLANET_CONTAINING_COME_LIQUIDS;
            } else {
                s += Lang::ROCKY_PLANET;
            }

            if (m_volatileGas < fixed(1, 100)) {
                s += Lang::WITH_NO_SIGNIFICANT_ATMOSPHERE;
            } else {
                std::string thickness;
                if (m_volatileGas < fixed(1, 10)) thickness = Lang::TENUOUS;
                else if (m_volatileGas < fixed(1, 5)) thickness = Lang::THIN;
                else if (m_volatileGas < fixed(2, 1)) {} else if (m_volatileGas < fixed(4, 1)) thickness = Lang::THICK;
                else thickness = Lang::VERY_DENSE;

                if (m_atmosOxidizing > fixed(95, 100)) {
                    s += Lang::WITH_A + thickness + Lang::O2_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(7, 10)) {
                    s += Lang::WITH_A + thickness + Lang::CO2_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(65, 100)) {
                    s += Lang::WITH_A + thickness + Lang::CO_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(55, 100)) {
                    s += Lang::WITH_A + thickness + Lang::CH4_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(3, 10)) {
                    s += Lang::WITH_A + thickness + Lang::H_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(2, 10)) {
                    s += Lang::WITH_A + thickness + Lang::HE_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(15, 100)) {
                    s += Lang::WITH_A + thickness + Lang::AR_ATMOSPHERE;
                } else if (m_atmosOxidizing > fixed(1, 10)) {
                    s += Lang::WITH_A + thickness + Lang::S_ATMOSPHERE;
                } else {
                    s += Lang::WITH_A + thickness + Lang::N_ATMOSPHERE;
                }
            }

            if (m_life > fixed(1, 2)) {
                s += Lang::AND_HIGHLY_COMPLEX_ECOSYSTEM;
            } else if (m_life > fixed(1, 10)) {
                s += Lang::AND_INDIGENOUS_PLANT_LIFE;
            } else if (m_life > fixed(0)) {
                s += Lang::AND_INDIGENOUS_MICROBIAL_LIFE;
            } else {
                s += ".";
            }

            return s;
        }
        case TYPE_STARPORT_ORBITAL:
            return Lang::ORBITAL_STARPORT;
        case TYPE_STARPORT_SURFACE:
            return Lang::STARPORT;
        case TYPE_HYPERSPACE_CLOUD:
            return Lang::HYPERSPACE_PERMA_CLOUD;
        case TYPE_GRAVPOINT:
        default:
            Output("Warning: Invalid Astro Body Description found.\n");
            return Lang::UNKNOWN;
    }
}

const char *SystemBody::GetIcon() const
{
    PROFILE_SCOPED()
        switch (m_type) {
        case TYPE_BROWN_DWARF: return "icons/object_brown_dwarf.png";
        case TYPE_WHITE_DWARF: return "icons/object_white_dwarf.png";
        case TYPE_STAR_M: return "icons/object_star_m.png";
        case TYPE_STAR_K: return "icons/object_star_k.png";
        case TYPE_STAR_G: return "icons/object_star_g.png";
        case TYPE_STAR_F: return "icons/object_star_f.png";
        case TYPE_STAR_A: return "icons/object_star_a.png";
        case TYPE_STAR_B: return "icons/object_star_b.png";
        case TYPE_STAR_O: return "icons/object_star_b.png"; //shares B graphic for now
        case TYPE_STAR_M_GIANT: return "icons/object_star_m_giant.png";
        case TYPE_STAR_K_GIANT: return "icons/object_star_k_giant.png";
        case TYPE_STAR_G_GIANT: return "icons/object_star_g_giant.png";
        case TYPE_STAR_F_GIANT: return "icons/object_star_f_giant.png";
        case TYPE_STAR_A_GIANT: return "icons/object_star_a_giant.png";
        case TYPE_STAR_B_GIANT: return "icons/object_star_b_giant.png";
        case TYPE_STAR_O_GIANT: return "icons/object_star_o.png"; // uses old O type graphic
        case TYPE_STAR_M_SUPER_GIANT: return "icons/object_star_m_super_giant.png";
        case TYPE_STAR_K_SUPER_GIANT: return "icons/object_star_k_super_giant.png";
        case TYPE_STAR_G_SUPER_GIANT: return "icons/object_star_g_super_giant.png";
        case TYPE_STAR_F_SUPER_GIANT: return "icons/object_star_g_super_giant.png"; //shares G graphic for now
        case TYPE_STAR_A_SUPER_GIANT: return "icons/object_star_a_super_giant.png";
        case TYPE_STAR_B_SUPER_GIANT: return "icons/object_star_b_super_giant.png";
        case TYPE_STAR_O_SUPER_GIANT: return "icons/object_star_b_super_giant.png";// uses B type graphic for now
        case TYPE_STAR_M_HYPER_GIANT: return "icons/object_star_m_hyper_giant.png";
        case TYPE_STAR_K_HYPER_GIANT: return "icons/object_star_k_hyper_giant.png";
        case TYPE_STAR_G_HYPER_GIANT: return "icons/object_star_g_hyper_giant.png";
        case TYPE_STAR_F_HYPER_GIANT: return "icons/object_star_f_hyper_giant.png";
        case TYPE_STAR_A_HYPER_GIANT: return "icons/object_star_a_hyper_giant.png";
        case TYPE_STAR_B_HYPER_GIANT: return "icons/object_star_b_hyper_giant.png";
        case TYPE_STAR_O_HYPER_GIANT: return "icons/object_star_b_hyper_giant.png";// uses B type graphic for now
        case TYPE_STAR_M_WF: return "icons/object_star_m_wf.png";
        case TYPE_STAR_B_WF: return "icons/object_star_b_wf.png";
        case TYPE_STAR_O_WF: return "icons/object_star_o_wf.png";
        case TYPE_STAR_S_BH: return "icons/object_star_bh.png";
        case TYPE_STAR_IM_BH: return "icons/object_star_smbh.png";
        case TYPE_STAR_SM_BH: return "icons/object_star_smbh.png";
        case TYPE_PLANET_GAS_GIANT:
            if (m_mass > 800) {
                if (m_averageTemp > 1000) return "icons/object_planet_large_gas_giant_hot.png";
                else return "icons/object_planet_large_gas_giant.png";
            }
            if (m_mass > 300) {
                if (m_averageTemp > 1000) return "icons/object_planet_large_gas_giant_hot.png";
                else return "icons/object_planet_large_gas_giant.png";
            }
            if (m_mass > 80) {
                if (m_averageTemp > 1000) return "icons/object_planet_medium_gas_giant_hot.png";
                else return "icons/object_planet_medium_gas_giant.png";
            } else {
                if (m_averageTemp > 1000) return "icons/object_planet_small_gas_giant_hot.png";
                else return "icons/object_planet_small_gas_giant.png";
            }
        case TYPE_PLANET_ASTEROID:
            return "icons/object_planet_asteroid.png";
        case TYPE_PLANET_TERRESTRIAL:
            if (m_volatileLiquid > fixed(7, 10)) {
                if (m_averageTemp > 250) return "icons/object_planet_water.png";
                else return "icons/object_planet_ice.png";
            }
            if ((m_life > fixed(9, 10)) &&
                (m_volatileGas > fixed(6, 10))) return "icons/object_planet_life.png";
            if ((m_life > fixed(8, 10)) &&
                (m_volatileGas > fixed(5, 10))) return "icons/object_planet_life6.png";
            if ((m_life > fixed(7, 10)) &&
                (m_volatileGas > fixed(45, 100))) return "icons/object_planet_life7.png";
            if ((m_life > fixed(6, 10)) &&
                (m_volatileGas > fixed(4, 10))) return "icons/object_planet_life8.png";
            if ((m_life > fixed(5, 10)) &&
                (m_volatileGas > fixed(3, 10))) return "icons/object_planet_life4.png";
            if ((m_life > fixed(4, 10)) &&
                (m_volatileGas > fixed(2, 10))) return "icons/object_planet_life5.png";
            if ((m_life > fixed(1, 10)) &&
                (m_volatileGas > fixed(2, 10))) return "icons/object_planet_life2.png";
            if (m_life > fixed(1, 10)) return "icons/object_planet_life3.png";
            if (m_mass < fixed(1, 100)) return "icons/object_planet_dwarf.png";
            if (m_mass < fixed(1, 10)) return "icons/object_planet_small.png";
            if ((m_volatileLiquid < fixed(1, 10)) &&
                (m_volatileGas > fixed(1, 5))) return "icons/object_planet_desert.png";

            if (m_volatileIces + m_volatileLiquid > fixed(3, 5)) {
                if (m_volatileIces > m_volatileLiquid) {
                    if (m_averageTemp < 250)	return "icons/object_planet_ice.png";
                } else {
                    if (m_averageTemp > 250) {
                        return "icons/object_planet_water.png";
                    } else return "icons/object_planet_ice.png";
                }
            }

            if (m_volatileGas > fixed(1, 2)) {
                if (m_atmosOxidizing < fixed(1, 2)) {
                    if (m_averageTemp > 300) return "icons/object_planet_methane3.png";
                    else if (m_averageTemp > 250) return "icons/object_planet_methane2.png";
                    else return "icons/object_planet_methane.png";
                } else {
                    if (m_averageTemp > 300) return "icons/object_planet_co2_2.png";
                    else if (m_averageTemp > 250) {
                        if ((m_volatileLiquid > fixed(3, 10)) && (m_volatileGas > fixed(2, 10)))
                            return "icons/object_planet_co2_4.png";
                        else return "icons/object_planet_co2_3.png";
                    } else return "icons/object_planet_co2.png";
                }
            }

            if ((m_volatileLiquid > fixed(1, 10)) &&
                (m_volatileGas < fixed(1, 10))) return "icons/object_planet_ice.png";
            if (m_volcanicity > fixed(7, 10)) return "icons/object_planet_volcanic.png";
            return "icons/object_planet_small.png";
            /*
            "icons/object_planet_water_n1.png"
            "icons/object_planet_life3.png"
            "icons/object_planet_life2.png"
            */
        case TYPE_STARPORT_ORBITAL:
            return "icons/object_orbital_starport.png";
        case TYPE_GRAVPOINT:
        case TYPE_STARPORT_SURFACE:
        case TYPE_HYPERSPACE_CLOUD:
        default:
            Output("Warning: Invalid body icon.\n");
            return 0;
    }
}

/*
* Position a surface starport anywhere. Space.cpp::MakeFrameFor() ensures it
* is on dry land (discarding this position if necessary)
*/
void SystemBody::PositionSettlementOnPlanet()
{
    PROFILE_SCOPED()
        Random r(m_seed);
    // used for orientation on planet surface
    double r2 = r.Double(); 	// function parameter evaluation order is implementation-dependent
    double r1 = r.Double();		// can't put two rands in the same expression
    m_orbit.SetPlane(matrix3x3d::RotateZ(2 * M_PI*r1) * matrix3x3d::RotateY(2 * M_PI*r2));

    // store latitude and longitude to equivalent orbital parameters to
    // be accessible easier
    m_inclination = fixed(r1 * 10000, 10000) + FIXED_PI / 2;	// latitide
    m_orbitalOffset = FIXED_PI / 2;							// longitude

}

double SystemBody::GetMaxChildOrbitalDistance() const
{
    PROFILE_SCOPED()
        double max = 0;
    for (unsigned int i = 0; i<m_children.size(); i++) {
        if (m_children[i]->m_orbMax.ToDouble() > max) {
            max = m_children[i]->m_orbMax.ToDouble();
        }
    }
    return AU * max;
}

double SystemBody::CalcSurfaceGravity() const
{
    PROFILE_SCOPED()
        double r = GetRadius();
    if (r > 0.0) {
        return G * GetMass() / pow(r, 2);
    } else {
        return 0.0;
    }
}

bool SystemBody::HasAtmosphere() const
{
    PROFILE_SCOPED()
        return (m_volatileGas > fixed(1, 100));
}

bool SystemBody::IsScoopable() const
{
    PROFILE_SCOPED()
        return (GetSuperType() == SUPERTYPE_GAS_GIANT);
}

void SystemBody::PickAtmosphere()
{
    PROFILE_SCOPED()
        /* Alpha value isn't real alpha. in the shader fog depth is determined
        * by density*alpha, so that we can have very dense atmospheres
        * without having them a big stinking solid color obscuring everything

        These are our atmosphere colours, for terrestrial planets we use m_atmosOxidizing
        for some variation to atmosphere colours
        */
        switch (m_type) {
        case SystemBody::TYPE_PLANET_GAS_GIANT:

            m_atmosColor = Color(64, 64, 64, 3);
            m_atmosDensity = 14.0;
            break;
        case SystemBody::TYPE_PLANET_ASTEROID:
            m_atmosColor = Color(0);
            m_atmosDensity = 0.0;
            break;
        default:
        case SystemBody::TYPE_PLANET_TERRESTRIAL:
            double r = 0, g = 0, b = 0;
            double atmo = m_atmosOxidizing.ToDouble();
            if (m_volatileGas.ToDouble() > 0.001) {
                if (atmo > 0.95) {
                    // o2
                    r = 1.0f + ((0.95f - atmo)*15.0f);
                    g = 0.95f + ((0.95f - atmo)*10.0f);
                    b = atmo*atmo*atmo*atmo*atmo;
                } else if (atmo > 0.7) {
                    // co2
                    r = atmo + 0.05f;
                    g = 1.0f + (0.7f - atmo);
                    b = 0.8f;
                } else if (atmo > 0.65) {
                    // co
                    r = 1.0f + (0.65f - atmo);
                    g = 0.8f;
                    b = atmo + 0.25f;
                } else if (atmo > 0.55) {
                    // ch4
                    r = 1.0f + ((0.55f - atmo)*5.0);
                    g = 0.35f - ((0.55f - atmo)*5.0);
                    b = 0.4f;
                } else if (atmo > 0.3) {
                    // h
                    r = 1.0f;
                    g = 1.0f;
                    b = 1.0f;
                } else if (atmo > 0.2) {
                    // he
                    r = 1.0f;
                    g = 1.0f;
                    b = 1.0f;
                } else if (atmo > 0.15) {
                    // ar
                    r = 0.5f - ((0.15f - atmo)*5.0);
                    g = 0.0f;
                    b = 0.5f + ((0.15f - atmo)*5.0);
                } else if (atmo > 0.1) {
                    // s
                    r = 0.8f - ((0.1f - atmo)*4.0);
                    g = 1.0f;
                    b = 0.5f - ((0.1f - atmo)*10.0);
                } else {
                    // n
                    r = 1.0f;
                    g = 1.0f;
                    b = 1.0f;
                }
                m_atmosColor = Color(r * 255, g * 255, b * 255, 255);
            } else {
                m_atmosColor = Color(0);
            }
            m_atmosDensity = m_volatileGas.ToDouble();
            //Output("| Atmosphere :\n|      red   : [%f] \n|      green : [%f] \n|      blue  : [%f] \n", r, g, b);
            //Output("-------------------------------\n");
            break;
            /*default:
            m_atmosColor = Color(0.6f, 0.6f, 0.6f, 1.0f);
            m_atmosDensity = m_body->m_volatileGas.ToDouble();
            break;*/
    }
}

static const unsigned char RANDOM_RING_COLORS[][4] = {
    {156, 122, 98, 217}, // jupiter-like
    {156, 122, 98, 217}, // saturn-like
    {181, 173, 174, 217}, // neptune-like
    {130, 122, 98, 217}, // uranus-like
    {207, 122, 98, 217}  // brown dwarf-like
};

void SystemBody::PickRings(bool forceRings)
{
    PROFILE_SCOPED()
        m_rings.minRadius = fixed(0);
    m_rings.maxRadius = fixed(0);
    m_rings.baseColor = Color(255, 255, 255, 255);

    if (m_type == SystemBody::TYPE_PLANET_GAS_GIANT) {
        Random ringRng(m_seed + 965467);

        // today's forecast: 50% chance of rings
        double rings_die = ringRng.Double();
        if (forceRings || (rings_die < 0.5)) {
            const unsigned char * const baseCol
                = RANDOM_RING_COLORS[ringRng.Int32(COUNTOF(RANDOM_RING_COLORS))];
            m_rings.baseColor.r = Clamp(baseCol[0] + ringRng.Int32(-20, 20), 0, 255);
            m_rings.baseColor.g = Clamp(baseCol[1] + ringRng.Int32(-20, 20), 0, 255);
            m_rings.baseColor.b = Clamp(baseCol[2] + ringRng.Int32(-20, 10), 0, 255);
            m_rings.baseColor.a = Clamp(baseCol[3] + ringRng.Int32(-5, 5), 0, 255);

            // from wikipedia: http://en.wikipedia.org/wiki/Roche_limit
            // basic Roche limit calculation assuming a rigid satellite
            // d = R (2 p_M / p_m)^{1/3}
            //
            // where R is the radius of the primary, p_M is the density of
            // the primary and p_m is the density of the satellite
            //
            // I assume a satellite density of 500 kg/m^3
            // (which Wikipedia says is an average comet density)
            //
            // also, I can't be bothered to think about unit conversions right now,
            // so I'm going to ignore the real density of the primary and take it as 1100 kg/m^3
            // (note: density of Saturn is ~687, Jupiter ~1,326, Neptune ~1,638, Uranus ~1,318)
            //
            // This gives: d = 1.638642 * R
            fixed innerMin = fixed(110, 100);
            fixed innerMax = fixed(145, 100);
            fixed outerMin = fixed(150, 100);
            fixed outerMax = fixed(168642, 100000);

            m_rings.minRadius = innerMin + (innerMax - innerMin)*ringRng.Fixed();
            m_rings.maxRadius = outerMin + (outerMax - outerMin)*ringRng.Fixed();
        }
    }
}

// Calculate parameters used in the atmospheric model for shaders
SystemBody::AtmosphereParameters SystemBody::CalcAtmosphereParams() const
{
    PROFILE_SCOPED()
        AtmosphereParameters params;

    double atmosDensity;

    GetAtmosphereFlavor(&params.atmosCol, &atmosDensity);
    // adjust global atmosphere opacity
    atmosDensity *= 1e-5;

    params.atmosDensity = static_cast<float>(atmosDensity);

    // Calculate parameters used in the atmospheric model for shaders
    // Isothermal atmospheric model
    // See http://en.wikipedia.org/wiki/Atmospheric_pressure#Altitude_atmospheric_pressure_variation
    // This model features an exponential decrease in pressure and density with altitude.
    // The scale height is 1/the exponential coefficient.

    // The equation for pressure is:
    // Pressure at height h = Pressure surface * e^((-Mg/RT)*h)

    // calculate (inverse) atmosphere scale height
    // The formula for scale height is:
    // h = RT / Mg
    // h is height above the surface in meters
    // R is the universal gas constant
    // T is the surface temperature in Kelvin
    // g is the gravity in m/s^2
    // M is the molar mass of air in kg/mol

    // calculate gravity
    // radius of the planet
    const double radiusPlanet_in_m = (m_radius.ToDouble()*EARTH_RADIUS);
    const double massPlanet_in_kg = (m_mass.ToDouble()*EARTH_MASS);
    const double g = G*massPlanet_in_kg / (radiusPlanet_in_m*radiusPlanet_in_m);

    double T = static_cast<double>(m_averageTemp);

    // XXX hack to avoid issues with sysgen giving 0 temps
    // temporary as part of sysgen needs to be rewritten before the proper fix can be used
    if (T < 1)
        T = 165;

    // We have two kinds of atmosphere: Earth-like and gas giant (hydrogen/helium)
    const double M = m_type == TYPE_PLANET_GAS_GIANT ? 0.0023139903 : 0.02897f; // in kg/mol

    float atmosScaleHeight = static_cast<float>(GAS_CONSTANT_R*T / (M*g));

    // min of 2.0 corresponds to a scale height of 1/20 of the planet's radius,
    params.atmosInvScaleHeight = std::max(20.0f, static_cast<float>(GetRadius() / atmosScaleHeight));
    // integrate atmospheric density between surface and this radius. this is 10x the scale
    // height, which should be a height at which the atmospheric density is negligible
    params.atmosRadius = 1.0f + static_cast<float>(10.0f * atmosScaleHeight) / GetRadius();

    params.planetRadius = static_cast<float>(radiusPlanet_in_m);

    return params;
}

/*
* http://en.wikipedia.org/wiki/Hill_sphere
*/
fixed SystemBody::CalcHillRadius() const
{
    PROFILE_SCOPED()
        if (GetSuperType() <= SUPERTYPE_STAR) {
            return fixed(0);
        } else {
            // playing with precision since these numbers get small
            // masses in earth masses
            fixedf<32> mprimary = m_parent->GetMassInEarths();

            fixedf<48> a = m_semiMajorAxis;
            fixedf<48> e = m_eccentricity;

            return fixed(a * (fixedf<48>(1, 1) - e) *
                fixedf<48>::CubeRootOf(fixedf<48>(
                m_mass / (fixedf<32>(3, 1)*mprimary))));

            //fixed hr = semiMajorAxis*(fixed(1,1) - eccentricity) *
            //  fixedcuberoot(mass / (3*mprimary));
        }
}

/*
* For moons distance from star is not orbMin, orbMax.
*/
const SystemBody *SystemBody::FindStarAndTrueOrbitalRange(fixed &orbMin_, fixed &orbMax_)
{
    PROFILE_SCOPED()
        const SystemBody *planet = this;
    const SystemBody *star = this->m_parent;

    assert(star);

    /* while not found star yet.. */
    while (star->GetSuperType() > SystemBody::SUPERTYPE_STAR) {
        planet = star;
        star = star->m_parent;
    }

    orbMin_ = planet->m_orbMin;
    orbMax_ = planet->m_orbMax;
    return star;
}

void SystemBody::PickPlanetType(Random &rand)
{
    PROFILE_SCOPED()
        fixed albedo = fixed(0);
    fixed greenhouse = fixed(0);

    fixed minDistToStar, maxDistToStar, averageDistToStar;
    const SystemBody *star = FindStarAndTrueOrbitalRange(minDistToStar, maxDistToStar);
    averageDistToStar = (minDistToStar + maxDistToStar) >> 1;

    /* first calculate blackbody temp (no greenhouse effect, zero albedo) */
    int bbody_temp = CalcSurfaceTemp(star, averageDistToStar, albedo, greenhouse);

    m_averageTemp = bbody_temp;

    // radius is just the cube root of the mass. we get some more fractional
    // bits for small bodies otherwise we can easily end up with 0 radius
    // which breaks stuff elsewhere
    if (m_mass <= fixed(1, 1))
        m_radius = fixed(fixedf<48>::CubeRootOf(fixedf<48>(m_mass)));
    else
        m_radius = fixed::CubeRootOf(m_mass);
    // enforce minimum size of 10km
    m_radius = std::max(m_radius, fixed(1, 630));

    // Tidal lock for planets close to their parents:
    //		http://en.wikipedia.org/wiki/Tidal_locking
    //
    //		Formula: time ~ semiMajorAxis^6 * radius / mass / parentMass^2
    //
    //		compared to Earth's Moon
    static fixed MOON_TIDAL_LOCK = fixed(6286, 1);
    fixed invTidalLockTime = fixed(1, 1);

    // fine-tuned not to give overflows, order of evaluation matters!
    if (m_parent->m_type <= TYPE_STAR_MAX) {
        invTidalLockTime /= (m_semiMajorAxis * m_semiMajorAxis);
        invTidalLockTime *= m_mass;
        invTidalLockTime /= (m_semiMajorAxis * m_semiMajorAxis);
        invTidalLockTime *= m_parent->GetMassAsFixed()*m_parent->GetMassAsFixed();
        invTidalLockTime /= m_radius;
        invTidalLockTime /= (m_semiMajorAxis * m_semiMajorAxis)*MOON_TIDAL_LOCK;
    } else {
        invTidalLockTime /= (m_semiMajorAxis * m_semiMajorAxis)*SUN_MASS_TO_EARTH_MASS;
        invTidalLockTime *= m_mass;
        invTidalLockTime /= (m_semiMajorAxis * m_semiMajorAxis)*SUN_MASS_TO_EARTH_MASS;
        invTidalLockTime *= m_parent->GetMassAsFixed()*m_parent->GetMassAsFixed();
        invTidalLockTime /= m_radius;
        invTidalLockTime /= (m_semiMajorAxis * m_semiMajorAxis)*MOON_TIDAL_LOCK;
    }
    //Output("tidal lock of %s: %.5f, a %.5f R %.4f mp %.3f ms %.3f\n", name.c_str(),
    //		invTidalLockTime.ToFloat(), semiMajorAxis.ToFloat(), radius.ToFloat(), parent->mass.ToFloat(), mass.ToFloat());

    if (invTidalLockTime > 10) { // 10x faster than Moon, no chance not to be tidal-locked
        m_rotationPeriod = fixed(int(round(m_orbit.Period())), 3600 * 24);
        m_axialTilt = m_inclination;
    } else if (invTidalLockTime > fixed(1, 100)) { // rotation speed changed in favour of tidal lock
        // XXX: there should be some chance the satellite was captured only recenly and ignore this
        //		I'm ommiting that now, I do not want to change the Universe by additional rand call.

        fixed lambda = invTidalLockTime / (fixed(1, 20) + invTidalLockTime);
        m_rotationPeriod = (1 - lambda)*m_rotationPeriod + lambda*m_orbit.Period() / 3600 / 24;
        m_axialTilt = (1 - lambda)*m_axialTilt + lambda*m_inclination;
    } // else .. nothing happens to the satellite

    if (m_parent->m_type <= TYPE_STAR_MAX)
        // get it from the table now rather than setting it on stars/gravpoints as
        // currently nothing else needs them to have metallicity
        m_metallicity = StarSystem::starMetallicities[m_parent->m_type] * rand.Fixed();
    else
        // this assumes the parent's parent is a star/gravpoint, which is currently always true
        m_metallicity = StarSystem::starMetallicities[m_parent->m_parent->m_type] * rand.Fixed();
    // harder to be volcanic when you are tiny (you cool down)
    m_volcanicity = std::min(fixed(1, 1), m_mass) * rand.Fixed();
    m_atmosOxidizing = rand.Fixed();
    m_life = fixed(0);
    m_volatileGas = fixed(0);
    m_volatileLiquid = fixed(0);
    m_volatileIces = fixed(0);

    // pick body type
    if (m_mass > 317 * 13) {
        // more than 13 jupiter masses can fuse deuterium - is a brown dwarf
        m_type = SystemBody::TYPE_BROWN_DWARF;
        m_averageTemp = m_averageTemp + rand.Int32(starTypeInfo[m_type].tempMin,
            starTypeInfo[m_type].tempMax);
        // prevent mass exceeding 65 jupiter masses or so, when it becomes a star
        // XXX since TYPE_BROWN_DWARF is supertype star, mass is now in
        // solar masses. what a fucking mess
        m_mass = std::min(m_mass, fixed(317 * 65, 1)) / SUN_MASS_TO_EARTH_MASS;
        //Radius is too high as it now uses the planetary calculations to work out radius (Cube root of mass)
        // So tell it to use the star data instead:
        m_radius = fixed(rand.Int32(starTypeInfo[m_type].radius[0],
            starTypeInfo[m_type].radius[1]), 100);
    } else if (m_mass > 6) {
        m_type = SystemBody::TYPE_PLANET_GAS_GIANT;
        m_averageTemp += 334;
        fixed amount_volatiles = fixed(2, 1)*rand.Fixed();
    } else if (m_mass > fixed(1, 15000)) {
        m_type = SystemBody::TYPE_PLANET_TERRESTRIAL;

        fixed amount_volatiles = fixed(2, 1)*rand.Fixed();
        if (rand.Int32(3)) amount_volatiles *= m_mass;
        // total atmosphere loss
        if (rand.Fixed() > m_mass) amount_volatiles = fixed(0);

        //Output("Amount volatiles: %f\n", amount_volatiles.ToFloat());
        // fudge how much of the volatiles are in which state
        greenhouse = fixed(0);
        albedo = fixed(0);
        // CO2 sublimation
        if (m_averageTemp > 195) greenhouse += amount_volatiles * fixed(1, 3);
        else albedo += fixed(2, 6);
        // H2O liquid
        if (m_averageTemp > 273) greenhouse += amount_volatiles * fixed(1, 5);
        else albedo += fixed(3, 6);
        // H2O boils
        if (m_averageTemp > 373) greenhouse += amount_volatiles * fixed(1, 3);

        if (greenhouse > fixed(7, 10)) { // never reach 1, but 1/(1-greenhouse) still grows
            greenhouse *= greenhouse;
            greenhouse *= greenhouse;
            greenhouse = greenhouse / (greenhouse + fixed(32, 311));
        }

        m_averageTemp = CalcSurfaceTemp(star, averageDistToStar, albedo, greenhouse);

        const fixed proportion_gas = m_averageTemp / (fixed(100, 1) + m_averageTemp);
        m_volatileGas = proportion_gas * amount_volatiles;

        const fixed proportion_liquid = (fixed(1, 1) - proportion_gas) * (m_averageTemp / (fixed(50, 1) + m_averageTemp));
        m_volatileLiquid = proportion_liquid * amount_volatiles;

        const fixed proportion_ices = fixed(1, 1) - (proportion_gas + proportion_liquid);
        m_volatileIces = proportion_ices * amount_volatiles;

        //Output("temp %dK, gas:liquid:ices %f:%f:%f\n", averageTemp, proportion_gas.ToFloat(),
        //		proportion_liquid.ToFloat(), proportion_ices.ToFloat());

        if ((m_volatileLiquid > fixed(0)) &&
            (m_averageTemp > CELSIUS - 60) &&
            (m_averageTemp < CELSIUS + 200)) {
            // try for life
            int minTemp = CalcSurfaceTemp(star, maxDistToStar, albedo, greenhouse);
            int maxTemp = CalcSurfaceTemp(star, minDistToStar, albedo, greenhouse);

            if ((star->m_type != TYPE_BROWN_DWARF) &&
                (star->m_type != TYPE_WHITE_DWARF) &&
                (star->m_type != TYPE_STAR_O) &&
                (minTemp > CELSIUS - 10) && (minTemp < CELSIUS + 90) &&
                (maxTemp > CELSIUS - 10) && (maxTemp < CELSIUS + 90)) {
                m_life = rand.Fixed();
            }
        }
    } else {
        m_type = SystemBody::TYPE_PLANET_ASTEROID;
    }

    PickAtmosphere();
    PickRings();
}

/*
* Set natural resources, tech level, industry strengths and population levels
*/
void SystemBody::PopulateStage1(StarSystem *system, fixed &outTotalPop)
{
    PROFILE_SCOPED()
        for (unsigned int i = 0; i<m_children.size(); i++) {
            m_children[i]->PopulateStage1(system, outTotalPop);
        }

    // unexplored systems have no population (that we know about)
    if (system->m_unexplored) {
        m_population = outTotalPop = fixed(0);
        return;
    }

    // grav-points have no population themselves
    if (m_type == SystemBody::TYPE_GRAVPOINT) {
        m_population = fixed(0);
        return;
    }

    Uint32 _init[6] = {system->m_path.systemIndex, Uint32(system->m_path.sectorX),
        Uint32(system->m_path.sectorY), Uint32(system->m_path.sectorZ), UNIVERSE_SEED, Uint32(this->m_seed)};

    Random rand;
    rand.seed(_init, 6);

    RefCountedPtr<Random> namerand(new Random);
    namerand->seed(_init, 6);

    m_population = fixed(0);

    /* Bad type of planet for settlement */
    if ((m_averageTemp > CELSIUS + 100) || (m_averageTemp < 100) ||
        (m_type != SystemBody::TYPE_PLANET_TERRESTRIAL && m_type != SystemBody::TYPE_PLANET_ASTEROID)) {

        // orbital starports should carry a small amount of population
        if (m_type == SystemBody::TYPE_STARPORT_ORBITAL) {
            m_population = fixed(1, 100000);
            outTotalPop += m_population;
        }

        return;
    }

    m_agricultural = fixed(0);

    if (m_life > fixed(9, 10)) {
        m_agricultural = Clamp(fixed(1, 1) - fixed(CELSIUS + 25 - m_averageTemp, 40), fixed(0), fixed(1, 1));
        system->m_agricultural += 2 * m_agricultural;
    } else if (m_life > fixed(1, 2)) {
        m_agricultural = Clamp(fixed(1, 1) - fixed(CELSIUS + 30 - m_averageTemp, 50), fixed(0), fixed(1, 1));
        system->m_agricultural += 1 * m_agricultural;
    } else {
        // don't bother populating crap planets
        if (m_metallicity < fixed(5, 10) &&
            m_metallicity < (fixed(1, 1) - system->m_humanProx)) return;
    }

    const int NUM_CONSUMABLES = 10;
    const Equip::Type consumables[NUM_CONSUMABLES] = {
        Equip::AIR_PROCESSORS,
        Equip::GRAIN,
        Equip::FRUIT_AND_VEG,
        Equip::ANIMAL_MEAT,
        Equip::LIQUOR,
        Equip::CONSUMER_GOODS,
        Equip::MEDICINES,
        Equip::HAND_WEAPONS,
        Equip::NARCOTICS,
        Equip::LIQUID_OXYGEN
    };

    /* Commodities we produce (mining and agriculture) */
    for (int i = Equip::FIRST_COMMODITY; i<Equip::LAST_COMMODITY; i++) {
        Equip::Type t = Equip::Type(i);
        const EquipType &itype = Equip::types[t];

        fixed affinity = fixed(1, 1);
        if (itype.econType & ECON_AGRICULTURE) {
            affinity *= 2 * m_agricultural;
        }
        if (itype.econType & ECON_INDUSTRY) affinity *= system->m_industrial;
        // make industry after we see if agriculture and mining are viable
        if (itype.econType & ECON_MINING) {
            affinity *= m_metallicity;
        }
        affinity *= rand.Fixed();
        // producing consumables is wise
        for (int j = 0; j<NUM_CONSUMABLES; j++) {
            if (i == consumables[j]) {
                affinity *= 2;
                break;
            }
        }
        assert(affinity >= 0);
        /* workforce... */
        m_population += affinity * system->m_humanProx;

        int howmuch = (affinity * 256).ToInt32();

        system->m_tradeLevel[t] += -2 * howmuch;
        for (int j = 0; j<EQUIP_INPUTS; j++) {
            if (!itype.inputs[j]) continue;
            system->m_tradeLevel[itype.inputs[j]] += howmuch;
        }
    }

    if (!system->m_hasCustomBodies && m_population > 0)
        m_name = Pi::luaNameGen->BodyName(this, namerand);

    // Add a bunch of things people consume
    for (int i = 0; i<NUM_CONSUMABLES; i++) {
        Equip::Type t = consumables[i];
        if (m_life > fixed(1, 2)) {
            // life planets can make this jizz probably
            if ((t == Equip::AIR_PROCESSORS) ||
                (t == Equip::LIQUID_OXYGEN) ||
                (t == Equip::GRAIN) ||
                (t == Equip::FRUIT_AND_VEG) ||
                (t == Equip::ANIMAL_MEAT)) {
                continue;
            }
        }
        system->m_tradeLevel[t] += rand.Int32(32, 128);
    }
    // well, outdoor worlds should have way more people
    m_population = fixed(1, 10)*m_population + m_population*m_agricultural;

    //	Output("%s: pop %.3f billion\n", name.c_str(), m_population.ToFloat());

    outTotalPop += m_population;
}

void SystemBody::PopulateAddStations(StarSystem *system)
{
    PROFILE_SCOPED()

    for (unsigned int i = 0; i < m_children.size(); i++) {
        m_children[i]->PopulateAddStations(system);
    }

    Uint32 _init[6] = {system->m_path.systemIndex, Uint32(system->m_path.sectorX),
        Uint32(system->m_path.sectorY), Uint32(system->m_path.sectorZ), this->m_seed, UNIVERSE_SEED};

    Random rand;
    rand.seed(_init, 6);

    RefCountedPtr<Random> namerand(new Random);
    namerand->seed(_init, 6);

    if (m_population < fixed(1, 1000)) return;

    fixed pop = m_population + rand.Fixed();

    fixed orbMaxS = fixed(1, 4)*this->CalcHillRadius();
    fixed orbMinS = 4 * this->m_radius * AU_EARTH_RADIUS;

    if (m_children.size()) { 
        orbMaxS = std::min(orbMaxS, fixed(1, 2) * m_children[0]->m_orbMin);
    }

    // starports - orbital
    pop -= rand.Fixed();
    if ((orbMinS < orbMaxS) && (pop >= 0)) {

        SystemBody *sp = system->NewBody();
        sp->m_type = SystemBody::TYPE_STARPORT_ORBITAL;
        sp->m_seed = rand.Int32();
        sp->m_parent = this;
        sp->m_rotationPeriod = fixed(1, 3600);
        sp->m_averageTemp = this->m_averageTemp;
        sp->m_mass = 0;
        /* just always plonk starports in near orbit */
        sp->m_semiMajorAxis = orbMinS;
        sp->m_eccentricity = fixed(0);
        sp->m_axialTilt = fixed(0);

        sp->m_orbit.SetShapeAroundPrimary(sp->m_semiMajorAxis.ToDouble() * AU, 
            this->GetMassAsFixed().ToDouble() * EARTH_MASS, 0.0);
        sp->m_orbit.SetPlane(matrix3x3d::Identity());

        sp->m_inclination = fixed(0);
        m_children.insert(m_children.begin(), sp);
        system->m_spaceStations.push_back(sp);
        sp->m_orbMin = sp->m_semiMajorAxis;
        sp->m_orbMax = sp->m_semiMajorAxis;

        sp->m_name = gen_unique_station_name(sp, system, namerand);

        pop -= rand.Fixed();
        if (pop > 0) {
            SystemBody *sp2 = system->NewBody();
            sp2->m_type = sp->m_type;
            sp2->m_seed = sp->m_seed;
            sp2->m_parent = sp->m_parent;
            sp2->m_rotationPeriod = sp->m_rotationPeriod;
            sp2->m_averageTemp = sp->m_averageTemp;
            sp2->m_mass = sp->m_mass;
            sp2->m_semiMajorAxis = sp->m_semiMajorAxis;
            sp2->m_eccentricity = sp->m_eccentricity;
            sp2->m_axialTilt = sp->m_axialTilt;

            sp2->m_orbit = sp->m_orbit;
            sp2->m_orbit.SetPlane(matrix3x3d::RotateZ(M_PI));

            sp2->m_inclination = sp->m_inclination;
            sp2->m_orbMin = sp->m_orbMin;
            sp2->m_orbMax = sp->m_orbMax;

            sp2->m_name = gen_unique_station_name(sp, system, namerand);
            m_children.insert(m_children.begin(), sp2);
            system->m_spaceStations.push_back(sp2);
        }
    }
    // starports - surface
    pop = m_population + rand.Fixed();
    int max = 6;
    while (max-- > 0) {
        pop -= rand.Fixed();
        if (pop < 0) break;

        SystemBody *sp = system->NewBody();
        sp->m_type = SystemBody::TYPE_STARPORT_SURFACE;
        sp->m_seed = rand.Int32();
        sp->m_parent = this;
        sp->m_averageTemp = this->m_averageTemp;
        sp->m_mass = 0;
        sp->m_name = gen_unique_station_name(sp, system, namerand);
        memset(&sp->m_orbit, 0, sizeof(Orbit));
        sp->PositionSettlementOnPlanet();
        m_children.insert(m_children.begin(), sp);
        system->m_spaceStations.push_back(sp);
    }
}

void SystemBody::ClearParentAndChildPointers()
{
    PROFILE_SCOPED()
        for (std::vector<SystemBody*>::iterator i = m_children.begin(); i != m_children.end(); ++i)
            (*i)->ClearParentAndChildPointers();
    m_parent = 0;
    m_children.clear();
}

void SystemBody::RemoveChild(SystemBody* child)
{
    auto child_it = std::find(m_children.begin(), m_children.end(), child);
    m_children.erase(child_it);
}


