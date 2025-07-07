#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace; 

out vec4 FragColor;

uniform sampler2D objTexture;
uniform sampler2D shadowMap;   
uniform bool useobjTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;   
    float currentDepth = projCoords.z;
    float bias = 0.001;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    if(projCoords.z > 1.0)
        shadow = 0.0;
    return shadow;
}

void main()
{
    vec3 lightColor = vec3(1.0, 1.0, 1.0);
    float lightIntensity = 0.85;
    
    vec4 textureColor;
    if (useobjTexture) {
        textureColor = texture(objTexture, TexCoords);
    }
    else {
        textureColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
    
    float ambientStrength = 0.8;
    vec3 ambient = ambientStrength * lightColor * lightIntensity;
    
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * lightIntensity * vec3(0.5);
    
    float specularStrength = 0.3;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor * lightIntensity * vec3(0.1);
    
    float shadow = ShadowCalculation(FragPosLightSpace);
    vec3 shadowColor = vec3(0.05, 0.05, 0.1);    
    float shadowIntensity = 0.9;                 

    vec3 lightContribution = diffuse + specular;
    vec3 shadingColor = mix(vec3(1.0), shadowColor, shadow * shadowIntensity);
    vec3 result = (ambient + lightContribution * shadingColor) * textureColor.rgb;
    result = mix(result * 0.8, result, 0.7);
    
    FragColor = vec4(result, textureColor.a);
}