//-----------------------------------------------------------------------------
// SMAA Presets

/**
 * Note that if you use one of these presets, the corresponding macros below
 * won't be used.
 */

#if SMAA_PRESET_LOW == 1
#define SMAA_THRESHOLD 0.15
#define SMAA_MAX_SEARCH_STEPS 4
#define SMAA_MAX_SEARCH_STEPS_DIAG 0
#define SMAA_CORNER_ROUNDING 100
#elif SMAA_PRESET_MEDIUM == 1
#define SMAA_THRESHOLD 0.1
#define SMAA_MAX_SEARCH_STEPS 8
#define SMAA_MAX_SEARCH_STEPS_DIAG 0
#define SMAA_CORNER_ROUNDING 100
#elif SMAA_PRESET_HIGH == 1
#define SMAA_THRESHOLD 0.1
#define SMAA_MAX_SEARCH_STEPS 16
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#define SMAA_CORNER_ROUNDING 25
#elif SMAA_PRESET_ULTRA == 1
#define SMAA_THRESHOLD 0.05
#define SMAA_MAX_SEARCH_STEPS 32
#define SMAA_MAX_SEARCH_STEPS_DIAG 16
#define SMAA_CORNER_ROUNDING 25
#endif

//-----------------------------------------------------------------------------
// Configurable Defines

/**
 * SMAA_THRESHOLD specifies the threshold or sensitivity to edges.
 * Lowering this value you will be able to detect more edges at the expense of
 * performance. 
 *
 * Range: [0, 0.5]
 *   0.1 is a reasonable value, and allows to catch most visible edges.
 *   0.05 is a rather overkill value, that allows to catch 'em all.
 *
 *   If temporal supersampling is used, 0.2 could be a reasonable value, as low
 *   contrast edges are properly filtered by just 2x.
 */
#ifndef SMAA_THRESHOLD
#define SMAA_THRESHOLD 0.1
#endif

/**
 * SMAA_DEPTH_THRESHOLD specifies the threshold for depth edge detection.
 * 
 * Range: depends on the depth range of the scene.
 */
#ifndef SMAA_DEPTH_THRESHOLD
#define SMAA_DEPTH_THRESHOLD (0.1 * SMAA_THRESHOLD)
#endif

/**
 * SMAA_MAX_SEARCH_STEPS specifies the maximum steps performed in the
 * horizontal/vertical pattern searches, at each side of the pixel.
 *
 * In number of pixels, it's actually the double. So the maximum line length
 * perfectly handled by, for example 16, is 64 (by perfectly, we meant that
 * longer lines won't look as good, but still antialiased).
 *
 * Range: [0, 98]
 */
#ifndef SMAA_MAX_SEARCH_STEPS
#define SMAA_MAX_SEARCH_STEPS 16
#endif

/**
 * SMAA_MAX_SEARCH_STEPS_DIAG specifies the maximum steps performed in the
 * diagonal pattern searches, at each side of the pixel. In this case we jump
 * one pixel at time, instead of two.
 *
 * Range: [0, 20]; set it to 0 to disable diagonal processing.
 *
 * On high-end machines it is cheap (between a 0.8x and 0.9x slower for 16 
 * steps), but it can have a significant impact on older machines.
 */
#ifndef SMAA_MAX_SEARCH_STEPS_DIAG
#define SMAA_MAX_SEARCH_STEPS_DIAG 8
#endif

/**
 * SMAA_CORNER_ROUNDING specifies how much sharp corners will be rounded.
 *
 * Range: [0, 100]; set it to 100 to disable corner detection.
 */
#ifndef SMAA_CORNER_ROUNDING
#define SMAA_CORNER_ROUNDING 25
#endif

/**
 * Predicated thresholding allows to better preserve texture details and to
 * improve performance, by decreasing the number of detected edges using an
 * additional buffer like the light accumulation buffer, object ids or even the
 * depth buffer (the depth buffer usage may be limited to indoor or short range
 * scenes).
 *
 * It locally decreases the luma or color threshold if an edge is found in an
 * additional buffer (so the global threshold can be higher).
 *
 * This method was developed by Playstation EDGE MLAA team, and used in 
 * Killzone 3, by using the light accumulation buffer. More information here:
 *     http://iryoku.com/aacourse/downloads/06-MLAA-on-PS3.pptx 
 */
