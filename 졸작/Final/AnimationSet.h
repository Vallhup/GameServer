#pragma once

// Base : Idle / Walk / Run - 도중 전환 가능
// Action : Attack / Dodge / Parry / Stun / Hit - 도중 전환 불가능 (애니메이션 시작시 한 주기는 무조건 끝까지 실행)
// Special : Guard / Drinking
// Die : Death

enum class AnimCategory {
    Base,
    Action,
    Special,
    Die
};

struct ClipInfo
{
    int index = -1;              
    AnimCategory category = AnimCategory::Base;
    float blendDuration = 0.2f;  
};

class AnimationSet
{
public:
    AnimationSet(const string& setName);

    void RegisterClip(const string& clipName, int index,
        AnimCategory category = AnimCategory::Base, float blendDuration = 0.2f);

    const ClipInfo* GetClip(const string& clipName) const;
    int GetClipIndex(const string& clipName) const;

    const unordered_map<string, ClipInfo>& GetAllClips() const;

    string GetName() const;

private:
    string name;
    unordered_map<string, ClipInfo> clips;
};
