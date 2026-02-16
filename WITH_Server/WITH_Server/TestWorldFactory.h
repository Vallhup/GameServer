#pragma once

class TestWorldFactory final : public IWorldFactory {
public:
	virtual ~TestWorldFactory() = default;

	virtual std::unique_ptr<IWorldImpl> CreateImpl(const WorldDesc& desc) override;
};

