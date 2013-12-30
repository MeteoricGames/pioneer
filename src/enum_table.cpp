/* Copyright © 2008-2013 Pioneer Developers. See AUTHORS.txt for details */
/* Licensed under the terms of the GPL v3. See licenses/GPL-3.txt        */

/* THIS FILE IS AUTO-GENERATED, CHANGES WILL BE OVERWRITTEN */
/* enum table generated by scan_enums.py */

#include "enum_table.h"
#include "EquipType.h"
#include "LuaEngine.h"
#include "LuaFileSystem.h"
#include "Polit.h"
#include "Ship.h"
#include "ShipType.h"
#include "galaxy/StarSystem.h"
#include "gameui/Face.h"
#include "ui/Align.h"
#include "ui/Event.h"
#include "ui/Expand.h"
#include "ui/Gradient.h"
#include "ui/Margin.h"
#include "ui/NumberLabel.h"
#include "ui/Table.h"
#include "ui/Widget.h"

const struct EnumItem ENUM_EquipSlot[] = {
	{ "CARGO", Equip::SLOT_CARGO },
	{ "ENGINE", Equip::SLOT_ENGINE },
	{ "LASER", Equip::SLOT_LASER },
	{ "MISSILE", Equip::SLOT_MISSILE },
	{ "ECM", Equip::SLOT_ECM },
	{ "SCANNER", Equip::SLOT_SCANNER },
	{ "RADARMAPPER", Equip::SLOT_RADARMAPPER },
	{ "HYPERCLOUD", Equip::SLOT_HYPERCLOUD },
	{ "HULLAUTOREPAIR", Equip::SLOT_HULLAUTOREPAIR },
	{ "ENERGYBOOSTER", Equip::SLOT_ENERGYBOOSTER },
	{ "CABIN", Equip::SLOT_CABIN },
	{ "SHIELD", Equip::SLOT_SHIELD },
	{ "ATMOSHIELD", Equip::SLOT_ATMOSHIELD },
	{ "FUELSCOOP", Equip::SLOT_FUELSCOOP },
	{ "CARGOSCOOP", Equip::SLOT_CARGOSCOOP },
	{ "LASERCOOLER", Equip::SLOT_LASERCOOLER },
	{ "CARGOLIFESUPPORT", Equip::SLOT_CARGOLIFESUPPORT },
	{ "AUTOPILOT", Equip::SLOT_AUTOPILOT },
	{ 0, 0 },
};

