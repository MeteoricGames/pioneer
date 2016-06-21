// Copyright © 2008-2014 Pioneer Developers. See AUTHORS.txt for details
// Copyright © 2013-14 Meteoric Games Ltd
// Licensed under the terms of the GPL v3. See licenses/GPL-3.txt

#ifndef _MATERIAL_GL3_H
#define _MATERIAL_GL3_H

/*
 * Material for GL3 effects, replacing old material that mirrored the fixed function pipeline.
 * Effect object must be created before hand, EffectMaterial increments the reference counter for Effect.
 * Effect includes compiled vertex/fragment shaders and an OpenGL program to use them. All shaders and programs are 
 * cached internally by Effect.
 */
#include "libs.h"
#include "graphics/Material.h"

namespace Graphics {

	class Renderer;

	namespace GL3 {

		class Effect;
		struct EffectDescriptor;
		struct EffectDescriptorDirect;

		static const std::string LightSourcesBlock_Name("UBLightSources");
		static const std::string MaterialBlock_Name("UBMaterial");

		class EffectMaterial : public Graphics::Material {
		public:
			EffectMaterial(Renderer* renderer, Effect* effect);
			EffectMaterial(Renderer* renderer, EffectDescriptor& effect_desc);
			EffectMaterial(Renderer* renderer, EffectDescriptorDirect& effect_desc);
			virtual ~EffectMaterial();

			virtual void Apply() override;
			virtual void Unapply() override;

			virtual Effect* GetEffect() const override { return m_effect; }

		protected:
			EffectMaterial();

			void PreInit();
			virtual void Init();
			void CreateEffect(EffectDescriptor& effect_desc);
			void CreateEffect(EffectDescriptorDirect& effect_desc);

			Renderer *m_renderer;
			Effect* m_effect;

			int m_invLogZfarPlus1Id;

			bool m_usesDeprecatedMtrl;
			bool m_usesDefaultTextures;

			int m_numLightsId;

			// Uniform blocks
			int m_lightsBlock;
			int m_materialsBlock;

			// Legacy material
			int m_diffuseId;
			int m_specularId;
			int m_emissiveId;
			int m_shininessId;

			int m_texture0Id;
			int m_texture1Id;
			int m_texture2Id;
			int m_texture3Id;
			int m_texture4Id;
			int m_heatGradId;
			int m_heatMatId;
			int m_heatNormId;
			int m_heatAmountId;
			int m_ambientId;
			int m_tintId;
			int m_pointSizeId;
			int m_universeBoxId;
			int m_atmosColorId;
			int m_atmosDensityId;
			int m_hemiDiffId;
			int m_hemiGlossId;
			int m_irrGlossyExpId;
			
			bool m_isInit;
		};
	}
}
#endif
