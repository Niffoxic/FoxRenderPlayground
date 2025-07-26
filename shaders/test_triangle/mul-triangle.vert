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
    float r = length(inPosition);
    float angle = atan(inPosition.y, inPosition.x);

    float pulse = sin(tcb.timeElapsed * 6.0 + r * 20.0 + angle * 10.0) * 0.25;
    float dynamicRadius = r + pulse;

    float spin = tcb.timeElapsed * 2.0;
    float dynamicAngle = angle + sin(tcb.timeElapsed + r * 15.0) * 1.0 + spin;

    vec2 vortexPos = vec2(cos(dynamicAngle), sin(dynamicAngle)) * dynamicRadius;

    float breathingScale = 1.0 + 0.15 * sin(tcb.timeElapsed * 2.0);
    vortexPos *= breathingScale;

    vec3 finalPos = vec3(vortexPos, 0.0);
    gl_Position = tcb.projection * tcb.view * tcb.transformation * vec4(finalPos, 1.0);

    outColor = inColor;
}