const struct EnumItem ENUM_EquipType[] = {
	{ "NONE", Equip::NONE },
	{ "HYDROGEN", Equip::HYDROGEN },
	{ "LIQUID_OXYGEN", Equip::LIQUID_OXYGEN },
	{ "METAL_ORE", Equip::METAL_ORE },
	{ "CARBON_ORE", Equip::CARBON_ORE },
	{ "METAL_ALLOYS", Equip::METAL_ALLOYS },
	{ "PLASTICS", Equip::PLASTICS },
	{ "FRUIT_AND_VEG", Equip::FRUIT_AND_VEG },
	{ "ANIMAL_MEAT", Equip::ANIMAL_MEAT },
	{ "LIVE_ANIMALS", Equip::LIVE_ANIMALS },
	{ "LIQUOR", Equip::LIQUOR },
	{ "GRAIN", Equip::GRAIN },
	{ "TEXTILES", Equip::TEXTILES },
	{ "FERTILIZER", Equip::FERTILIZER },
	{ "WATER", Equip::WATER },
	{ "MEDICINES", Equip::MEDICINES },
	{ "CONSUMER_GOODS", Equip::CONSUMER_GOODS },
	{ "COMPUTERS", Equip::COMPUTERS },
	{ "ROBOTS", Equip::ROBOTS },
	{ "PRECIOUS_METALS", Equip::PRECIOUS_METALS },
	{ "INDUSTRIAL_MACHINERY", Equip::INDUSTRIAL_MACHINERY },
	{ "FARM_MACHINERY", Equip::FARM_MACHINERY },
	{ "MINING_MACHINERY", Equip::MINING_MACHINERY },
	{ "AIR_PROCESSORS", Equip::AIR_PROCESSORS },
	{ "SLAVES", Equip::SLAVES },
	{ "HAND_WEAPONS", Equip::HAND_WEAPONS },
	{ "BATTLE_WEAPONS", Equip::BATTLE_WEAPONS },
	{ "NERVE_GAS", Equip::NERVE_GAS },
	{ "NARCOTICS", Equip::NARCOTICS },
	{ "MILITARY_FUEL", Equip::MILITARY_FUEL },
	{ "RUBBISH", Equip::RUBBISH },
	{ "RADIOACTIVES", Equip::RADIOACTIVES },
	{ "MISSILE_UNGUIDED", Equip::MISSILE_UNGUIDED },
	{ "MISSILE_GUIDED", Equip::MISSILE_GUIDED },
	{ "MISSILE_SMART", Equip::MISSILE_SMART },
	{ "MISSILE_NAVAL", Equip::MISSILE_NAVAL },
	{ "ATMOSPHERIC_SHIELDING", Equip::ATMOSPHERIC_SHIELDING },
	{ "ECM_BASIC", Equip::ECM_BASIC },
	{ "SCANNER", Equip::SCANNER },
	{ "ECM_ADVANCED", Equip::ECM_ADVANCED },
	{ "UNOCCUPIED_CABIN", Equip::UNOCCUPIED_CABIN },
	{ "PASSENGER_CABIN", Equip::PASSENGER_CABIN },
	{ "SHIELD_GENERATOR", Equip::SHIELD_GENERATOR },
	{ "LASER_COOLING_BOOSTER", Equip::LASER_COOLING_BOOSTER },
	{ "CARGO_LIFE_SUPPORT", Equip::CARGO_LIFE_SUPPORT },
	{ "AUTOPILOT", Equip::AUTOPILOT },
	{ "RADAR_MAPPER", Equip::RADAR_MAPPER },
	{ "FUEL_SCOOP", Equip::FUEL_SCOOP },
	{ "CARGO_SCOOP", Equip::CARGO_SCOOP },
	{ "HYPERCLOUD_ANALYZER", Equip::HYPERCLOUD_ANALYZER },
	{ "HULL_AUTOREPAIR", Equip::HULL_AUTOREPAIR },
	{ "SHIELD_ENERGY_BOOSTER", Equip::SHIELD_ENERGY_BOOSTER },
	{ "DRIVE_CLASS1", Equip::DRIVE_CLASS1 },
	{ "DRIVE_CLASS2", Equip::DRIVE_CLASS2 },
	{ "DRIVE_CLASS3", Equip::DRIVE_CLASS3 },
	{ "DRIVE_CLASS4", Equip::DRIVE_CLASS4 },
	{ "DRIVE_CLASS5", Equip::DRIVE_CLASS5 },
	{ "DRIVE_CLASS6", Equip::DRIVE_CLASS6 },
	{ "DRIVE_CLASS7", Equip::DRIVE_CLASS7 },
	{ "DRIVE_CLASS8", Equip::DRIVE_CLASS8 },
	{ "DRIVE_CLASS9", Equip::DRIVE_CLASS9 },
	{ "DRIVE_CLASS10", Equip::DRIVE_CLASS10 },
	{ "DRIVE_CLASS11", Equip::DRIVE_CLASS11 },
	{ "DRIVE_CLASS12", Equip::DRIVE_CLASS12 },
	{ "DRIVE_CLASS13", Equip::DRIVE_CLASS13 },
	{ "DRIVE_CLASS14", Equip::DRIVE_CLASS14 },
	{ "DRIVE_CLASS15", Equip::DRIVE_CLASS15 },
	{ "DRIVE_CLASS16", Equip::DRIVE_CLASS16 },
	{ "DRIVE_CLASS17", Equip::DRIVE_CLASS17 },
	{ "DRIVE_CLASS18", Equip::DRIVE_CLASS18 },
	{ "DRIVE_CLASS19", Equip::DRIVE_CLASS19 },
	{ "DRIVE_CLASS20", Equip::DRIVE_CLASS20 },
	{ "DRIVE_MIL1", Equip::DRIVE_MIL1 },
	{ "DRIVE_MIL2", Equip::DRIVE_MIL2 },
	{ "DRIVE_MIL3", Equip::DRIVE_MIL3 },
	{ "DRIVE_MIL4", Equip::DRIVE_MIL4 },
	{ "PULSECANNON_1MW", Equip::PULSECANNON_1MW },
	{ "PULSECANNON_DUAL_1MW", Equip::PULSECANNON_DUAL_1MW },
	{ "PULSECANNON_2MW", Equip::PULSECANNON_2MW },
	{ "PULSECANNON_RAPID_2MW", Equip::PULSECANNON_RAPID_2MW },
	{ "PULSECANNON_4MW", Equip::PULSECANNON_4MW },
	{ "PULSECANNON_10MW", Equip::PULSECANNON_10MW },
	{ "PULSECANNON_20MW", Equip::PULSECANNON_20MW },
	{ "MININGCANNON_17MW", Equip::MININGCANNON_17MW },
	{ "SMALL_PLASMA_ACCEL", Equip::SMALL_PLASMA_ACCEL },
	{ "LARGE_PLASMA_ACCEL", Equip::LARGE_PLASMA_ACCEL },
	{ 0, 0 },
};

