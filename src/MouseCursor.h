// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MOUSECURSOR_H
#define _MOUSECURSOR_H

#include "libs.h"
#include "gui/Gui.h"

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

protected:

};

#endif