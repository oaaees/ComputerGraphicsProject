#version 410

in vec2 texture_coordinates;
in vec3 Normal;

out vec4 color;

uniform sampler2D texture_sampler;

void main()
{
    color = texture(texture_sampler, texture_coordinates);
}