const struct EnumItem ENUM_DetailLevel[] = {
	{ "VERY_LOW", LuaEngine::DETAIL_VERY_LOW },
	{ "LOW", LuaEngine::DETAIL_LOW },
	{ "MEDIUM", LuaEngine::DETAIL_MEDIUM },
	{ "HIGH", LuaEngine::DETAIL_HIGH },
	{ "VERY_HIGH", LuaEngine::DETAIL_VERY_HIGH },
	{ 0, 0 },
};

const struct EnumItem ENUM_FileSystemRoot[] = {
	{ "USER", LuaFileSystem::ROOT_USER },
	{ "DATA", LuaFileSystem::ROOT_DATA },
	{ 0, 0 },
};

const struct EnumItem ENUM_PolitCrime[] = {
	{ "TRADING_ILLEGAL_GOODS", Polit::CRIME_TRADING_ILLEGAL_GOODS },
	{ "WEAPON_DISCHARGE", Polit::CRIME_WEAPON_DISCHARGE },
	{ "PIRACY", Polit::CRIME_PIRACY },
	{ "MURDER", Polit::CRIME_MURDER },
	{ 0, 0 },
};

const struct EnumItem ENUM_PolitEcon[] = {
	{ "NONE", Polit::ECON_NONE },
	{ "VERY_CAPITALIST", Polit::ECON_VERY_CAPITALIST },
	{ "CAPITALIST", Polit::ECON_CAPITALIST },
	{ "MIXED", Polit::ECON_MIXED },
	{ "PLANNED", Polit::ECON_PLANNED },
	{ 0, 0 },
};

