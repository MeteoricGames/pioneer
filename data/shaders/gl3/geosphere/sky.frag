// EFFECT: GEOSPHERE SKY

//----------------------------------------------------- In/Out/Uniforms

// IN
in float varLogDepth;
in vec4 v_eyepos;

// OUT
out vec4 o_FragColor;

// UNIFORMS
uniform float invLogZfarPlus1;
uniform vec4 u_atmosColor;
// to keep distances sane we do a nearer, smaller scam. this is how many times
// smaller the geosphere has been made
uniform float u_geosphereScale;
uniform float u_geosphereScaledRadius;
uniform float u_geosphereAtmosTopRad;
uniform vec3 u_geosphereCenter;
uniform float u_geosphereAtmosFogDensity;
uniform float u_geosphereAtmosInvScaleHeight;
#ifdef ECLIPSE
	uniform int u_shadows;
	uniform ivec3 u_occultedLight;
	uniform vec3 u_shadowCentreX;
	uniform vec3 u_shadowCentreY;
	uniform vec3 u_shadowCentreZ;
	uniform vec3 u_srad;
	uniform vec3 u_lrad;
	uniform vec3 u_sdivlrad;
#endif // ECLIPSE

// SHARED_UNIFORMS
#ifdef LIGHTING
	// light uniform parameters
	struct s_LightSourceParameters{
		vec4 position;
		vec4 diffuse;
		vec4 specular;
	};	
	layout(std140) uniform UBLightSources {
		s_LightSourceParameters su_LightSource[4];
	};
	uniform float u_numLights;
#endif // LIGHTING

//--------------------------------------------------------- FRAGMENT SHADER
void SetFragDepth()
{
	gl_FragDepth = gl_DepthRange.near + (gl_DepthRange.far * log(varLogDepth + 1.0) * invLogZfarPlus1);
}

//Currently used by: planet ring shader, geosphere shaders
float findSphereEyeRayEntryDistance(in vec3 sphereCenter, in vec3 eyeTo, in float radius)
{
	vec3 v = -sphereCenter;
	vec3 dir = normalize(eyeTo);
	float b = -dot(v, dir);
	float det = (b * b) - dot(v, v) + (radius * radius);
	float entryDist = 0.0;
	if (det > 0.0) {
		det = sqrt(det);
		float i1 = b - det;
		float i2 = b + det;
		if (i2 > 0.0) {
			entryDist = max(i1, 0.0);
		}
	}
	return entryDist;
}

// Used by: geosphere shaders
// Calculate length*density product of a line through the atmosphere
// a - start coord (normalized relative to atmosphere radius)
// b - end coord " "
// centerDensity - atmospheric density at centre of sphere
// length - real length of line in meters
float AtmosLengthDensityProduct(vec3 a, vec3 b, float surfaceDensity, float len, float invScaleHeight)
{
	/* 4 samples */
	float ldprod = 0.0;
	vec3 dir = b-a;
	ldprod = surfaceDensity * (
			exp(-invScaleHeight*(length(a)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.2*dir)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.4*dir)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.6*dir)-1.0)) +
			exp(-invScaleHeight*(length(a + 0.8*dir)-1.0)) +
			exp(-invScaleHeight*max(length(b)-1.0, 0.0)));
	ldprod *= len;
	return ldprod;
}

void sphereEntryExitDist(out float near, out float far, const in vec3 sphereCenter, const in vec3 eyeTo, const in float radius)
{
	vec3 v = -sphereCenter;
	vec3 dir = normalize(eyeTo);
	float b = -dot(v, dir);
	float det = (b * b) - dot(v, v) + (radius * radius);
	float i1, i2;
	near = 0.0;
	far = 0.0;
	if (det > 0.0) {
		det = sqrt(det);
		i1 = b - det;
		i2 = b + det;
		if (i2 > 0.0) {
			near = max(i1, 0.0);
			far = i2;
		}
	}
}

// integral used in shadow calculations:
// \Int (m - \sqrt(d^2+t^2)) dt = (t\sqrt(d^2+t^2) + d^2 log(\sqrt(d^2+t^2)+t))/2
float shadowInt(const in float t1, const in float t2, const in float dsq, const in float m)
{
	float s1 = sqrt(dsq+t1*t1);
	float s2 = sqrt(dsq+t2*t2);
	return m*(t2-t1) - (t2*s2 - t1*s1 + dsq*( log(max(0.000001, s2+t2)) - log(max(0.000001, s1+t1)))) * 0.5;
}

