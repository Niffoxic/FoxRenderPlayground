#version 450

layout(binding = 0) uniform TransformationCB
{
    mat4  transformation;
    mat4  view;
    mat4  projection;
    float timeElapsed;
    vec3  padding;
} tcb;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 outColor;

void main()
{
    // --- Animate position with time-based sine wave ---
    float wave = sin(tcb.timeElapsed * 2.0 + inPosition.x * 10.0) * 0.05;

    // Apply the wave to Y position for a ripple-like bounce
    vec3 animatedPos = vec3(inPosition.x, inPosition.y + wave, 0.0);

    // Standard MVP transform
    gl_Position = tcb.projection * tcb.view * tcb.transformation * vec4(animatedPos, 1.0);
    outColor = inColor;
}
