//-----------------------------------------------------------------------------
// Settings (should be filled in by code)
//#define SMAA_PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
//#define SMAA_PRESET_HIGH 1

varying vec2 texcoord;
varying float4 offset[2];

void SMAANeighborhoodBlendingVS()
{   
   offset[0] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4(-1.0, 0.0, 0.0, -1.0);
   offset[1] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4( 1.0, 0.0, 0.0,  1.0);
}

void main( void )
{
   // Clean up inaccuracies
   vec2 Pos = sign(gl_Vertex.xy);
   gl_Position = vec4(Pos.xy, 0, 1);
   // Image-space
   texcoord.x = 0.5 * (1.0 + Pos.x);
   texcoord.y = 0.5 * (1.0 + Pos.y); 
   
   SMAANeighborhoodBlendingVS();
}
