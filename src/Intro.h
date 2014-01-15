// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _INTRO_H
#define _INTRO_H

#include "Cutscene.h"
#include "Background.h"

class Intro : public Cutscene {
public:
	Intro(Graphics::Renderer *r, int width, int height);
	~Intro();
	virtual void Draw(float time);

private:
	std::unique_ptr<Background::Container> m_background;
};

#endif
