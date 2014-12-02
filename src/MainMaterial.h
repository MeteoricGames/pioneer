// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MAINMATERIAL_H_
#define _MAINMATERIAL_H_

#include "graphics/Renderer.h"
#include "graphics/gl3/Effect.h"
#include "graphics/gl3/EffectMaterial.h"
#include "graphics/Material.h"

class MainMaterial : public Graphics::GL3::EffectMaterial
{
public:
	MainMaterial(Graphics::Renderer* renderer, Graphics::MaterialDescriptor& material_desc, bool lit = false);
	virtual ~MainMaterial();

	virtual void Apply() override;

private:
	MainMaterial(const MainMaterial&);
	MainMaterial& operator=(const MainMaterial&);

	virtual void Init() override;

	bool m_isLit;

	int m_shieldStrengthId;
	int m_shieldCooldownId;
	int m_hitPosId;
	int m_radiiId;
	int m_numHitsId;
};

#endif