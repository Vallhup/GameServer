#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 BoneIDs;
layout (location = 4) in vec4 Weights;

out vec2 TexCoords;
out vec3 FragPos;    
out vec3 Normal;    
out vec4 FragPosLightSpace;

const int MAX_BONES = 100;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 gBones[MAX_BONES];
uniform mat4 lightSpaceMatrix;

void main()
{
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    for(int i = 0; i < 4; i++) {
        if(BoneIDs[i] != -1) {
            totalPosition += Weights[i] * (gBones[BoneIDs[i]] * vec4(aPos, 1.0));
            totalNormal += Weights[i] * mat3(gBones[BoneIDs[i]]) * aNormal;
        }
    }
    
    FragPos = vec3(model * totalPosition);
    
    Normal = mat3(transpose(inverse(model))) * totalNormal;
    
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    
    gl_Position = projection * view * model * totalPosition;
    TexCoords = aTexCoords;
}