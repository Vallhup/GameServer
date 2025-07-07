#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;

const int MAX_BONES = 100;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform mat4 gBones[MAX_BONES];

void main()
{
    vec4 totalPosition = vec4(0.0);
    for(int i = 0; i < 4; i++) {
        if(BoneIDs[i] != -1) {
            totalPosition += Weights[i] * (gBones[BoneIDs[i]] * vec4(aPos, 1.0));
        }
    }
    gl_Position = lightSpaceMatrix * model * totalPosition;
}