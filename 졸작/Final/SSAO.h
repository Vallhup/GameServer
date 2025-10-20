#pragma once

/*
1. DX12 core 클래스가 SSAO ptr 들고있기
->DX12 클래스 몸집 증가
->외부에서 ssao만 따로 참조할 경우가 생기는가 ? X

2. SSAO 클래스 따로 만들기
->RootSig나 Shader는 코드의 변동성이 존재하는데 SSAO는 그렇지 않을듯 함
->굳이 따로 빼야하나 ? 변동성 없는 코드들은 DX12Core에 묶어서 코딩햇는데 ?

=> 클래스 따로 빼는걸로, 나중에 Blur & Bloom 등도 마찬가지로 진행
*/

class SSAO
{
public:
	void Initialize(ID3D12Device* device);

	bool GetSSAOState() const;
	void SetSSAOState(bool in);

private:
	ComPtr<ID3D12Resource> ssaoTexture;
	bool enableSSAO = false;
};

