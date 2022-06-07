#version 450
#extension GL_ARB_separate_shader_objects:enable

// NOTE(dhaval): Uniforms
layout(binding=0)uniform uniform_buffer_object{
    mat4 model;
    mat4 view;
    mat4 projection;
}ubo;

// NOTE(dhaval): Input
layout(location=0)in vec3 inPosition;
layout(location=1)in vec3 inColor;
layout(location=2)in vec2 inTextureCoordinate;

// NOTE(dhaval): Output
layout(location=0)out vec3 fragColor;
layout(location=1)out vec2 fragTextureCoordinate;

void main(){
    gl_Position=ubo.projection*ubo.view*ubo.model*vec4(inPosition,1.);
    fragColor=inColor;
    fragTextureCoordinate=inTextureCoordinate;
}