#ifndef SMAA_PREDICATION
#define SMAA_PREDICATION 0
#endif

/**
 * Threshold to be used in the additional predication buffer. 
 *
 * Range: depends on the input, so you'll have to find the magic number that
 * works for you.
 */
#ifndef SMAA_PREDICATION_THRESHOLD
#define SMAA_PREDICATION_THRESHOLD 0.01
#endif

/**
 * How much to scale the global threshold used for luma or color edge
 * detection when using predication.
 *
 * Range: [1, 5]
 */
#ifndef SMAA_PREDICATION_SCALE
#define SMAA_PREDICATION_SCALE 2.0
#endif

/**
 * How much to locally decrease the threshold.
 *
 * Range: [0, 1]
 */
#ifndef SMAA_PREDICATION_STRENGTH
#define SMAA_PREDICATION_STRENGTH 0.4
#endif

/**
 * Temporal reprojection allows to remove ghosting artifacts when using
 * temporal supersampling. We use the CryEngine 3 method which also introduces
 * velocity weighting. This feature is of extreme importance for totally
 * removing ghosting. More information here:
 *    http://iryoku.com/aacourse/downloads/13-Anti-Aliasing-Methods-in-CryENGINE-3.pdf
 *
 * Note that you'll need to setup a velocity buffer for enabling reprojection.
 * For static geometry, saving the previous depth buffer is a viable
 * alternative.
 */
#ifndef SMAA_REPROJECTION
#define SMAA_REPROJECTION 0
#endif

/**
 * SMAA_REPROJECTION_WEIGHT_SCALE controls the velocity weighting. It allows to
 * remove ghosting trails behind the moving object, which are not removed by
 * just using reprojection. Using low values will exhibit ghosting, while using
 * high values will disable temporal supersampling under motion.
 *
 * Behind the scenes, velocity weighting removes temporal supersampling when
 * the velocity of the subsamples differs (meaning they are different objects).
 *
 * Range: [0, 80]
 */
#define SMAA_REPROJECTION_WEIGHT_SCALE 30.0

//-----------------------------------------------------------------------------
// Non-Configurable Defines

#ifndef SMAA_AREATEX_MAX_DISTANCE
#define SMAA_AREATEX_MAX_DISTANCE 16
#endif
#ifndef SMAA_AREATEX_MAX_DISTANCE_DIAG
#define SMAA_AREATEX_MAX_DISTANCE_DIAG 20
#endif
#define SMAA_AREATEX_PIXEL_SIZE (1.0 / float2(160.0, 560.0))
#define SMAA_AREATEX_SUBTEX_SIZE (1.0 / 7.0)

//-----------------------------------------------------------------------------
// Porting Functions

#define SMAATexture2D sampler2D
#define SMAASampleLevelZero(tex, coord) textureLod(tex, coord, 0.0)
#define SMAASampleLevelZeroPoint(tex, coord) textureLod(tex, coord, 0.0)
#define SMAASample(tex, coord) texture(tex, coord)
#define SMAASamplePoint(tex, coord) texture(tex, coord)
#define SMAASampleLevelZeroOffset(tex, coord, offset) textureLodOffset(tex, coord, 0.0, offset)
#define SMAASampleOffset(tex, coord, offset) texture(tex, coord, offset)
#define SMAALerp(a, b, t) mix(a, b, t)
#define SMAASaturate(a) clamp(a, 0.0, 1.0)
#define SMAA_FLATTEN
#define SMAA_BRANCH
#define float2 vec2
#define float3 vec3
#define float4 vec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define SMAAMad(a, b, c) (a * b + c)

//-----------------------------------------------------------------------------
// Misc functions

/**
 * Gathers current pixel, and the top-left neighbors.
 */
float3 SMAAGatherNeighbours(float2 texcoord,
                            float4 offset[3],
                            SMAATexture2D tex) {
    float P = SMAASample(tex, texcoord).r;
    float Pleft = SMAASample(tex, offset[0].xy).r;
    float Ptop  = SMAASample(tex, offset[0].zw).r;
    return float3(P, Pleft, Ptop);
}

/**
 * Adjusts the threshold by means of predication.
 */
