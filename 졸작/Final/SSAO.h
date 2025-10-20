#pragma once
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