const struct EnumItem ENUM_PolitGovType[] = {
	{ "NONE", Polit::GOV_NONE },
	{ "EARTHCOLONIAL", Polit::GOV_EARTHCOLONIAL },
	{ "EARTHDEMOC", Polit::GOV_EARTHDEMOC },
	{ "EMPIRERULE", Polit::GOV_EMPIRERULE },
	{ "CISLIBDEM", Polit::GOV_CISLIBDEM },
	{ "CISSOCDEM", Polit::GOV_CISSOCDEM },
	{ "LIBDEM", Polit::GOV_LIBDEM },
	{ "CORPORATE", Polit::GOV_CORPORATE },
	{ "SOCDEM", Polit::GOV_SOCDEM },
	{ "EARTHMILDICT", Polit::GOV_EARTHMILDICT },
	{ "MILDICT1", Polit::GOV_MILDICT1 },
	{ "MILDICT2", Polit::GOV_MILDICT2 },
	{ "EMPIREMILDICT", Polit::GOV_EMPIREMILDICT },
	{ "COMMUNIST", Polit::GOV_COMMUNIST },
	{ "PLUTOCRATIC", Polit::GOV_PLUTOCRATIC },
	{ "DISORDER", Polit::GOV_DISORDER },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipFlightState[] = {
	{ "FLYING", Ship::FLYING },
	{ "DOCKING", Ship::DOCKING },
	{ "DOCKED", Ship::DOCKED },
	{ "LANDED", Ship::LANDED },
	{ "HYPERSPACE", Ship::HYPERSPACE },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipJumpStatus[] = {
	{ "OK", Ship::HYPERJUMP_OK },
	{ "CURRENT_SYSTEM", Ship::HYPERJUMP_CURRENT_SYSTEM },
	{ "NO_DRIVE", Ship::HYPERJUMP_NO_DRIVE },
	{ "DRIVE_ACTIVE", Ship::HYPERJUMP_DRIVE_ACTIVE },
	{ "OUT_OF_RANGE", Ship::HYPERJUMP_OUT_OF_RANGE },
	{ "INSUFFICIENT_FUEL", Ship::HYPERJUMP_INSUFFICIENT_FUEL },
	{ "SAFETY_LOCKOUT", Ship::HYPERJUMP_SAFETY_LOCKOUT },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipAlertStatus[] = {
	{ "NONE", Ship::ALERT_NONE },
	{ "SHIP_NEARBY", Ship::ALERT_SHIP_NEARBY },
	{ "SHIP_FIRING", Ship::ALERT_SHIP_FIRING },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipAIError[] = {
	{ "NONE", Ship::AIERROR_NONE },
	{ "GRAV_TOO_HIGH", Ship::AIERROR_GRAV_TOO_HIGH },
	{ "REFUSED_PERM", Ship::AIERROR_REFUSED_PERM },
	{ "ORBIT_IMPOSSIBLE", Ship::AIERROR_ORBIT_IMPOSSIBLE },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipFuelStatus[] = {
	{ "OK", Ship::FUEL_OK },
	{ "WARNING", Ship::FUEL_WARNING },
	{ "EMPTY", Ship::FUEL_EMPTY },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipTypeThruster[] = {
	{ "REVERSE", ShipType::THRUSTER_REVERSE },
	{ "FORWARD", ShipType::THRUSTER_FORWARD },
	{ "UP", ShipType::THRUSTER_UP },
	{ "DOWN", ShipType::THRUSTER_DOWN },
	{ "LEFT", ShipType::THRUSTER_LEFT },
	{ "RIGHT", ShipType::THRUSTER_RIGHT },
	{ 0, 0 },
};

const struct EnumItem ENUM_DualLaserOrientation[] = {
	{ "HORIZONTAL", ShipType::DUAL_LASERS_HORIZONTAL },
	{ "VERTICAL", ShipType::DUAL_LASERS_VERTICAL },
	{ 0, 0 },
};

const struct EnumItem ENUM_ShipTypeTag[] = {
	{ "NONE", ShipType::TAG_NONE },
	{ "SHIP", ShipType::TAG_SHIP },
	{ "STATIC_SHIP", ShipType::TAG_STATIC_SHIP },
	{ "WRECK_SHIP", ShipType::TAG_WRECK_SHIP },
	{ "WEAPON_SHIP", ShipType::TAG_WEAPON_SHIP },
	{ "NPC_SHIP", ShipType::TAG_NPC_SHIP },
	{ "MISSILE", ShipType::TAG_MISSILE },
	{ 0, 0 },
};

const struct EnumItem ENUM_EconType[] = {
	{ "MINING", ECON_MINING },
	{ "AGRICULTURE", ECON_AGRICULTURE },
	{ "INDUSTRY", ECON_INDUSTRY },
	{ 0, 0 },
};

const struct EnumItem ENUM_BodyType[] = {
	{ "GRAVPOINT", SystemBody::TYPE_GRAVPOINT },
	{ "BROWN_DWARF", SystemBody::TYPE_BROWN_DWARF },
	{ "WHITE_DWARF", SystemBody::TYPE_WHITE_DWARF },
	{ "STAR_M", SystemBody::TYPE_STAR_M },
	{ "STAR_K", SystemBody::TYPE_STAR_K },
	{ "STAR_G", SystemBody::TYPE_STAR_G },
	{ "STAR_F", SystemBody::TYPE_STAR_F },
	{ "STAR_A", SystemBody::TYPE_STAR_A },
	{ "STAR_B", SystemBody::TYPE_STAR_B },
	{ "STAR_O", SystemBody::TYPE_STAR_O },
	{ "STAR_M_GIANT", SystemBody::TYPE_STAR_M_GIANT },
	{ "STAR_K_GIANT", SystemBody::TYPE_STAR_K_GIANT },
	{ "STAR_G_GIANT", SystemBody::TYPE_STAR_G_GIANT },
	{ "STAR_F_GIANT", SystemBody::TYPE_STAR_F_GIANT },
	{ "STAR_A_GIANT", SystemBody::TYPE_STAR_A_GIANT },
	{ "STAR_B_GIANT", SystemBody::TYPE_STAR_B_GIANT },
	{ "STAR_O_GIANT", SystemBody::TYPE_STAR_O_GIANT },
	{ "STAR_M_SUPER_GIANT", SystemBody::TYPE_STAR_M_SUPER_GIANT },
	{ "STAR_K_SUPER_GIANT", SystemBody::TYPE_STAR_K_SUPER_GIANT },
	{ "STAR_G_SUPER_GIANT", SystemBody::TYPE_STAR_G_SUPER_GIANT },
	{ "STAR_F_SUPER_GIANT", SystemBody::TYPE_STAR_F_SUPER_GIANT },
	{ "STAR_A_SUPER_GIANT", SystemBody::TYPE_STAR_A_SUPER_GIANT },
	{ "STAR_B_SUPER_GIANT", SystemBody::TYPE_STAR_B_SUPER_GIANT },
	{ "STAR_O_SUPER_GIANT", SystemBody::TYPE_STAR_O_SUPER_GIANT },
	{ "STAR_M_HYPER_GIANT", SystemBody::TYPE_STAR_M_HYPER_GIANT },
	{ "STAR_K_HYPER_GIANT", SystemBody::TYPE_STAR_K_HYPER_GIANT },
	{ "STAR_G_HYPER_GIANT", SystemBody::TYPE_STAR_G_HYPER_GIANT },
	{ "STAR_F_HYPER_GIANT", SystemBody::TYPE_STAR_F_HYPER_GIANT },
	{ "STAR_A_HYPER_GIANT", SystemBody::TYPE_STAR_A_HYPER_GIANT },
	{ "STAR_B_HYPER_GIANT", SystemBody::TYPE_STAR_B_HYPER_GIANT },
	{ "STAR_O_HYPER_GIANT", SystemBody::TYPE_STAR_O_HYPER_GIANT },
	{ "STAR_M_WF", SystemBody::TYPE_STAR_M_WF },
	{ "STAR_B_WF", SystemBody::TYPE_STAR_B_WF },
	{ "STAR_O_WF", SystemBody::TYPE_STAR_O_WF },
	{ "STAR_S_BH", SystemBody::TYPE_STAR_S_BH },
	{ "STAR_IM_BH", SystemBody::TYPE_STAR_IM_BH },
	{ "STAR_SM_BH", SystemBody::TYPE_STAR_SM_BH },
	{ "PLANET_GAS_GIANT", SystemBody::TYPE_PLANET_GAS_GIANT },
	{ "PLANET_ASTEROID", SystemBody::TYPE_PLANET_ASTEROID },
	{ "PLANET_TERRESTRIAL", SystemBody::TYPE_PLANET_TERRESTRIAL },
	{ "STARPORT_ORBITAL", SystemBody::TYPE_STARPORT_ORBITAL },
	{ "STARPORT_SURFACE", SystemBody::TYPE_STARPORT_SURFACE },
	{ 0, 0 },
};

const struct EnumItem ENUM_BodySuperType[] = {
	{ "NONE", SystemBody::SUPERTYPE_NONE },
	{ "STAR", SystemBody::SUPERTYPE_STAR },
	{ "ROCKY_PLANET", SystemBody::SUPERTYPE_ROCKY_PLANET },
	{ "GAS_GIANT", SystemBody::SUPERTYPE_GAS_GIANT },
	{ "STARPORT", SystemBody::SUPERTYPE_STARPORT },
	{ 0, 0 },
};

const struct EnumItem ENUM_GameUIFaceFlags[] = {
	{ "RAND", GameUI::Face::RAND },
	{ "MALE", GameUI::Face::MALE },
	{ "FEMALE", GameUI::Face::FEMALE },
	{ "ARMOUR", GameUI::Face::ARMOUR },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIAlignDirection[] = {
	{ "TOP_LEFT", UI::Align::TOP_LEFT },
	{ "TOP", UI::Align::TOP },
	{ "TOP_RIGHT", UI::Align::TOP_RIGHT },
	{ "LEFT", UI::Align::LEFT },
	{ "MIDDLE", UI::Align::MIDDLE },
	{ "RIGHT", UI::Align::RIGHT },
	{ "BOTTOM_LEFT", UI::Align::BOTTOM_LEFT },
	{ "BOTTOM", UI::Align::BOTTOM },
	{ "BOTTOM_RIGHT", UI::Align::BOTTOM_RIGHT },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIEventType[] = {
	{ "KEYBOARD", UI::Event::KEYBOARD },
	{ "TEXT_INPUT", UI::Event::TEXT_INPUT },
	{ "MOUSE_BUTTON", UI::Event::MOUSE_BUTTON },
	{ "MOUSE_MOTION", UI::Event::MOUSE_MOTION },
	{ "MOUSE_WHEEL", UI::Event::MOUSE_WHEEL },
	{ "JOYSTICK_AXIS_MOTION", UI::Event::JOYSTICK_AXIS_MOTION },
	{ "JOYSTICK_HAT_MOTION", UI::Event::JOYSTICK_HAT_MOTION },
	{ "JOYSTICK_BUTTON", UI::Event::JOYSTICK_BUTTON },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIKeyboardAction[] = {
	{ "DOWN", UI::KeyboardEvent::KEY_DOWN },
	{ "UP", UI::KeyboardEvent::KEY_UP },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIMouseButtonAction[] = {
	{ "DOWN", UI::MouseButtonEvent::BUTTON_DOWN },
	{ "UP", UI::MouseButtonEvent::BUTTON_UP },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIMouseButtonType[] = {
	{ "LEFT", UI::MouseButtonEvent::BUTTON_LEFT },
	{ "MIDDLE", UI::MouseButtonEvent::BUTTON_MIDDLE },
	{ "RIGHT", UI::MouseButtonEvent::BUTTON_RIGHT },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIMouseWheelDirection[] = {
	{ "UP", UI::MouseWheelEvent::WHEEL_UP },
	{ "DOWN", UI::MouseWheelEvent::WHEEL_DOWN },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIJoystickHatDirection[] = {
	{ "CENTRE", UI::JoystickHatMotionEvent::HAT_CENTRE },
	{ "UP", UI::JoystickHatMotionEvent::HAT_UP },
	{ "RIGHT", UI::JoystickHatMotionEvent::HAT_RIGHT },
	{ "DOWN", UI::JoystickHatMotionEvent::HAT_DOWN },
	{ "LEFT", UI::JoystickHatMotionEvent::HAT_LEFT },
	{ "RIGHTUP", UI::JoystickHatMotionEvent::HAT_RIGHTUP },
	{ "RIGHTDOWN", UI::JoystickHatMotionEvent::HAT_RIGHTDOWN },
	{ "LEFTUP", UI::JoystickHatMotionEvent::HAT_LEFTUP },
	{ "LEFTDOWN", UI::JoystickHatMotionEvent::HAT_LEFTDOWN },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIJoystickButtonAction[] = {
	{ "DOWN", UI::JoystickButtonEvent::BUTTON_DOWN },
	{ "UP", UI::JoystickButtonEvent::BUTTON_UP },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIExpandDirection[] = {
	{ "BOTH", UI::Expand::BOTH },
	{ "HORIZONTAL", UI::Expand::HORIZONTAL },
	{ "VERTICAL", UI::Expand::VERTICAL },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIGradientDirection[] = {
	{ "HORIZONTAL", UI::Gradient::HORIZONTAL },
	{ "VERTICAL", UI::Gradient::VERTICAL },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIMarginDirection[] = {
	{ "ALL", UI::Margin::ALL },
	{ "HORIZONTAL", UI::Margin::HORIZONTAL },
	{ "VERTICAL", UI::Margin::VERTICAL },
	{ "LEFT", UI::Margin::LEFT },
	{ "RIGHT", UI::Margin::RIGHT },
	{ "TOP", UI::Margin::TOP },
	{ "BOTTOM", UI::Margin::BOTTOM },
	{ 0, 0 },
};

const struct EnumItem ENUM_UINumberLabelFormat[] = {
	{ "NUMBER", UI::NumberLabel::FORMAT_NUMBER },
	{ "NUMBER_2DP", UI::NumberLabel::FORMAT_NUMBER_2DP },
	{ "INTEGER", UI::NumberLabel::FORMAT_INTEGER },
	{ "PERCENT", UI::NumberLabel::FORMAT_PERCENT },
	{ "PERCENT_INTEGER", UI::NumberLabel::FORMAT_PERCENT_INTEGER },
	{ "MONEY", UI::NumberLabel::FORMAT_MONEY },
	{ "MASS_TONNES", UI::NumberLabel::FORMAT_MASS_TONNES },
	{ 0, 0 },
};

const struct EnumItem ENUM_UITableRowAlignDirection[] = {
	{ "TOP", UI::Table::TOP },
	{ "CENTER", UI::Table::CENTER },
	{ "BOTTOM", UI::Table::BOTTOM },
	{ 0, 0 },
};

const struct EnumItem ENUM_UISizeControl[] = {
	{ "NO_WIDTH", UI::Widget::NO_WIDTH },
	{ "NO_HEIGHT", UI::Widget::NO_HEIGHT },
	{ "EXPAND_WIDTH", UI::Widget::EXPAND_WIDTH },
	{ "EXPAND_HEIGHT", UI::Widget::EXPAND_HEIGHT },
	{ "PRESERVE_ASPECT", UI::Widget::PRESERVE_ASPECT },
	{ 0, 0 },
};

const struct EnumItem ENUM_UIFont[] = {
	{ "XSMALL", UI::Widget::FONT_XSMALL },
	{ "SMALL", UI::Widget::FONT_SMALL },
	{ "NORMAL", UI::Widget::FONT_NORMAL },
	{ "LARGE", UI::Widget::FONT_LARGE },
	{ "XLARGE", UI::Widget::FONT_XLARGE },
	{ "HEADING_XSMALL", UI::Widget::FONT_HEADING_XSMALL },
	{ "HEADING_SMALL", UI::Widget::FONT_HEADING_SMALL },
	{ "HEADING_NORMAL", UI::Widget::FONT_HEADING_NORMAL },
	{ "HEADING_LARGE", UI::Widget::FONT_HEADING_LARGE },
	{ "HEADING_XLARGE", UI::Widget::FONT_HEADING_XLARGE },
	{ "INHERIT", UI::Widget::FONT_INHERIT },
	{ 0, 0 },
};

const struct EnumTable ENUM_TABLES[] = {
	{ "EquipSlot", ENUM_EquipSlot },
	{ "EquipType", ENUM_EquipType },
	{ "DetailLevel", ENUM_DetailLevel },
	{ "FileSystemRoot", ENUM_FileSystemRoot },
	{ "PolitCrime", ENUM_PolitCrime },
	{ "PolitEcon", ENUM_PolitEcon },
	{ "PolitGovType", ENUM_PolitGovType },
	{ "ShipFlightState", ENUM_ShipFlightState },
	{ "ShipJumpStatus", ENUM_ShipJumpStatus },
	{ "ShipAlertStatus", ENUM_ShipAlertStatus },
	{ "ShipAIError", ENUM_ShipAIError },
	{ "ShipFuelStatus", ENUM_ShipFuelStatus },
	{ "ShipTypeThruster", ENUM_ShipTypeThruster },
	{ "DualLaserOrientation", ENUM_DualLaserOrientation },
	{ "ShipTypeTag", ENUM_ShipTypeTag },
	{ "EconType", ENUM_EconType },
	{ "BodyType", ENUM_BodyType },
	{ "BodySuperType", ENUM_BodySuperType },
	{ "GameUIFaceFlags", ENUM_GameUIFaceFlags },
	{ "UIAlignDirection", ENUM_UIAlignDirection },
	{ "UIEventType", ENUM_UIEventType },
	{ "UIKeyboardAction", ENUM_UIKeyboardAction },
	{ "UIMouseButtonAction", ENUM_UIMouseButtonAction },
	{ "UIMouseButtonType", ENUM_UIMouseButtonType },
	{ "UIMouseWheelDirection", ENUM_UIMouseWheelDirection },
	{ "UIJoystickHatDirection", ENUM_UIJoystickHatDirection },
	{ "UIJoystickButtonAction", ENUM_UIJoystickButtonAction },
	{ "UIExpandDirection", ENUM_UIExpandDirection },
	{ "UIGradientDirection", ENUM_UIGradientDirection },
	{ "UIMarginDirection", ENUM_UIMarginDirection },
	{ "UINumberLabelFormat", ENUM_UINumberLabelFormat },
	{ "UITableRowAlignDirection", ENUM_UITableRowAlignDirection },
	{ "UISizeControl", ENUM_UISizeControl },
	{ "UIFont", ENUM_UIFont },
	{ 0, 0 },
};

const struct EnumTable ENUM_TABLES_PUBLIC[] = {
	{ "EquipSlot", ENUM_EquipSlot },
	{ "EquipType", ENUM_EquipType },
	{ "DetailLevel", ENUM_DetailLevel },
	{ "FileSystemRoot", ENUM_FileSystemRoot },
	{ "PolitCrime", ENUM_PolitCrime },
	{ "PolitEcon", ENUM_PolitEcon },
	{ "PolitGovType", ENUM_PolitGovType },
	{ "ShipFlightState", ENUM_ShipFlightState },
	{ "ShipJumpStatus", ENUM_ShipJumpStatus },
	{ "ShipAlertStatus", ENUM_ShipAlertStatus },
	{ "ShipAIError", ENUM_ShipAIError },
	{ "ShipFuelStatus", ENUM_ShipFuelStatus },
	{ "ShipTypeThruster", ENUM_ShipTypeThruster },
	{ "DualLaserOrientation", ENUM_DualLaserOrientation },
	{ "ShipTypeTag", ENUM_ShipTypeTag },
	{ "EconType", ENUM_EconType },
	{ "BodyType", ENUM_BodyType },
	{ "BodySuperType", ENUM_BodySuperType },
	{ "GameUIFaceFlags", ENUM_GameUIFaceFlags },
	{ "UIAlignDirection", ENUM_UIAlignDirection },
	{ "UIEventType", ENUM_UIEventType },
	{ "UIKeyboardAction", ENUM_UIKeyboardAction },
	{ "UIMouseButtonAction", ENUM_UIMouseButtonAction },
	{ "UIMouseButtonType", ENUM_UIMouseButtonType },
	{ "UIMouseWheelDirection", ENUM_UIMouseWheelDirection },
	{ "UIJoystickHatDirection", ENUM_UIJoystickHatDirection },
	{ "UIJoystickButtonAction", ENUM_UIJoystickButtonAction },
	{ "UIExpandDirection", ENUM_UIExpandDirection },
	{ "UIGradientDirection", ENUM_UIGradientDirection },
	{ "UIMarginDirection", ENUM_UIMarginDirection },
	{ "UITableRowAlignDirection", ENUM_UITableRowAlignDirection },
	{ "UISizeControl", ENUM_UISizeControl },
	{ "UIFont", ENUM_UIFont },
	{ 0, 0 },
};
