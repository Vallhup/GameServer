#pragma once

class AnimationSet;

class AnimationSetFactory
{
public:
	static shared_ptr<AnimationSet> CreateKnightSet();
	static shared_ptr<AnimationSet> CreateFinalBossSet();
	static shared_ptr<AnimationSet> CreateImpSet();
	static shared_ptr<AnimationSet> CreateDemonStrikerSet();
	static shared_ptr<AnimationSet> CreateDemonExecutionerSet();
	static shared_ptr<AnimationSet> CreateLancerSet();
	static shared_ptr<AnimationSet> CreateTankerSet();
};