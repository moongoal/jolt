#version 450

#include <jolt.h>

layout(push_constant) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = proj * view * model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
