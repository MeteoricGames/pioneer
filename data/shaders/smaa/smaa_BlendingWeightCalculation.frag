//-----------------------------------------------------------------------------
// Settings (should be filled in by code)
//#define SMAA_PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
//#define SMAA_PRESET_HIGH 1

uniform SMAATexture2D edgesTex;
uniform SMAATexture2D areaTex;
uniform SMAATexture2D searchTex;

varying vec2 texcoord;
varying vec2 pixcoord;
varying vec4 offset0;
varying vec4 offset1;
varying vec4 offset2;
int4 subsampleIndices;

void SMAABlendingWeightCalculationPS()
{    
    subsampleIndices = int4(0, 0, 0, 0);
    
    float4 weights = float4(0.0, 0.0, 0.0, 0.0);

    float2 e = SMAASample(edgesTex, texcoord).rg;
    
    SMAA_BRANCH
    if (e.g > 0.0) { // Edge at north
        #if SMAA_MAX_SEARCH_STEPS_DIAG > 0 || SMAA_FORCE_DIAGONAL_DETECTION == 1
        // Diagonals have both north and west edges, so searching for them in
        // one of the boundaries is enough.
        weights.rg = SMAACalculateDiagWeights(edgesTex, areaTex, texcoord, e, subsampleIndices);

        // We give priority to diagonals, so if we find a diagonal we skip 
        // horizontal/vertical processing.
        SMAA_BRANCH
        if (dot(weights.rg, float2(1.0, 1.0)) == 0.0) {
        #endif

        float2 d;

        // Find the distance to the left:
        float2 coords;
        coords.x = SMAASearchXLeft(edgesTex, searchTex, offset0.xy, offset2.x);
        coords.y = offset1.y; // offset1.y = texcoord.y - 0.25 * SMAA_PIXEL_SIZE.y (@CROSSING_OFFSET)
        d.x = coords.x;

        // Now fetch the left crossing edges, two at a time using bilinear
        // filtering. Sampling at -0.25 (see @CROSSING_OFFSET) enables to
        // discern what value each edge has:
        float e1 = SMAASampleLevelZero(edgesTex, coords).r;

        // Find the distance to the right:
        coords.x = SMAASearchXRight(edgesTex, searchTex, offset0.zw, offset2.y);
        d.y = coords.x;

        // We want the distances to be in pixel units (doing this here allow to
        // better interleave arithmetic and memory accesses):
        d = d / SMAA_PIXEL_SIZE.x - pixcoord.x;

        // SMAAArea below needs a sqrt, as the areas texture is compressed 
        // quadratically:
        float2 sqrt_d = sqrt(abs(d));

        // Fetch the right crossing edges:
        float e2 = SMAASampleLevelZeroOffset(edgesTex, coords, int2(1, 0)).r;

        // Ok, we know how this pattern looks like, now it is time for getting
        // the actual area:
        weights.rg = SMAAArea(areaTex, sqrt_d, e1, e2, float(subsampleIndices.y));

        // Fix corners:
        SMAADetectHorizontalCornerPattern(edgesTex, weights.rg, texcoord, d);

        #if SMAA_MAX_SEARCH_STEPS_DIAG > 0 || SMAA_FORCE_DIAGONAL_DETECTION == 1
        } else
            e.r = 0.0; // Skip vertical processing.
        #endif
    }
    
    SMAA_BRANCH
    if (e.r > 0.0) { // Edge at west
        float2 d;

        // Find the distance to the top:
        float2 coords;
        coords.y = SMAASearchYUp(edgesTex, searchTex, offset1.xy, offset2.z);
        coords.x = offset0.x; // offset[1].x = texcoord.x - 0.25 * SMAA_PIXEL_SIZE.x;
        d.x = coords.y;

        // Fetch the top crossing edges:
        float e1 = SMAASampleLevelZero(edgesTex, coords).g;

        // Find the distance to the bottom:
        coords.y = SMAASearchYDown(edgesTex, searchTex, offset1.zw, offset2.w);
        d.y = coords.y;

        // We want the distances to be in pixel units:
        d = d / SMAA_PIXEL_SIZE.y - pixcoord.y;

        // SMAAArea below needs a sqrt, as the areas texture is compressed 
        // quadratically:
        float2 sqrt_d = sqrt(abs(d));

        // Fetch the bottom crossing edges:
        float e2 = SMAASampleLevelZeroOffset(edgesTex, coords, int2(0, 1)).g;

        // Get the area for this direction:
        weights.ba = SMAAArea(areaTex, sqrt_d, e1, e2, float(subsampleIndices.x));

        // Fix corners:
        SMAADetectVerticalCornerPattern(edgesTex, weights.ba, texcoord, d);
    }
    
    gl_FragColor = weights;
}

void main(void)
{
   SMAABlendingWeightCalculationPS();
}