varying vec2 texcoord;
varying vec4 offset [3];

void SMAALumaEdgeDetectionVS()
{
   offset[0] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4(-1.0, 0.0, 0.0, -1.0);
   offset[1] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4( 1.0, 0.0, 0.0,  1.0);
   offset[2] = texcoord.xyxy + SMAA_PIXEL_SIZE.xyxy * float4(-2.0, 0.0, 0.0, -2.0);
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
   
   SMAALumaEdgeDetectionVS();
}