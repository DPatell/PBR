#version 450
#extension GL_ARB_separate_shader_objects : enable

// NOTE(dhaval): Uniforms
layout(binding = 1) uniform sampler2D textureSampler;

// NOTE(dhaval): Input
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTextureCoordinate;

// NOTE(dhaval): Output
layout(location = 0) out vec4 outputColor;

void main() {
    outputColor = vec4(fragColor, 1.0) * texture(textureSampler, fragTextureCoordinate);
}