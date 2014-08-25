uniform sampler2D texture0; // noise texture

uniform vec2 fViewportDimensions;
uniform float fTime0_X;

uniform float u_intensity;
uniform float u_speed;       // multiplier for speed

varying vec2 v_texCoord;

vec4 tunnel(void)
{
   vec2 uv = v_texCoord - .5;
   uv.x *= fViewportDimensions.x / fViewportDimensions.y;
   float t = fTime0_X * 1.6323 * u_speed;
   float fov = 1.5 + sin(t);
   vec3 dir = -normalize(vec3(uv * fov, 1.0));
   vec3 from = vec3(0.0, 0.0, t * 5.0) * 0.005;
   float dist = -0.015; 
   vec3 vol = vec3(0.0);
   for (float v = 0.0; v < 25.0; v++) {
      dist += 0.00015;
      vec3 p = from + dist * dir * vec3(vec2(sign(dist)), 1.0);
      vec3 disp = texture2D(texture0, vec2(length(p.xy), p.z * 0.3) * 2.2568).xyz - 0.5;
      vol += pow(length(abs(0.5 - mod(p + disp * 0.013, vec3(0.01)) / 0.01)), 14.0)
         * (0.7 + normalize(disp) * 0.3) * exp(-500.0 * dist * dist);
   }
   vol = (vol + 0.2) * min(1.0, t - 0.05) * vec3(1.0, 0.8, 0.7);
   return vec4(pow(vol, vec3(1.3)), 1.0);
}

void main(void)
{
   gl_FragColor = tunnel() * u_intensity;
}