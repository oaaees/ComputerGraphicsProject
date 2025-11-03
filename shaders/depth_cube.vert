#version 410 core

layout(location = 0) in vec3 position;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 FragPos;

void main()
{
    vec4 worldPos = model * vec4(position, 1.0);
    FragPos = worldPos.xyz;
    gl_Position = projection * view * worldPos;
}
