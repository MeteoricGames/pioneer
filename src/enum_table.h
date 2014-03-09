/* Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details */
/* Licensed under the terms of the GPL v3. See licenses/GPL-3.txt        */

#ifndef HX_GEN_ENUM_TABLES
#define HX_GEN_ENUM_TABLES

/* THIS FILE IS AUTO-GENERATED, CHANGES WILL BE OVERWRITTEN */
/* enum table generated by scan_enums.py */

struct EnumItem { const char *name; int value; };
struct EnumTable { const char *name; const EnumItem *first; };

extern const struct EnumItem ENUM_EquipSlot[];
extern const struct EnumItem ENUM_EquipType[];
extern const struct EnumItem ENUM_DetailLevel[];
extern const struct EnumItem ENUM_FileSystemRoot[];
extern const struct EnumItem ENUM_PolitCrime[];
extern const struct EnumItem ENUM_PolitEcon[];
extern const struct EnumItem ENUM_PolitGovType[];
extern const struct EnumItem ENUM_ShipFlightState[];
extern const struct EnumItem ENUM_ShipJumpStatus[];
extern const struct EnumItem ENUM_ShipAlertStatus[];
extern const struct EnumItem ENUM_ShipAIError[];
extern const struct EnumItem ENUM_ShipFuelStatus[];
extern const struct EnumItem ENUM_ShipTypeThruster[];
extern const struct EnumItem ENUM_DualLaserOrientation[];
extern const struct EnumItem ENUM_ShipTypeTag[];
extern const struct EnumItem ENUM_EconType[];
extern const struct EnumItem ENUM_BodyType[];
extern const struct EnumItem ENUM_BodySuperType[];
extern const struct EnumItem ENUM_GameUIFaceFlags[];
extern const struct EnumItem ENUM_ModelDebugFlags[];
extern const struct EnumItem ENUM_UIAlignDirection[];
extern const struct EnumItem ENUM_UIEventType[];
extern const struct EnumItem ENUM_UIKeyboardAction[];
extern const struct EnumItem ENUM_UIMouseButtonAction[];
extern const struct EnumItem ENUM_UIMouseButtonType[];
extern const struct EnumItem ENUM_UIMouseWheelDirection[];
extern const struct EnumItem ENUM_UIJoystickHatDirection[];
extern const struct EnumItem ENUM_UIJoystickButtonAction[];
extern const struct EnumItem ENUM_UIExpandDirection[];
extern const struct EnumItem ENUM_UIGradientDirection[];
extern const struct EnumItem ENUM_UIMarginDirection[];
extern const struct EnumItem ENUM_UINumberLabelFormat[];
extern const struct EnumItem ENUM_UITableRowAlignDirection[];
extern const struct EnumItem ENUM_UITableColumnAlignDirection[];
extern const struct EnumItem ENUM_UISizeControl[];
extern const struct EnumItem ENUM_UIFont[];

extern const struct EnumTable ENUM_TABLES[];
extern const struct EnumTable ENUM_TABLES_PUBLIC[];

#endif
