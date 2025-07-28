#pragma once

class RootSignature
{
public:
	void Initialize(ID3D12Device* device);

	ID3D12RootSignature* Get() const;
private:
	ComPtr<ID3D12RootSignature> rootsignature;
};