void main(void)
{
	float skyNear, skyFar;
	vec3 eyenorm = normalize(v_eyepos.xyz);
	float specularHighlight=0.0;

	sphereEntryExitDist(skyNear, skyFar, u_geosphereCenter, v_eyepos.xyz, u_geosphereScaledRadius * u_geosphereAtmosTopRad);
	float atmosDist = u_geosphereScale * (skyFar - skyNear);
	float ldprod=0.0;
	
	// a&b scaled so length of 1.0 means planet surface.
	vec3 a = (skyNear * eyenorm - u_geosphereCenter) / u_geosphereScaledRadius;
	vec3 b = (skyFar * eyenorm - u_geosphereCenter) / u_geosphereScaledRadius;
	ldprod = AtmosLengthDensityProduct(a, b, u_atmosColor.a * u_geosphereAtmosFogDensity, atmosDist, u_geosphereAtmosInvScaleHeight);
	
	float fogFactor = 1.0 / exp(ldprod);
	vec4 atmosDiffuse = vec4(0.0);

	#ifdef LIGHTING	
		float INV_NUM_LIGHTS = 1.0 / u_numLights;
		vec3 surfaceNorm = normalize(skyNear * eyenorm - u_geosphereCenter);
		for (int i=0; i<u_numLights; ++i) {
			vec3 lightDir = normalize(vec3(su_LightSource[i].position));
			float uneclipsed = 1.0;
			#ifdef ECLIPSE
				for (int j=0; j<u_shadows; j++) {
					if (i != u_occultedLight[j]) {
						continue;
					}

					// Eclipse handling:
					// Calculate proportion of the in-atmosphere eyeline which is shadowed,
					// weighting according to completeness of the shadow (penumbra vs umbra).
					// This ignores variation in atmosphere density, and ignores outscatter along
					// the eyeline, so is not very accurate. But it gives decent results.

					vec3 centre = vec3( u_shadowCentreX[j], u_shadowCentreY[j], u_shadowCentreZ[j] );

					vec3 ap = a - dot(a,lightDir)*lightDir - centre;
					vec3 bp = b - dot(b,lightDir)*lightDir - centre;

					vec3 dirp = normalize(bp-ap);
					float ad = dot(ap,dirp);
					float bd = dot(bp,dirp);
					vec3 p = ap - dot(ap,dirp)*dirp;
					float perpsq = dot(p,p);

					// we now want to calculate the proportion of shadow on the horizontal line
					// segment from ad to bd, shifted vertically from centre by \sqrt(perpsq). For
					// the partially occluded segments, to have an analytic solution to the integral
					// we estimate the light intensity to drop off linearly with radius between
					// maximal occlusion and none.

					float minr = u_srad[j]-u_lrad[j];
					float maxr = u_srad[j]+u_lrad[j];
					float maxd = sqrt( max(0.0, maxr*maxr - perpsq) );
					float mind = sqrt( max(0.0, minr*minr - perpsq) );

					float shadow = ( shadowInt(clamp(ad, -maxd, -mind), clamp(bd, -maxd, -mind), perpsq, maxr)
						+ shadowInt(clamp(ad, mind, maxd), clamp(bd, mind, maxd), perpsq, maxr) )
						/ (maxr-minr) + (clamp(bd, -mind, mind) - clamp(ad, -mind, mind));

					float maxOcclusion = min(1.0, (u_sdivlrad[j])*(u_sdivlrad[j]));

					uneclipsed -= maxOcclusion * shadow / (bd-ad);
				}
			#endif // ECLIPSE
			uneclipsed = clamp(uneclipsed, 0.0, 1.0);

			float nDotVP =  max(0.0, dot(surfaceNorm, lightDir));
			float nnDotVP = max(0.0, dot(surfaceNorm, -lightDir));  //need backlight to increase horizon
			atmosDiffuse +=  su_LightSource[i].diffuse * uneclipsed * 0.5*(nDotVP+0.5*clamp(1.0-nnDotVP*4.0,0.0,1.0) * INV_NUM_LIGHTS);

			//Calculate Specular Highlight
			vec3 L = normalize(su_LightSource[i].position.xyz - v_eyepos.xyz); 
			vec3 E = normalize(-v_eyepos.xyz);
			vec3 R = normalize(-reflect(L,vec3(0.0))); 
			specularHighlight += pow(max(dot(R,E),0.0),64.0) * uneclipsed * INV_NUM_LIGHTS;

		}
	#endif

	//calculate sunset tone red when passing through more atmosphere, clamp everything.
	float atmpower = (atmosDiffuse.r + atmosDiffuse.g + atmosDiffuse.b) / 3.0;
	vec4 sunset = vec4(0.8, clamp(pow(atmpower, 0.8), 0.0, 1.0), clamp(pow(atmpower, 1.2), 0.0, 1.0), 1.0);

	atmosDiffuse.a = 1.0;
	o_FragColor = (1.0 - fogFactor) * (atmosDiffuse*
		vec4(u_atmosColor.rgb, 1.0)) +
		(0.02-clamp(fogFactor,0.0,0.01))*atmosDiffuse*ldprod*sunset +     //increase light on lower atmosphere.
		u_atmosColor*specularHighlight*(1.0-fogFactor)*sunset *		  //add light from specularHighlight.
		clamp(1.0/sqrt(u_geosphereAtmosFogDensity*10000.0),0.4,1.0);	  //darken atmposphere based on density

	SetFragDepth();
}
