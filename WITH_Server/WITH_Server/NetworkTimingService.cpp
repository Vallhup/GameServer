#include "pch.h"
#include "NetworkTimingService.h"

#include <algorithm>
#include <chrono>
#include <cmath>

void NetworkTimingService::Clear() noexcept
{
	_sessions.clear();
}

void NetworkTimingService::RemoveSession(SessionId sessionId) noexcept
{
	_sessions.erase(sessionId);
}

bool NetworkTimingService::TryBuildProbe(
	SessionId sessionId,
	uint64_t serverFrame,
	NetworkTimeProbe& outProbe)
{
	outProbe = {};
	if (sessionId == 0)
	{
		return false;
	}

	const uint32_t nowMs = NowMs();
	SessionTiming& timing = _sessions[sessionId];
	if (timing.hasSentProbe &&
		ElapsedMs(nowMs, timing.lastProbeSentTimeMs) < kProbeIntervalMs)
	{
		return false;
	}

	if (timing.nextProbeSeq == 0)
	{
		timing.nextProbeSeq = 1;
	}

	timing.lastProbeSeq = timing.nextProbeSeq++;
	timing.lastProbeSentTimeMs = nowMs;
	timing.hasSentProbe = true;
	++timing.sentProbeCount;

	SessionTiming::ProbeRecord& record =
		timing.outstandingProbes[
			timing.lastProbeSeq % timing.outstandingProbes.size()];
	record.probeSeq = timing.lastProbeSeq;
	record.sentTimeMs = nowMs;
	record.valid = true;

	outProbe.probeSeq = timing.lastProbeSeq;
	outProbe.serverSendTimeMs = nowMs;
	outProbe.serverFrame = serverFrame;
	return true;
}

void NetworkTimingService::HandleClientEcho(
	SessionId sessionId,
	uint32_t probeSeq,
	uint32_t echoedServerSendTimeMs)
{
	if (sessionId == 0)
	{
		return;
	}

	SessionTiming& timing = _sessions[sessionId];
	if (probeSeq == 0 ||
		(timing.lastProbeSeq != 0 && probeSeq > timing.lastProbeSeq))
	{
		++timing.rejectedProbeCount;
		return;
	}

	SessionTiming::ProbeRecord* const record =
		FindProbeRecord(timing, probeSeq);
	if (record == nullptr ||
		record->sentTimeMs != echoedServerSendTimeMs)
	{
		++timing.rejectedProbeCount;
		return;
	}

	const uint32_t sampleRttMs = ElapsedMs(NowMs(), record->sentTimeMs);
	if (sampleRttMs > kMaxAcceptedRttMs)
	{
		++timing.rejectedProbeCount;
		return;
	}

	UpdateRttEstimate(timing, sampleRttMs);
	record->valid = false;
	++timing.receivedProbeCount;
}

bool NetworkTimingService::TryGetSnapshot(
	SessionId sessionId,
	NetworkTimingSnapshot& outSnapshot) const noexcept
{
	outSnapshot = {};

	const auto it = _sessions.find(sessionId);
	if (it == _sessions.end())
	{
		return false;
	}

	outSnapshot = ToSnapshot(sessionId, it->second);
	return true;
}

uint32_t NetworkTimingService::NowMs() noexcept
{
	using Clock = std::chrono::steady_clock;

	static const Clock::time_point startTime = Clock::now();
	const uint64_t elapsedMs =
		static_cast<uint64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				Clock::now() - startTime).count());
	return static_cast<uint32_t>(elapsedMs);
}

uint32_t NetworkTimingService::ElapsedMs(
	uint32_t nowMs,
	uint32_t thenMs) noexcept
{
	return nowMs - thenMs;
}

uint32_t NetworkTimingService::RoundToUInt32(double value) noexcept
{
	return static_cast<uint32_t>(std::max(0.0, std::round(value)));
}

NetworkTimingSnapshot NetworkTimingService::ToSnapshot(
	SessionId sessionId,
	const SessionTiming& timing) noexcept
{
	NetworkTimingSnapshot snapshot{};
	snapshot.sessionId = sessionId;
	snapshot.latestRttMs = timing.latestRttMs;
	snapshot.smoothedRttMs = RoundToUInt32(timing.smoothedRttMs);
	snapshot.rttVarMs = RoundToUInt32(timing.rttVariationMs);
	snapshot.estimatedOneWayMs =
		RoundToUInt32(timing.smoothedRttMs * 0.5);
	snapshot.sentProbeCount = timing.sentProbeCount;
	snapshot.receivedProbeCount = timing.receivedProbeCount;
	snapshot.rejectedProbeCount = timing.rejectedProbeCount;
	snapshot.initialized = timing.initialized;
	return snapshot;
}

void NetworkTimingService::UpdateRttEstimate(
	SessionTiming& timing,
	uint32_t sampleRttMs) noexcept
{
	timing.latestRttMs = sampleRttMs;

	if (!timing.initialized)
	{
		timing.smoothedRttMs = static_cast<double>(sampleRttMs);
		timing.rttVariationMs = static_cast<double>(sampleRttMs) * 0.5;
		timing.initialized = true;
		return;
	}

	const double sample = static_cast<double>(sampleRttMs);
	const double deviation = std::abs(timing.smoothedRttMs - sample);
	timing.rttVariationMs =
		((1.0 - kJitterBeta) * timing.rttVariationMs) + (kJitterBeta * deviation);
	timing.smoothedRttMs =
		((1.0 - kRttAlpha) * timing.smoothedRttMs) + (kRttAlpha * sample);
}

NetworkTimingService::SessionTiming::ProbeRecord*
NetworkTimingService::FindProbeRecord(
	SessionTiming& timing,
	uint32_t probeSeq) noexcept
{
	SessionTiming::ProbeRecord& record =
		timing.outstandingProbes[probeSeq % timing.outstandingProbes.size()];
	if (!record.valid || record.probeSeq != probeSeq)
	{
		return nullptr;
	}

	return &record;
}
