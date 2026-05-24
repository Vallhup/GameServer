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
	SessionId sessionId{ 0 };
	uint32_t latestRttMs{ 0 };
	uint32_t smoothedRttMs{ 0 };
	uint32_t rttVarMs{ 0 };
	uint32_t arrivalJitterMs{ 0 };
	uint32_t estimatedOneWayMs{ 0 };
	uint32_t sentProbeCount{ 0 };
	uint32_t receivedProbeCount{ 0 };
	uint32_t rejectedProbeCount{ 0 };
	uint32_t inputArrivalSampleCount{ 0 };
	bool initialized{ false };
	bool arrivalJitterInitialized{ false };
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

	void RecordPeriodicInputArrival(SessionId sessionId) noexcept;

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
		uint32_t lastPeriodicInputArrivalTimeMs{ 0 };
		double arrivalJitterMs{ 0.0 };
		uint32_t sentProbeCount{ 0 };
		uint32_t receivedProbeCount{ 0 };
		uint32_t rejectedProbeCount{ 0 };
		uint32_t inputArrivalSampleCount{ 0 };
		bool hasSentProbe{ false };
		bool initialized{ false };
		bool hasPeriodicInputArrival{ false };
		bool arrivalJitterInitialized{ false };
		std::array<ProbeRecord, kOutstandingProbeSlots> outstandingProbes{};
	};

private:
	static constexpr uint32_t kProbeIntervalMs = 500;
	static constexpr uint32_t kMaxAcceptedRttMs = 10000;
	static constexpr uint32_t kExpectedPeriodicInputIntervalMs = 33;
	static constexpr uint32_t kMaxAcceptedInputArrivalIntervalMs = 1000;
	static constexpr double kRttAlpha = 0.125;
	static constexpr double kJitterBeta = 0.25;
	static constexpr double kArrivalJitterAlpha = 0.125;

	static uint32_t NowMs() noexcept;
	static uint32_t ElapsedMs(uint32_t nowMs, uint32_t thenMs) noexcept;
	static uint32_t RoundToUInt32(double value) noexcept;
	static NetworkTimingSnapshot ToSnapshot(
		SessionId sessionId,
		const SessionTiming& timing) noexcept;
	static void UpdateRttEstimate(SessionTiming& timing, uint32_t sampleRttMs) noexcept;
	static void UpdateArrivalJitterEstimate(
		SessionTiming& timing,
		uint32_t arrivalIntervalMs) noexcept;
	static SessionTiming::ProbeRecord* FindProbeRecord(
		SessionTiming& timing,
		uint32_t probeSeq) noexcept;

	std::unordered_map<SessionId, SessionTiming> _sessions;
};
