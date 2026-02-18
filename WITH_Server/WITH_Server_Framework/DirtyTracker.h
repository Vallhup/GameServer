#pragma once

// 엔티티별 DirtyMask 기록 후 변경사항 추적

enum DirtyBit : uint32
{
	DIRTY_TRANSFORM = 1 << 0,
	DIRTY_ANIMATION = 1 << 1,
};


class DirtyTracker {
public:
	void Mark(uint64 id, uint32 mask) { _dirty[id] |= mask; }

	const auto& View() const { return _dirty; }

	void Clear() { _dirty.clear(); }

private:
	std::unordered_map<uint64, uint32> _dirty;
};

// 1. TCP Framing
// 2. NetId, SpawnId, ReplicatedTag 도입
// 3. RepRegistry로 Transform 복제
// 4. SessionRepState 기반 Spawn / Despawn 자동화
// 5. DirtyTracker로 Transform 자동화
// 6. InterestPolicy 도입
// 7. 세션 당 byte budget + 프레임 분할
// 8. lastSent 기반 Delta
// 9. 클라 보간/스냅샷 버퍼
//
//