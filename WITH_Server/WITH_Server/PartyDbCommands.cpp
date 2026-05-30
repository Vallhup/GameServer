#include "pch.h"
#include "PartyDbCommands.h"

#include <algorithm>
#include <string>

#include "FrameworkLog.h"

namespace
{
	constexpr const char* kLogCategory = "PartyDb";

	// 멤버 한 명을 JSON object 한 칸으로 직렬화한다.
	// 값은 정수와 ISO8601(숫자/'-'/':'/'T'/'.') 뿐이라 escape가 필요 없다.
	void AppendMemberJson(std::wstring& out, const PartyPersistMemberRow& member)
	{
		out += L"{\"accountId\":";
		out += std::to_wstring(member.accountId);
		out += L",\"role\":";
		out += std::to_wstring(static_cast<uint32_t>(member.role));
		out += L",\"presence\":";
		out += std::to_wstring(static_cast<uint32_t>(member.presence));
		out += L",\"joinedAt\":\"";
		out += member.joinedAtUtcIso;
		out += L"\",\"lastSeenAt\":";
		if (member.lastSeenAtUtcIso.empty())
		{
			out += L"null";
		}
		else
		{
			out += L'\"';
			out += member.lastSeenAtUtcIso;
			out += L'\"';
		}
		out += L'}';
	}
}

// ---------------------------------------------------------------------------
// UpsertPartySnapshotCommand
// ---------------------------------------------------------------------------

UpsertPartySnapshotCommand::UpsertPartySnapshotCommand(
	PartyPersistData data,
	PartyDbResultQueue& resultSink)
	: _data(std::move(data))
	, _resultSink(resultSink)
{
}

std::wstring UpsertPartySnapshotCommand::BuildMembersJson() const
{
	std::wstring json;
	json += L'[';
	for (size_t i = 0; i < _data.members.size(); ++i)
	{
		if (i > 0)
		{
			json += L',';
		}
		AppendMemberJson(json, _data.members[i]);
	}
	json += L']';
	return json;
}

void UpsertPartySnapshotCommand::Execute(DBCommandContext& ctx) noexcept
{
	PartyDbResult result{};
	result.kind = PartyDbResultKind::PersistAck;
	result.partyId = _data.partyId;
	result.version = _data.version;

	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_UpsertPartySnapshot(?, ?, ?, ?, ?)}" },
		stmt))
	{
		_resultSink.Submit(std::move(result));
		return;
	}

	const std::wstring membersJson = BuildMembersJson();

	if (!stmt.BindInt64(1, static_cast<int64_t>(_data.partyId))          ||
		!stmt.BindInt64(2, static_cast<int64_t>(_data.leaderAccountId))  ||
		!stmt.BindInt16(3, static_cast<int16_t>(_data.lifecycle))        ||
		!stmt.BindInt64(4, static_cast<int64_t>(_data.version))          ||
		!stmt.BindString(5, membersJson, 4000)                           ||
		!stmt.Execute()                                                  ||
		!stmt.Fetch())
	{
		FWLOG_WARN(kLogCategory,
			"UpsertPartySnapshot failed (partyId=%llu, version=%llu): %ls",
			static_cast<unsigned long long>(_data.partyId),
			static_cast<unsigned long long>(_data.version),
			stmt.LastError().c_str());
		_resultSink.Submit(std::move(result));
		return;
	}

	int32_t staleFlag = 0;
	if (!stmt.GetInt32(1, staleFlag))
	{
		_resultSink.Submit(std::move(result));
		return;
	}

	result.success = true;
	result.stale = staleFlag != 0;
	_resultSink.Submit(std::move(result));
}

// ---------------------------------------------------------------------------
// SoftDeletePartyCommand
// ---------------------------------------------------------------------------

SoftDeletePartyCommand::SoftDeletePartyCommand(
	PartyId partyId,
	uint64_t version,
	PartyDbResultQueue& resultSink)
	: _partyId(partyId)
	, _version(version)
	, _resultSink(resultSink)
{
}

void SoftDeletePartyCommand::Execute(DBCommandContext& ctx) noexcept
{
	PartyDbResult result{};
	result.kind = PartyDbResultKind::SoftDeleteAck;
	result.partyId = _partyId;
	result.version = _version;

	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_SoftDeleteParty(?, ?)}" },
		stmt))
	{
		_resultSink.Submit(std::move(result));
		return;
	}

	if (!stmt.BindInt64(1, static_cast<int64_t>(_partyId)) ||
		!stmt.BindInt64(2, static_cast<int64_t>(_version)) ||
		!stmt.Execute()                                    ||
		!stmt.Fetch())
	{
		FWLOG_WARN(kLogCategory,
			"SoftDeleteParty failed (partyId=%llu, version=%llu): %ls",
			static_cast<unsigned long long>(_partyId),
			static_cast<unsigned long long>(_version),
			stmt.LastError().c_str());
		_resultSink.Submit(std::move(result));
		return;
	}

	int32_t staleFlag = 0;
	if (!stmt.GetInt32(1, staleFlag))
	{
		_resultSink.Submit(std::move(result));
		return;
	}

	result.success = true;
	result.stale = staleFlag != 0;
	_resultSink.Submit(std::move(result));
}

