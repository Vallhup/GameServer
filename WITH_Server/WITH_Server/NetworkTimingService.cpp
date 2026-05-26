#include "pch.h"
#include "NetworkTimingService.h"

#include <algorithm>
#include <chrono>
#include <cmath>

#include "FrameworkLog.h"

namespace
{
	constexpr const char* kLogCategory = "NetworkTiming";
}

void NetworkTimingService::Clear() noexcept
{
	FWLOG_INFO(kLogCategory,
		"NetworkTimingService::Clear (sessionCount=%zu)",
		_sessions.size());
	_sessions.clear();
}

void NetworkTimingService::RemoveSession(SessionId sessionId) noexcept
{
	const auto it = _sessions.find(sessionId);
	const bool existed = it != _sessions.end();
	FWLOG_INFO(kLogCategory,
		"NetworkTimingService::RemoveSession (sid=%u, existed=%u, sentProbeCount=%u, receivedProbeCount=%u, rejectedProbeCount=%u)",
		sessionId,
		existed ? 1u : 0u,
		existed ? it->second.sentProbeCount : 0u,
		existed ? it->second.receivedProbeCount : 0u,
		existed ? it->second.rejectedProbeCount : 0u);
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
		FWLOG_WARN(kLogCategory,
			"TryBuildProbe rejected: sessionId=0 (serverFrame=%llu)",
			static_cast<unsigned long long>(serverFrame));
		return false;
	}

	const uint32_t nowMs = NowMs();
	// 호출 측(예: ServerSessionSystem::StageTimeSyncPackets)이 세션 상태
	// 게이트를 통과시킨 세션만 넘기는 것을 전제로 하며, 이 함수가 송신
	// 진입점이므로 여기서만 명시적으로 엔트리를 생성한다.
	const auto emplaceResult = _sessions.try_emplace(sessionId);
	SessionTiming& timing = emplaceResult.first->second;
	const bool newlyCreated = emplaceResult.second;
	if (newlyCreated)
	{
		FWLOG_INFO(kLogCategory,
			"TryBuildProbe created new SessionTiming (sid=%u, nowMs=%u, sessionCount=%zu)",
			sessionId,
			nowMs,
			_sessions.size());
	}

	if (timing.hasSentProbe &&
		ElapsedMs(nowMs, timing.lastProbeSentTimeMs) < kProbeIntervalMs)
	{
		FWLOG_INFO(kLogCategory,
			"TryBuildProbe skipped: interval not elapsed (sid=%u, nowMs=%u, lastProbeSentTimeMs=%u, elapsedMs=%u, intervalMs=%u)",
			sessionId,
			nowMs,
			timing.lastProbeSentTimeMs,
			ElapsedMs(nowMs, timing.lastProbeSentTimeMs),
			kProbeIntervalMs);
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

	FWLOG_INFO(kLogCategory,
		"TryBuildProbe built (sid=%u, probeSeq=%u, serverSendTimeMs=%u, serverFrame=%llu, sentProbeCount=%u)",
		sessionId,
		outProbe.probeSeq,
		outProbe.serverSendTimeMs,
		static_cast<unsigned long long>(outProbe.serverFrame),
		timing.sentProbeCount);
	return true;
}

