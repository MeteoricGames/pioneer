//-----------------------------------------------------------------------------
// Settings (should be filled in by code)
//#define SMAA_PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
//#define SMAA_PRESET_HIGH 1

uniform SMAATexture2D colorTex;
uniform SMAATexture2D blendTex;

varying vec2 texcoord;
varying float4 offset[2];

void SMAANeighborhoodBlendingPS()
{
   // Fetch the blending weights for current pixel:
    float4 a;
    a.xz = SMAASample(blendTex, texcoord).xz;
    a.y = SMAASample(blendTex, offset[1].zw).g;
    a.w = SMAASample(blendTex, offset[1].xy).a;

    // Is there any blending weight with a value greater than 0.0?
    SMAA_BRANCH
    if (dot(a, float4(1.0, 1.0, 1.0, 1.0)) < 1e-5) {
        gl_FragColor = SMAASampleLevelZero(colorTex, texcoord);
    } else {
        float4 color = float4(0.0, 0.0, 0.0, 0.0);

        // Up to 4 lines can be crossing a pixel (one through each edge). We
        // favor blending by choosing the line with the maximum weight for each
        // direction:
        float2 offset;
        offset.x = a.a > a.b? a.a : -a.b; // left vs. right 
        offset.y = a.g > a.r? a.g : -a.r; // top vs. bottom

        // Then we go in the direction that has the maximum weight:
        if (abs(offset.x) > abs(offset.y)) // horizontal vs. vertical
            offset.y = 0.0;
        else
            offset.x = 0.0;

        #if SMAA_REPROJECTION == 1
        // Fetch the opposite color and lerp by hand:
        float4 C = SMAASampleLevelZero(colorTex, texcoord);
        texcoord += sign(offset) * SMAA_PIXEL_SIZE;
        float4 Cop = SMAASampleLevelZero(colorTex, texcoord);
        float s = abs(offset.x) > abs(offset.y)? abs(offset.x) : abs(offset.y);

        // Unpack the velocity values:
        C.a *= C.a;
        Cop.a *= Cop.a;

        // Lerp the colors:
        float4 Caa = SMAALerp(C, Cop, s);

        // Unpack velocity and return the resulting value:
        Caa.a = sqrt(Caa.a);
        gl_FragColor = Caa;
        #elif SMAA_HLSL_4 == 1 || SMAA_DIRECTX9_LINEAR_BLEND == 0
        // We exploit bilinear filtering to mix current pixel with the chosen
        // neighbor:
        texcoord += offset * SMAA_PIXEL_SIZE;
        gl_FragColor = SMAASampleLevelZero(colorTex, texcoord);
        #else
        // Fetch the opposite color and lerp by hand:
        float4 C = SMAASampleLevelZero(colorTex, texcoord);
        texcoord += sign(offset) * SMAA_PIXEL_SIZE;
        float4 Cop = SMAASampleLevelZero(colorTex, texcoord);
        float s = abs(offset.x) > abs(offset.y)? abs(offset.x) : abs(offset.y);
        gl_FragColor = SMAALerp(C, Cop, s);
        #endif
    }
}

void main( void )
{
    SMAANeighborhoodBlendingPS();
}