#pragma once

#include <array>
#include <cstdint>
#include <cstddef>
#include <unordered_map>

#include "Session.h"

struct NetworkTimeProbe
{
	uint32_t probeSeq{ 0 };
	uint32_t serverSendTimeMs{ 0 };
	uint64_t serverFrame{ 0 };
};

struct NetworkTimingSnapshot
{
	uint32_t latestRttMs{ 0 };
	uint32_t smoothedRttMs{ 0 };
	uint32_t jitterMs{ 0 };
	uint32_t sentProbeCount{ 0 };
	uint32_t receivedProbeCount{ 0 };
	uint32_t rejectedProbeCount{ 0 };
	bool initialized{ false };
};

class NetworkTimingService final {
public:
	void Clear() noexcept;
	void RemoveSession(SessionId sessionId) noexcept;

	bool TryBuildProbe(
		SessionId sessionId,
		uint64_t serverFrame,
		NetworkTimeProbe& outProbe);

	void HandleClientEcho(
		SessionId sessionId,
		uint32_t probeSeq,
		uint32_t echoedServerSendTimeMs);

	bool TryGetSnapshot(
		SessionId sessionId,
		NetworkTimingSnapshot& outSnapshot) const noexcept;

private:
	struct SessionTiming
	{
		static constexpr size_t kOutstandingProbeSlots = 16;

		struct ProbeRecord
		{
			uint32_t probeSeq{ 0 };
			uint32_t sentTimeMs{ 0 };
			bool valid{ false };
		};

		uint32_t nextProbeSeq{ 1 };
		uint32_t lastProbeSeq{ 0 };
		uint32_t lastProbeSentTimeMs{ 0 };
		uint32_t latestRttMs{ 0 };
		double smoothedRttMs{ 0.0 };
		double rttVariationMs{ 0.0 };
		uint32_t sentProbeCount{ 0 };
		uint32_t receivedProbeCount{ 0 };
		uint32_t rejectedProbeCount{ 0 };
		bool hasSentProbe{ false };
		bool initialized{ false };
		std::array<ProbeRecord, kOutstandingProbeSlots> outstandingProbes{};
	};

private:
	static constexpr uint32_t kProbeIntervalMs = 500;
	static constexpr uint32_t kMaxAcceptedRttMs = 10000;
	static constexpr double kRttAlpha = 0.125;
	static constexpr double kJitterBeta = 0.25;

	static uint32_t NowMs() noexcept;
	static uint32_t ElapsedMs(uint32_t nowMs, uint32_t thenMs) noexcept;
	static uint32_t RoundToUInt32(double value) noexcept;
	static void UpdateRttEstimate(SessionTiming& timing, uint32_t sampleRttMs) noexcept;
	static SessionTiming::ProbeRecord* FindProbeRecord(
		SessionTiming& timing,
		uint32_t probeSeq) noexcept;

	std::unordered_map<SessionId, SessionTiming> _sessions;
};
