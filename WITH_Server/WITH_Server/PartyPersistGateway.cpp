#include "pch.h"
#include "PartyPersistGateway.h"

#include <cstdio>
#include <ctime>

#include "DBData.h"
#include "ODBCDatabaseBackend.h"
#include "PartyDbCommands.h"
#include "PartyService.h"

PartyPersistGateway::PartyPersistGateway(
	ODBCDatabaseBackend& database,
	PartyDbResultQueue& resultQueue,
	PartyService& partyService)
	: _database(database)
	, _resultQueue(resultQueue)
	, _partyService(partyService)
{
}

void PartyPersistGateway::SubmitStartupLoad()
{
	DBCommandEnvelope loadEnvelope{};
	loadEnvelope.command =
		std::make_unique<LoadActivePartiesCommand>(_resultQueue);
	(void)_database.Submit(std::move(loadEnvelope));

	DBCommandEnvelope maxIdEnvelope{};
	maxIdEnvelope.command =
		std::make_unique<GetMaxPartyIdCommand>(_resultQueue);
	(void)_database.Submit(std::move(maxIdEnvelope));
}

void PartyPersistGateway::PersistParty(
	const PartySnapshot& snapshot,
	double nowSec)
{
	if (snapshot.partyId == 0)
	{
		return;
	}

	const auto lastIt = _lastSubmittedVersion.find(snapshot.partyId);
	if (lastIt != _lastSubmittedVersion.end() &&
		snapshot.version <= lastIt->second)
	{
		// 이미 같거나 더 최신 버전을 제출했다. coalesce하여 중복 제출을 막는다.
		return;
	}

	_lastSubmittedVersion[snapshot.partyId] = snapshot.version;

	PendingJob job{};
	job.softDelete = false;
	job.version = snapshot.version;
	job.data = BuildPersistData(snapshot, nowSec);
	job.awaitingResult = true;

	_jobs[snapshot.partyId] = std::move(job);
	SubmitJob(_jobs[snapshot.partyId]);
}

void PartyPersistGateway::SoftDeleteParty(
	PartyId partyId,
	uint64_t version,
	double /*nowSec*/)
{
	if (partyId == 0)
	{
		return;
	}

	const auto lastIt = _lastSubmittedVersion.find(partyId);
	if (lastIt != _lastSubmittedVersion.end() &&
		version <= lastIt->second)
	{
		return;
	}

	_lastSubmittedVersion[partyId] = version;

	PendingJob job{};
	job.softDelete = true;
	job.version = version;
	job.data.partyId = partyId;
	job.data.version = version;
	job.awaitingResult = true;

	_jobs[partyId] = std::move(job);
	SubmitJob(_jobs[partyId]);
}

void PartyPersistGateway::PersistCurrentState(PartyId partyId, double nowSec)
{
	if (partyId == 0)
	{
		return;
	}

	const PartySnapshot snapshot = _partyService.BuildPartySnapshot(partyId);
	if (snapshot.partyId == 0)
	{
		return;
	}

	if (snapshot.lifecycle == PartyLifecycleState::Disbanded)
	{
		SoftDeleteParty(partyId, snapshot.version, nowSec);
	}
	else
	{
		PersistParty(snapshot, nowSec);
	}
}

void PartyPersistGateway::Process(double nowSec)
{
	_resultScratch.clear();
	_resultQueue.DrainInto(_resultScratch);
	for (const PartyDbResult& result : _resultScratch)
	{
		ApplyResult(result, nowSec);
	}

	for (auto& [partyId, job] : _jobs)
	{
		(void)partyId;
		if (!job.awaitingResult &&
			job.nextRetryAtSec > 0.0 &&
			job.nextRetryAtSec <= nowSec)
		{
			job.awaitingResult = true;
			job.nextRetryAtSec = 0.0;
			SubmitJob(job);
		}
	}
}