void NetworkTimingService::HandleClientEcho(
	SessionId sessionId,
	uint32_t probeSeq,
	uint32_t echoedServerSendTimeMs)
{
	if (sessionId == 0)
	{
		FWLOG_WARN(kLogCategory,
			"HandleClientEcho rejected: sessionId=0 (probeSeq=%u, echoedServerSendTimeMs=%u)",
			probeSeq,
			echoedServerSendTimeMs);
		return;
	}

	// echo는 응답이므로, 서버가 probe를 발사한 적이 없는 세션의 응답은
	// 무시한다. 여기서 _sessions[]를 사용하면 위조된 sessionId만으로도
	// 엔트리를 만들 수 있어 메모리를 키울 수 있다.
	const auto it = _sessions.find(sessionId);
	if (it == _sessions.end())
	{
		FWLOG_WARN(kLogCategory,
			"HandleClientEcho rejected: unknown session (sid=%u, probeSeq=%u, echoedServerSendTimeMs=%u)",
			sessionId,
			probeSeq,
			echoedServerSendTimeMs);
		return;
	}

	SessionTiming& timing = it->second;
	if (probeSeq == 0 ||
		(timing.lastProbeSeq != 0 && probeSeq > timing.lastProbeSeq))
	{
		FWLOG_WARN(kLogCategory,
			"HandleClientEcho rejected: bad probeSeq (sid=%u, probeSeq=%u, lastProbeSeq=%u, echoedServerSendTimeMs=%u, rejectedProbeCount=%u)",
			sessionId,
			probeSeq,
			timing.lastProbeSeq,
			echoedServerSendTimeMs,
			timing.rejectedProbeCount + 1);
		++timing.rejectedProbeCount;
		return;
	}

	SessionTiming::ProbeRecord* const record =
		FindProbeRecord(timing, probeSeq);
	if (record == nullptr ||
		record->sentTimeMs != echoedServerSendTimeMs)
	{
		FWLOG_WARN(kLogCategory,
			"HandleClientEcho rejected: probe record mismatch (sid=%u, probeSeq=%u, recordFound=%u, recordSentTimeMs=%u, echoedServerSendTimeMs=%u)",
			sessionId,
			probeSeq,
			record != nullptr ? 1u : 0u,
			record != nullptr ? record->sentTimeMs : 0u,
			echoedServerSendTimeMs);
		++timing.rejectedProbeCount;
		return;
	}

	const uint32_t sampleRttMs = ElapsedMs(NowMs(), record->sentTimeMs);
	if (sampleRttMs > kMaxAcceptedRttMs)
	{
		FWLOG_WARN(kLogCategory,
			"HandleClientEcho rejected: RTT too large (sid=%u, probeSeq=%u, sampleRttMs=%u, kMaxAcceptedRttMs=%u)",
			sessionId,
			probeSeq,
			sampleRttMs,
			kMaxAcceptedRttMs);
		++timing.rejectedProbeCount;
		return;
	}

	UpdateRttEstimate(timing, sampleRttMs);
	record->valid = false;
	++timing.receivedProbeCount;
	FWLOG_INFO(kLogCategory,
		"HandleClientEcho accepted (sid=%u, probeSeq=%u, sampleRttMs=%u, smoothedRttMs=%u, rttVarMs=%u, receivedProbeCount=%u)",
		sessionId,
		probeSeq,
		sampleRttMs,
		RoundToUInt32(timing.smoothedRttMs),
		RoundToUInt32(timing.rttVariationMs),
		timing.receivedProbeCount);
}

