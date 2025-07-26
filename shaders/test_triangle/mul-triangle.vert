#version 450

layout(binding = 0) uniform TransformationCB
{
    mat4 transformation;
    mat4 view;
    mat4 projection;
} tcb;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;


void main()
{
    gl_Position = tcb.projection * tcb.view * tcb.transformation * vec4(inPosition, 0.0f, 1.0f);
    outColor = inColor;
}
