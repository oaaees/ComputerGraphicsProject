#version 410 core

in vec3 FragPos;

uniform vec3 lightPos;
uniform float far_plane;

void main()
{
    // Store linear distance from the light in the depth buffer
    float lightDistance = length(FragPos - lightPos);
    // gl_FragDepth expects [0,1]
    gl_FragDepth = lightDistance / far_plane;
}