float2 SMAACalculatePredicatedThreshold(float2 texcoord,
                                        float4 offset[3],
                                        SMAATexture2D colorTex,
                                        SMAATexture2D predicationTex) {
    float3 neighbours = SMAAGatherNeighbours(texcoord, offset, predicationTex);
    float2 delta = abs(neighbours.xx - neighbours.yz);
    float2 edges = step(SMAA_PREDICATION_THRESHOLD, delta);
    return SMAA_PREDICATION_SCALE * SMAA_THRESHOLD * (1.0 - SMAA_PREDICATION_STRENGTH * edges);
}

//-----------------------------------------------------------------------------
// Diagonal Search Functions

#if SMAA_MAX_SEARCH_STEPS_DIAG > 0 || SMAA_FORCE_DIAGONAL_DETECTION == 1

/**
 * These functions allows to perform diagonal pattern searches.
 */
float SMAASearchDiag1(SMAATexture2D edgesTex, float2 texcoord, float2 dir, float c) {
    texcoord += dir * SMAA_PIXEL_SIZE;
    float2 e = float2(0.0, 0.0);
    float i;
    for (i = 0.0; i < float(SMAA_MAX_SEARCH_STEPS_DIAG); i++) {
        e.rg = SMAASampleLevelZero(edgesTex, texcoord).rg;
        SMAA_FLATTEN if (dot(e, float2(1.0, 1.0)) < 1.9) break;
        texcoord += dir * SMAA_PIXEL_SIZE;
    }
    return i + float(e.g > 0.9) * c;
}

float SMAASearchDiag2(SMAATexture2D edgesTex, float2 texcoord, float2 dir, float c) {
    texcoord += dir * SMAA_PIXEL_SIZE;
    float2 e = float2(0.0, 0.0);
    float i;
    for (i = 0.0; i < float(SMAA_MAX_SEARCH_STEPS_DIAG); i++) {
        e.g = SMAASampleLevelZero(edgesTex, texcoord).g;
        e.r = SMAASampleLevelZeroOffset(edgesTex, texcoord, int2(1, 0)).r;
        SMAA_FLATTEN if (dot(e, float2(1.0, 1.0)) < 1.9) break;
        texcoord += dir * SMAA_PIXEL_SIZE;
    }
    return i + float(e.g > 0.9) * c;
}

/** 
 * Similar to SMAAArea, this calculates the area corresponding to a certain
 * diagonal distance and crossing edges 'e'.
 */
float2 SMAAAreaDiag(SMAATexture2D areaTex, float2 dist, float2 e, float offset) {
    float2 texcoord = float(SMAA_AREATEX_MAX_DISTANCE_DIAG) * e + dist;

    // We do a scale and bias for mapping to texel space:
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + (0.5 * SMAA_AREATEX_PIXEL_SIZE);

    // Diagonal areas are on the second half of the texture:
    texcoord.x += 0.5;

    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    // Do it!
    return SMAASampleLevelZero(areaTex, texcoord).rg;
}

/**
 * This searches for diagonal patterns and returns the corresponding weights.
 */
float2 SMAACalculateDiagWeights(SMAATexture2D edgesTex, SMAATexture2D areaTex, float2 texcoord, float2 e, int4 subsampleIndices) {
    float2 weights = float2(0.0, 0.0);

    float2 d;
    d.x = e.r > 0.0? SMAASearchDiag1(edgesTex, texcoord, float2(-1.0,  1.0), 1.0) : 0.0;
    d.y = SMAASearchDiag1(edgesTex, texcoord, float2(1.0, -1.0), 0.0);

    SMAA_BRANCH
    if (d.r + d.g > 2.0) { // d.r + d.g + 1 > 3
        float4 coords = SMAAMad(float4(-d.r, d.r, d.g, -d.g), SMAA_PIXEL_SIZE.xyxy, texcoord.xyxy);

        float4 c;
        c.x = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2(-1,  0)).g;
        c.y = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2( 0,  0)).r;
        c.z = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2( 1,  0)).g;
        c.w = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2( 1, -1)).r;
        float2 e = 2.0 * c.xz + c.yw;
        float t = float(SMAA_MAX_SEARCH_STEPS_DIAG) - 1.0;
        e *= step(d.rg, float2(t, t));

        weights += SMAAAreaDiag(areaTex, d, e, float(subsampleIndices.z));
    }

    d.x = SMAASearchDiag2(edgesTex, texcoord, float2(-1.0, -1.0), 0.0);
    float right = SMAASampleLevelZeroOffset(edgesTex, texcoord, int2(1, 0)).r;
    d.y = right > 0.0? SMAASearchDiag2(edgesTex, texcoord, float2(1.0, 1.0), 1.0) : 0.0;

    SMAA_BRANCH
    if (d.r + d.g > 2.0) { // d.r + d.g + 1 > 3
        float4 coords = SMAAMad(float4(-d.r, -d.r, d.g, d.g), SMAA_PIXEL_SIZE.xyxy, texcoord.xyxy);

        float4 c;
        c.x  = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2(-1,  0)).g;
        c.y  = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2( 0, -1)).r;
        c.zw = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2( 1,  0)).gr;
        float2 e = 2.0 * c.xz + c.yw;
        float t = float(SMAA_MAX_SEARCH_STEPS_DIAG) - 1.0;
        e *= step(d.rg, float2(t, t));

        weights += SMAAAreaDiag(areaTex, d, e, float(subsampleIndices.w)).gr;
    }

    return weights;
}
#endif