void NetworkTimingService::RecordPeriodicInputArrival(
	SessionId sessionId) noexcept
{
	if (sessionId == 0)
	{
		FWLOG_WARN(kLogCategory,
			"RecordPeriodicInputArrival rejected: sessionId=0");
		return;
	}

	const uint32_t nowMs = NowMs();
	// CS_MOVE 핸들러는 SessionCurrentWorld로 등록되어 있어 InGame 세션에서만
	// 호출된다. 송신 진입점(TryBuildProbe)과 동일한 emplace 패턴을 명시적으로
	// 사용하여 의도된 엔트리 생성 경로를 좁힌다.
	const auto emplaceResult = _sessions.try_emplace(sessionId);
	SessionTiming& timing = emplaceResult.first->second;
	const bool newlyCreated = emplaceResult.second;
	if (newlyCreated)
	{
		FWLOG_INFO(kLogCategory,
			"RecordPeriodicInputArrival created new SessionTiming (sid=%u, nowMs=%u, sessionCount=%zu)",
			sessionId,
			nowMs,
			_sessions.size());
	}

	if (!timing.hasPeriodicInputArrival)
	{
		timing.lastPeriodicInputArrivalTimeMs = nowMs;
		timing.hasPeriodicInputArrival = true;
		FWLOG_INFO(kLogCategory,
			"RecordPeriodicInputArrival first sample (sid=%u, nowMs=%u)",
			sessionId,
			nowMs);
		return;
	}

	const uint32_t arrivalIntervalMs =
		ElapsedMs(nowMs, timing.lastPeriodicInputArrivalTimeMs);
	timing.lastPeriodicInputArrivalTimeMs = nowMs;

	if (arrivalIntervalMs > kMaxAcceptedInputArrivalIntervalMs)
	{
		FWLOG_WARN(kLogCategory,
			"RecordPeriodicInputArrival dropped: interval too large (sid=%u, arrivalIntervalMs=%u, kMaxAcceptedInputArrivalIntervalMs=%u)",
			sessionId,
			arrivalIntervalMs,
			kMaxAcceptedInputArrivalIntervalMs);
		return;
	}

	UpdateArrivalJitterEstimate(timing, arrivalIntervalMs);
	FWLOG_INFO(kLogCategory,
		"RecordPeriodicInputArrival sampled (sid=%u, arrivalIntervalMs=%u, arrivalJitterMs=%u, inputArrivalSampleCount=%u)",
		sessionId,
		arrivalIntervalMs,
		RoundToUInt32(timing.arrivalJitterMs),
		timing.inputArrivalSampleCount);
}

bool NetworkTimingService::TryGetSnapshot(
	SessionId sessionId,
	NetworkTimingSnapshot& outSnapshot) const noexcept
{
	outSnapshot = {};

	const auto it = _sessions.find(sessionId);
	if (it == _sessions.end())
	{
		FWLOG_WARN(kLogCategory,
			"TryGetSnapshot miss (sid=%u, sessionCount=%zu)",
			sessionId,
			_sessions.size());
		return false;
	}

	outSnapshot = ToSnapshot(sessionId, it->second);
	FWLOG_INFO(kLogCategory,
		"TryGetSnapshot hit (sid=%u, latestRttMs=%u, smoothedRttMs=%u, rttVarMs=%u, arrivalJitterMs=%u, initialized=%u, arrivalJitterInitialized=%u)",
		sessionId,
		outSnapshot.latestRttMs,
		outSnapshot.smoothedRttMs,
		outSnapshot.rttVarMs,
		outSnapshot.arrivalJitterMs,
		outSnapshot.initialized ? 1u : 0u,
		outSnapshot.arrivalJitterInitialized ? 1u : 0u);
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
	snapshot.arrivalJitterMs = RoundToUInt32(timing.arrivalJitterMs);
	snapshot.estimatedOneWayMs =
		RoundToUInt32(timing.smoothedRttMs * 0.5);
	snapshot.sentProbeCount = timing.sentProbeCount;
	snapshot.receivedProbeCount = timing.receivedProbeCount;
	snapshot.rejectedProbeCount = timing.rejectedProbeCount;
	snapshot.inputArrivalSampleCount = timing.inputArrivalSampleCount;
	snapshot.initialized = timing.initialized;
	snapshot.arrivalJitterInitialized = timing.arrivalJitterInitialized;
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

void NetworkTimingService::UpdateArrivalJitterEstimate(
	SessionTiming& timing,
	uint32_t arrivalIntervalMs) noexcept
{
	const double sample =
		std::abs(
			static_cast<double>(arrivalIntervalMs) -
			static_cast<double>(kExpectedPeriodicInputIntervalMs));

	if (!timing.arrivalJitterInitialized)
	{
		timing.arrivalJitterMs = sample;
		timing.arrivalJitterInitialized = true;
		++timing.inputArrivalSampleCount;
		return;
	}

	timing.arrivalJitterMs =
		((1.0 - kArrivalJitterAlpha) * timing.arrivalJitterMs) +
		(kArrivalJitterAlpha * sample);
	++timing.inputArrivalSampleCount;
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
