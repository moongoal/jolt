#version 450

#include <jolt.h>

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec2 inTexCoord;
layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor * texture(texSampler, inTexCoord).rgba);
    //outColor = vec4(inTexCoord, 1.0, 1.0);
}
