uniform sampler2D colorTex;

#if SMAA_PREDICATION == 1
uniform sampler2D predicationTex;
#endif

varying vec2 texcoord;
varying vec4 offset [3];

void SMAALumaEdgeDetectionPS()
{
   // Calculate the threshold:
    #if SMAA_PREDICATION == 1
    float2 threshold = SMAACalculatePredicatedThreshold(texcoord, offset, colorTex, predicationTex);
    #else
    float2 threshold = float2(SMAA_THRESHOLD, SMAA_THRESHOLD);
    #endif
    
    // Calculate lumas:
    float3 weights = float3(0.2126, 0.7152, 0.0722);
    float L = dot(SMAASample(colorTex, texcoord).rgb, weights);
    float Lleft = dot(SMAASample(colorTex, offset[0].xy).rgb, weights);
    float Ltop  = dot(SMAASample(colorTex, offset[0].zw).rgb, weights);
    
    // We do the usual threshold:
    float4 delta;
    delta.xy = abs(L - float2(Lleft, Ltop));
    float2 edges = step(threshold, delta.xy);
    
     // Then discard if there is no edge:
    if (dot(edges, float2(1.0, 1.0)) == 0.0)
        discard;
        
    // Calculate right and bottom deltas:
    float Lright = dot(SMAASample(colorTex, offset[1].xy).rgb, weights);
    float Lbottom  = dot(SMAASample(colorTex, offset[1].zw).rgb, weights);
    delta.zw = abs(L - float2(Lright, Lbottom));
    
    // Calculate the maximum delta in the direct neighborhood:
    float2 maxDelta = max(delta.xy, delta.zw);
    maxDelta = max(maxDelta.xx, maxDelta.yy);

    // Calculate left-left and top-top deltas:
    float Lleftleft = dot(SMAASample(colorTex, offset[2].xy).rgb, weights);
    float Ltoptop = dot(SMAASample(colorTex, offset[2].zw).rgb, weights);
    delta.zw = abs(float2(Lleft, Ltop) - float2(Lleftleft, Ltoptop));

    // Calculate the final maximum delta:
    maxDelta = max(maxDelta.xy, delta.zw);

    /**
     * Each edge with a delta in luma of less than 50% of the maximum luma
     * surrounding this pixel is discarded. This allows to eliminate spurious
     * crossing edges, and is based on the fact that, if there is too much
     * contrast in a direction, that will hide contrast in the other
     * neighbors.
     * This is done after the discard intentionally as this situation doesn't
     * happen too frequently (but it's important to do as it prevents some 
     * edges from going undetected).
     */
    edges.xy *= step(0.5 * maxDelta, delta.xy);

    gl_FragColor = float4(edges, 0.0, 0.0); 
}

void main( void )
{    
   SMAALumaEdgeDetectionPS();
}