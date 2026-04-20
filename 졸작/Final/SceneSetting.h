#pragma once

struct SceneLightSettings {
    XMFLOAT3 sunDirection = { -0.43f, -0.62f, -0.58f };
    XMFLOAT3 sunColor = { 1.0f, 1.0f, 1.0f };
    float    sunIntensity = 1.0f;

    XMFLOAT3 dir2Direction = { 0.0f, 0.0f, 1.0f };
    XMFLOAT3 dir2Color = { 1.0f, 1.0f, 1.0f };
    float    dir2Intensity = 0.0f;

    XMFLOAT3 pointPosition = { 481.f, 25.f, 482.0f };
    float    pointRange = 50.0f;
    XMFLOAT3 pointColor = { 1.0f, 1.0f, 1.0f };
    float    pointIntensity = 0.0f;
};

struct SceneLUTSettings {
    UINT  lutIndex = 14;
    float saturation = 1.0f;
};

struct SceneFogSettings {
    float    density = 0.03f;
    float    scattering = 0.8f;
    float    absorption = 0.1f;

    int      maxSteps = 32;
    float    maxDistance = 110.0f;
    float    jitterStrength = 1.0f;

    float    heightFalloff = 0.001f;
    float    groundHeight = 3.0f;

    float    hgAnisotropy = 0.6f;
    XMFLOAT3 lightColor = { 1.0f, 1.0f, 1.0f };
    float    lightIntensity = 1.0f;
};

struct SceneSkyboxSettings {
    XMFLOAT3 tintColor = { 1.0f, 1.0f, 1.0f };
    float    exposure = 1.0f;
    float    saturation = 1.0f;
};

struct SceneSettings {
    SceneLightSettings  light;
    SceneLUTSettings    lut;
    SceneFogSettings    fog;
    SceneSkyboxSettings skybox;
};
