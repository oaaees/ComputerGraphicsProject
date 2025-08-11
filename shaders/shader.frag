#version 410

in vec2 texture_coordinates;

out vec4 color;
uniform vec3 piece_color;
uniform sampler2D tex_sampler;

void main()
{
    vec4 tex_color = texture(tex_sampler, texture_coordinates);
    color = vec4(piece_color, 1.0) * tex_color;
}