// ---------------------------------------------------------------------------
// LoadActivePartiesCommand
// ---------------------------------------------------------------------------

LoadActivePartiesCommand::LoadActivePartiesCommand(
	PartyDbResultQueue& resultSink)
	: _resultSink(resultSink)
{
}

void LoadActivePartiesCommand::Execute(DBCommandContext& ctx) noexcept
{
	PartyDbResult result{};
	result.kind = PartyDbResultKind::LoadActiveParties;

	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_LoadActiveParties}" },
		stmt))
	{
		_resultSink.Submit(std::move(result));
		return;
	}

	if (!stmt.Execute())
	{
		FWLOG_WARN(kLogCategory,
			"LoadActiveParties execute failed: %ls",
			stmt.LastError().c_str());
		_resultSink.Submit(std::move(result));
		return;
	}

	// SP는 party_id, role 순으로 정렬해 반환하므로 같은 파티의 행은 연속이다.
	// party_id 경계마다 group을 닫으면서 복구 invariant를 검증한다.
	RestoredParty current{};
	bool hasCurrent = false;
	bool currentValid = true;

	const auto flushCurrent = [&]()
	{
		if (!hasCurrent)
		{
			return;
		}

		const bool leaderPresent = std::any_of(
			current.members.begin(),
			current.members.end(),
			[&current](const RestoredPartyMember& member)
			{
				return member.accountId == current.leaderAccountId;
			});

		if (currentValid && leaderPresent && !current.members.empty())
		{
			result.restoredParties.push_back(std::move(current));
		}
		else
		{
			FWLOG_WARN(kLogCategory,
				"LoadActiveParties skipped party with broken snapshot "
				"(partyId=%llu, version=%llu, leaderPresent=%u, members=%zu)",
				static_cast<unsigned long long>(current.partyId),
				static_cast<unsigned long long>(current.version),
				leaderPresent ? 1u : 0u,
				current.members.size());
		}
	};

	while (stmt.Fetch())
	{
		int64_t partyIdRaw = 0;
		int64_t leaderAccountRaw = 0;
		int32_t lifecycleRaw = 0;
		int64_t versionRaw = 0;
		int64_t accountIdRaw = 0;
		int32_t roleRaw = 0;
		int32_t presenceRaw = 0;
		int64_t partyVersionRaw = 0;

		if (!stmt.GetInt64(1, partyIdRaw)       ||
			!stmt.GetInt64(2, leaderAccountRaw) ||
			!stmt.GetInt32(3, lifecycleRaw)     ||
			!stmt.GetInt64(4, versionRaw)       ||
			!stmt.GetInt64(5, accountIdRaw)     ||
			!stmt.GetInt32(6, roleRaw)          ||
			!stmt.GetInt32(7, presenceRaw)      ||
			!stmt.GetInt64(8, partyVersionRaw))
		{
			_resultSink.Submit(std::move(result));
			return;
		}

		const PartyId partyId = static_cast<PartyId>(partyIdRaw);
		if (!hasCurrent || partyId != current.partyId)
		{
			flushCurrent();

			current = RestoredParty{};
			current.partyId = partyId;
			current.leaderAccountId = static_cast<uint64_t>(leaderAccountRaw);
			current.lifecycle = static_cast<PartyLifecycleState>(lifecycleRaw);
			current.version = static_cast<uint64_t>(versionRaw);
			hasCurrent = true;
			currentValid = true;

			if (partyId > result.maxPartyId)
			{
				result.maxPartyId = partyId;
			}
		}

		// 찢어진 snapshot(Party.version != PartyMember.party_version)은 복구 제외.
		if (static_cast<uint64_t>(partyVersionRaw) != current.version)
		{
			currentValid = false;
		}

		current.members.push_back(RestoredPartyMember{
			.accountId = static_cast<uint64_t>(accountIdRaw),
			.role = static_cast<PartyMemberRole>(roleRaw)
		});
		(void)presenceRaw; // 복구 직후 presence는 항상 Offline으로 재초기화한다.
	}

	flushCurrent();

	result.success = true;
	_resultSink.Submit(std::move(result));
}

// ---------------------------------------------------------------------------
// GetMaxPartyIdCommand
// ---------------------------------------------------------------------------

GetMaxPartyIdCommand::GetMaxPartyIdCommand(PartyDbResultQueue& resultSink)
	: _resultSink(resultSink)
{
}

void GetMaxPartyIdCommand::Execute(DBCommandContext& ctx) noexcept
{
	PartyDbResult result{};
	result.kind = PartyDbResultKind::MaxPartyId;

	DBStatement stmt;
	if (!ctx.Prepare(
		DBQueryText{ L"{CALL dbo.sp_GetMaxPartyId}" },
		stmt))
	{
		_resultSink.Submit(std::move(result));
		return;
	}

	int64_t maxRaw = 0;
	if (!stmt.Execute() ||
		!stmt.Fetch()   ||
		!stmt.GetInt64(1, maxRaw))
	{
		FWLOG_WARN(kLogCategory,
			"GetMaxPartyId failed: %ls",
			stmt.LastError().c_str());
		_resultSink.Submit(std::move(result));
		return;
	}

	result.success = true;
	result.maxPartyId = static_cast<uint64_t>(maxRaw);
	_resultSink.Submit(std::move(result));
}