//-----------------------------------------------------------------------------
// Horizontal/Vertical Search Functions

/**
 * This allows to determine how much length should we add in the last step
 * of the searches. It takes the bilinearly interpolated edge (see 
 * @PSEUDO_GATHER4), and adds 0, 1 or 2, depending on which edges and
 * crossing edges are active.
 */
float SMAASearchLength(SMAATexture2D searchTex, float2 e, float bias, float scale) {
    // Not required if searchTex accesses are set to point:
    float2 SEARCH_TEX_PIXEL_SIZE = 1.0 / float2(66.0, 33.0);
     e = float2(bias, 0.0) + 0.5 * SEARCH_TEX_PIXEL_SIZE + 
         e * float2(scale, 1.0) * float2(64.0, 32.0) * SEARCH_TEX_PIXEL_SIZE;
    //e.r = bias + e.r * scale;
    return 255.0 * SMAASampleLevelZeroPoint(searchTex, e).r;
}

/**
 * Horizontal/vertical search functions for the 2nd pass.
 */
float SMAASearchXLeft(SMAATexture2D edgesTex, SMAATexture2D searchTex, float2 texcoord, float end) {
    /**
     * @PSEUDO_GATHER4
     * This texcoord has been offset by (-0.25, -0.125) in the vertex shader to
     * sample between edge, thus fetching four edges in a row.
     * Sampling with different offsets in each direction allows to disambiguate
     * which edges are active from the four fetched ones.
     */
    float2 e = float2(0.0, 1.0);
    while (texcoord.x > end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0) { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero(edgesTex, texcoord).rg;
        texcoord -= float2(2.0, 0.0) * SMAA_PIXEL_SIZE;
    }

    // We correct the previous (-0.25, -0.125) offset we applied:
    texcoord.x += 0.25 * SMAA_PIXEL_SIZE.x;

    // The searches are bias by 1, so adjust the coords accordingly:
    texcoord.x += SMAA_PIXEL_SIZE.x;

    // Disambiguate the length added by the last step:
    texcoord.x += 2.0 * SMAA_PIXEL_SIZE.x; // Undo last step
    texcoord.x -= SMAA_PIXEL_SIZE.x * SMAASearchLength(searchTex, e, 0.0, 0.5);

    return texcoord.x;
}

float SMAASearchXRight(SMAATexture2D edgesTex, SMAATexture2D searchTex, float2 texcoord, float end) {
    float2 e = float2(0.0, 1.0);
    while (texcoord.x < end && 
           e.g > 0.8281 && // Is there some edge not activated?
           e.r == 0.0) { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero(edgesTex, texcoord).rg;
        texcoord += float2(2.0, 0.0) * SMAA_PIXEL_SIZE;
    }

    texcoord.x -= 0.25 * SMAA_PIXEL_SIZE.x;
    texcoord.x -= SMAA_PIXEL_SIZE.x;
    texcoord.x -= 2.0 * SMAA_PIXEL_SIZE.x;
    texcoord.x += SMAA_PIXEL_SIZE.x * SMAASearchLength(searchTex, e, 0.5, 0.5);
    return texcoord.x;
}