PartyPersistData PartyPersistGateway::BuildPersistData(
	const PartySnapshot& snapshot,
	double nowSec) const
{
	PartyPersistData data{};
	data.partyId = snapshot.partyId;
	data.version = snapshot.version;
	data.lifecycle = static_cast<uint8_t>(snapshot.lifecycle);

	for (const PartyMemberSnapshot& member : snapshot.members)
	{
		if (member.role == PartyMemberRole::Leader)
		{
			data.leaderAccountId = member.accountId;
			break;
		}
	}

	const std::chrono::system_clock::time_point systemNow =
		std::chrono::system_clock::now();

	data.members.reserve(snapshot.members.size());
	for (const PartyMemberSnapshot& member : snapshot.members)
	{
		const std::chrono::duration<double> joinedDelta{ nowSec - member.joinedAtSec };
		const std::chrono::system_clock::time_point joinedWall =
			systemNow -
			std::chrono::duration_cast<std::chrono::system_clock::duration>(joinedDelta);

		PartyPersistMemberRow row{};
		row.accountId = member.accountId;
		row.role = static_cast<uint8_t>(member.role);
		row.presence = static_cast<uint8_t>(member.presence);
		row.joinedAtUtcIso = FormatUtcIso8601(joinedWall);

		if (member.lastSeenAtSec > 0.0)
		{
			const std::chrono::duration<double> lastSeenDelta{ nowSec - member.lastSeenAtSec };
			const std::chrono::system_clock::time_point lastSeenWall =
				systemNow -
				std::chrono::duration_cast<std::chrono::system_clock::duration>(lastSeenDelta);
			row.lastSeenAtUtcIso = FormatUtcIso8601(lastSeenWall);
		}

		data.members.push_back(std::move(row));
	}

	return data;
}

void PartyPersistGateway::SubmitJob(const PendingJob& job)
{
	DBCommandEnvelope envelope{};
	if (job.softDelete)
	{
		envelope.command = std::make_unique<SoftDeletePartyCommand>(
			job.data.partyId,
			job.version,
			_resultQueue);
	}
	else
	{
		envelope.command = std::make_unique<UpsertPartySnapshotCommand>(
			job.data,
			_resultQueue);
	}

	(void)_database.Submit(std::move(envelope));
}

void PartyPersistGateway::ApplyResult(const PartyDbResult& result, double nowSec)
{
	switch (result.kind)
	{
	case PartyDbResultKind::LoadActiveParties:
	{
		for (const RestoredParty& restored : result.restoredParties)
		{
			(void)_partyService.RestorePartyFromSnapshot(restored, nowSec);
		}
		_partyService.AdvanceNextPartyIdTo(
			static_cast<PartyId>(result.maxPartyId));
		break;
	}
	case PartyDbResultKind::MaxPartyId:
	{
		_partyService.AdvanceNextPartyIdTo(
			static_cast<PartyId>(result.maxPartyId));
		break;
	}
	case PartyDbResultKind::PersistAck:
	case PartyDbResultKind::SoftDeleteAck:
	{
		const auto it = _jobs.find(result.partyId);
		if (it == _jobs.end() || it->second.version != result.version)
		{
			// 이미 더 최신 제출로 대체된 오래된 ack. 무시한다.
			break;
		}

		if (result.success)
		{
			// stale=true도 DB에 더 최신 상태가 있다는 뜻이므로 저장 완료로 본다.
			_jobs.erase(it);
		}
		else
		{
			PendingJob& job = it->second;
			job.awaitingResult = false;
			++job.retryCount;
			job.nextRetryAtSec = nowSec + ComputeBackoffSec(job.retryCount);
		}
		break;
	}
	}
}

double PartyPersistGateway::ComputeBackoffSec(uint32_t retryCount) noexcept
{
	switch (retryCount)
	{
	case 1:  return 1.0;
	case 2:  return 2.0;
	case 3:  return 5.0;
	case 4:  return 10.0;
	default: return 30.0;
	}
}

std::wstring PartyPersistGateway::FormatUtcIso8601(
	std::chrono::system_clock::time_point timePoint)
{
	const std::time_t seconds =
		std::chrono::system_clock::to_time_t(timePoint);

	const auto sinceEpoch = timePoint.time_since_epoch();
	const long long milliseconds =
		std::chrono::duration_cast<std::chrono::milliseconds>(sinceEpoch).count() % 1000;

	std::tm utc{};
	gmtime_s(&utc, &seconds);

	wchar_t buffer[32]{};
	std::swprintf(
		buffer,
		sizeof(buffer) / sizeof(buffer[0]),
		L"%04d-%02d-%02dT%02d:%02d:%02d.%03lld",
		utc.tm_year + 1900,
		utc.tm_mon + 1,
		utc.tm_mday,
		utc.tm_hour,
		utc.tm_min,
		utc.tm_sec,
		milliseconds < 0 ? -milliseconds : milliseconds);

	return std::wstring(buffer);
}
