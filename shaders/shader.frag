#version 410

out vec4 color;
uniform vec3 piece_color;

void main()
{
    color = vec4(piece_color, 1.0);
}