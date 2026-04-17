#include "pch.h"
#include "AnimationSet.h"

AnimationSet::AnimationSet(const string& setName, uint32_t animStartIdx)
    : name(setName), startIdx(animStartIdx)
{
    clipNames.reserve(40);
}

void AnimationSet::RegisterClip(const string& clipName, int index,
    AnimCategory category, float blendDuration)
{
    ClipInfo info;
    info.index = index;
    info.category = category;
    info.blendDuration = blendDuration;

    clips[clipName] = info;
    clipNames.push_back(clipName);
}

const ClipInfo* AnimationSet::GetClip(const string& clipName) const
{
    auto it = clips.find(clipName);
    if (it != clips.end())
        return &it->second;
    return nullptr;
}

int AnimationSet::GetClipIndex(const string& clipName) const
{
    auto it = clips.find(clipName);
    if (it != clips.end())
        return it->second.index;
    return -1;
}

string AnimationSet::GetClipNameByIndex(int idx) const
{
    return clipNames[idx];
}

uint32_t AnimationSet::GetStartIndex() const
{
    return startIdx;
}

const unordered_map<string, ClipInfo>& AnimationSet::GetAllClips() const
{
    return clips;
}

string AnimationSet::GetName() const
{
    return name;
}
