// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MOUSECURSOR_H
#define _MOUSECURSOR_H

#include "libs.h"
#include "gui/Gui.h"

static const float MouseFlightZoneDiameter = 0.1593f;
static const float MouseFlightDeadZoneDiameter = 0.0157f;

enum MouseCursorType
{
	MCT_NORMAL,
	MCT_FLIGHT,
};

class MouseCursor
{
public:
	MouseCursor(Graphics::Renderer* renderer);
	virtual ~MouseCursor();

	void Update();
	void Draw();
	void SetVisible(bool visible);
	void SetType(MouseCursorType type);
	void Reset();
	void GetFlightCursorState(int* x, int* y) { 
		*x = static_cast<int>(m_flightCursor.x); 
		*y = static_cast<int>(m_flightCursor.y); 
	}

private:
	MouseCursor(const MouseCursor&);
	MouseCursor& operator=(MouseCursor&);

	Graphics::Renderer* m_renderer;
	Graphics::RenderState* m_cursorRS;
	std::vector<Gui::TexturedQuad*> m_vCursor;
	std::vector<vector2f> m_vSize;
	std::vector<vector2f> m_vHotspot;
	vector2f m_pos;
	MouseCursorType m_type;
	bool m_visible;

	std::unique_ptr<Gui::TexturedQuad> m_mouseFlightZone;
	vector2f m_mouseFlightZoneSize;
	vector2f m_mouseFlightZonePos;
	vector2f m_flightCursor;

protected:

};

#endif