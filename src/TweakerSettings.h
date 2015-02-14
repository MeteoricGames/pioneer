// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _TWEAKER_SETTINGS_H_
#define _TWEAKER_SETTINGS_H_

#include "libs.h"

struct TS_ToneMapping
{
	TS_ToneMapping() {
		// Defaults
		defog = 0.01f;
		fogColor = Color4f(1.0f, 1.0f, 1.0f, 1.0f);
		exposure = 0.35f;
		gamma = 0.8f;
		vignetteCenter = vector2f(0.5f, 0.5f);
		vignetteRadius = 1.0f;
		vignetteAmount = -1.0f;
		blueShift = 0.25;

		effect = nullptr;
		defogId = -1;
		fogColorId = -1;
		exposureId = -1;
		gammaId = -1;
		vignetteCenterId = -1;
		vignetteRadiusId = -1;
		vignetteAmountId = -1;
		blueShiftId = -1;
	}

	float defog;
	Color4f fogColor;
	float exposure;
	float gamma;
	vector2f vignetteCenter;
	float vignetteRadius;
	float vignetteAmount;
	float blueShift;

	Graphics::GL3::Effect* effect;
	int defogId;
	int fogColorId;
	int exposureId;
	int gammaId;
	int vignetteCenterId;
	int vignetteRadiusId;
	int vignetteAmountId;
	int blueShiftId;
};

struct TS_FilmGrain
{
	TS_FilmGrain() {
		mode = 1.0f;
	}

	float mode;

	Graphics::GL3::Effect* effect;
	int modeId;
};

struct TS_Scanlines
{
	TS_Scanlines() {
		resolution = vector2f(2.0f, 2.0f);
		hardScan = -8.0f;
		//hardPix = -3.0f;
		warp = vector2f(1.0f / 32.0f, 1.0f / 24.0f);
		maskDark = 0.5f;
		maskLight = 1.5f;

		effect = nullptr;
		resolutionId = -1;
		hardScanId = -1;
		//hardPixId = -1;
		warpId = -1;
		maskDarkId = -1;
		maskLightId = -1;
	}

	vector2f resolution;
	float hardScan;
	//float hardPix;
	vector2f warp;
	float maskDark;
	float maskLight;

	Graphics::GL3::Effect* effect;
	int resolutionId;
	int hardScanId;
	//int hardPixId;
	int warpId;
	int maskDarkId;
	int maskLightId;
};

struct TS_ChromaticAberration
{
	TS_ChromaticAberration() {
		centerBuffer = 0.1f;
		aberrationStrength = 0.3f;

		effect = nullptr;
		centerBufferId = -1;
		aberrationStrengthId = -1;
	}
	float centerBuffer;
	float aberrationStrength;

	Graphics::GL3::Effect* effect;
	int centerBufferId;
	int aberrationStrengthId;
};

struct TS_FXAA
{
};

#endif // TWEAKER_SETTINGS_H