float SMAASearchYUp(SMAATexture2D edgesTex, SMAATexture2D searchTex, float2 texcoord, float end) {
    float2 e = float2(1.0, 0.0);
    while (texcoord.y > end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero(edgesTex, texcoord).rg;
        texcoord -= float2(0.0, 2.0) * SMAA_PIXEL_SIZE;
    }

    texcoord.y += 0.25 * SMAA_PIXEL_SIZE.y;
    texcoord.y += SMAA_PIXEL_SIZE.y;
    texcoord.y += 2.0 * SMAA_PIXEL_SIZE.y;
    texcoord.y -= SMAA_PIXEL_SIZE.y * SMAASearchLength(searchTex, e.gr, 0.0, 0.5);
    return texcoord.y;
}

float SMAASearchYDown(SMAATexture2D edgesTex, SMAATexture2D searchTex, float2 texcoord, float end) {
    float2 e = float2(1.0, 0.0);
    while (texcoord.y < end && 
           e.r > 0.8281 && // Is there some edge not activated?
           e.g == 0.0) { // Or is there a crossing edge that breaks the line?
        e = SMAASampleLevelZero(edgesTex, texcoord).rg;
        texcoord += float2(0.0, 2.0) * SMAA_PIXEL_SIZE;
    }
    
    texcoord.y -= 0.25 * SMAA_PIXEL_SIZE.y;
    texcoord.y -= SMAA_PIXEL_SIZE.y;
    texcoord.y -= 2.0 * SMAA_PIXEL_SIZE.y;
    texcoord.y += SMAA_PIXEL_SIZE.y * SMAASearchLength(searchTex, e.gr, 0.5, 0.5);
    return texcoord.y;
}

/** 
 * Ok, we have the distance and both crossing edges. So, what are the areas
 * at each side of current edge?
 */
float2 SMAAArea(SMAATexture2D areaTex, float2 dist, float e1, float e2, float offset) {
    // Rounding prevents precision errors of bilinear filtering:
    float2 texcoord = float(SMAA_AREATEX_MAX_DISTANCE) * round(4.0 * float2(e1, e2)) + dist;
    
    // We do a scale and bias for mapping to texel space:
    texcoord = SMAA_AREATEX_PIXEL_SIZE * texcoord + (0.5 * SMAA_AREATEX_PIXEL_SIZE);

    // Move to proper place, according to the subpixel offset:
    texcoord.y += SMAA_AREATEX_SUBTEX_SIZE * offset;

    // Do it!
    return SMAASampleLevelZero(areaTex, texcoord).rg;
}

//-----------------------------------------------------------------------------
// Corner Detection Functions

void SMAADetectHorizontalCornerPattern(SMAATexture2D edgesTex, inout float2 weights, float2 texcoord, float2 d) {
    #if SMAA_CORNER_ROUNDING < 100 || SMAA_FORCE_CORNER_DETECTION == 1
    float4 coords = SMAAMad(float4(d.x, 0.0, d.y, 0.0),
                            SMAA_PIXEL_SIZE.xyxy, texcoord.xyxy);
    float2 e;
    e.r = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2(0.0,  1.0)).r;
    bool left = abs(d.x) < abs(d.y);
    e.g = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2(0.0, -2.0)).r;
    if (left) weights *= SMAASaturate(float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e);

    e.r = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2(1.0,  1.0)).r;
    e.g = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2(1.0, -2.0)).r;
    if (!left) weights *= SMAASaturate(float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e);
    #endif
}

void SMAADetectVerticalCornerPattern(SMAATexture2D edgesTex, inout float2 weights, float2 texcoord, float2 d) {
    #if SMAA_CORNER_ROUNDING < 100 || SMAA_FORCE_CORNER_DETECTION == 1
    float4 coords = SMAAMad(float4(0.0, d.x, 0.0, d.y),
                            SMAA_PIXEL_SIZE.xyxy, texcoord.xyxy);
    float2 e;
    e.r = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2( 1.0, 0.0)).g;
    bool left = abs(d.x) < abs(d.y);
    e.g = SMAASampleLevelZeroOffset(edgesTex, coords.xy, int2(-2.0, 0.0)).g;
    if (left) weights *= SMAASaturate(float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e);

    e.r = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2( 1.0, 1.0)).g;
    e.g = SMAASampleLevelZeroOffset(edgesTex, coords.zw, int2(-2.0, 1.0)).g;
    if (!left) weights *= SMAASaturate(float(SMAA_CORNER_ROUNDING) / 100.0 + 1.0 - e);
    #endif
}
