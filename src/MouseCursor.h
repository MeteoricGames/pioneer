// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MOUSECURSOR_H
#define _MOUSECURSOR_H

#include "libs.h"
#include "gui/Gui.h"

class MouseCursor
{
public:
	MouseCursor();
	virtual ~MouseCursor();

	void Update();
	void Draw(Graphics::Renderer* renderer);
	void SetVisible(bool visible);

private:
	MouseCursor(const MouseCursor&);
	MouseCursor& operator=(MouseCursor&);

	std::unique_ptr<Gui::TexturedQuad> m_cursor;
	vector2f m_size;
	vector2f m_pos;
	vector2f m_hotspot;
	bool m_visible;

protected:

};

#endif