//-----------------------------------------------------------------------------
// Settings (should be filled in by code)
//#define SMAA_PIXEL_SIZE float2(1.0 / 1280.0, 1.0 / 720.0)
//#define SMAA_PRESET_HIGH 1


varying vec2 texcoord;
varying vec2 pixcoord;
varying vec4 offset0;
varying vec4 offset1;
varying vec4 offset2;

void SMAABlendingWeightCalculationVS()
{
   pixcoord = texcoord / SMAA_PIXEL_SIZE;

   // We will use these offsets for the searches later on (see @PSEUDO_GATHER4):
   offset0 = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4(-0.25, -0.125,  1.25, -0.125);
   offset1 = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4(-0.125, -0.25, -0.125,  1.25);

   // And these for the searches, they indicate the ends of the loops:
   offset2 = float4(offset0.xz, offset1.yw) + 
               float4(-2.0, 2.0, -2.0, 2.0) *
               SMAA_PIXEL_SIZE.xxyy * float(SMAA_MAX_SEARCH_STEPS);
}

void main( void )
{
   // Clean up inaccuracies
   vec2 Pos = sign(gl_Vertex.xy);
   gl_Position = vec4(Pos.xy, 0, 1);
   // Image-space
   float2 ps = SMAA_PIXEL_SIZE * 0.5;
   texcoord.x = (0.5 * (1.0 + Pos.x)) - ps.x;
   texcoord.y = (0.5 * (1.0 + Pos.y)) - ps.y; 
   
   SMAABlendingWeightCalculationVS();
}