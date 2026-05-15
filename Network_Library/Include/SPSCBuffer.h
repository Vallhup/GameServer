//#pragma once

#include <atomic>
#include <vector>
#include <type_traits>
#include <utility>

static constexpr inline bool IsPow2(size_t capacity)
{
	return capacity && ((capacity & (capacity - 1)) == 0);
}

template<typename T>
concept Copyable =
std::is_trivially_copyable_v<T> &&
std::is_trivially_destructible_v<T>;

template<Copyable T, size_t Capacity>
class SPSCBuffer {
	static_assert(IsPow2(Capacity), "Capacity must be power of two");
	static_assert(Capacity >= 2, "Capacity must be >= 2.");

	static constexpr size_t mask{ Capacity - 1 };

public:
	SPSCBuffer() : _buffer(Capacity) {}
	~SPSCBuffer() = default;

	SPSCBuffer(const SPSCBuffer&) = delete;
	SPSCBuffer& operator=(const SPSCBuffer&) = delete;

	bool TryPush(const T& item) 
	{ 
		// SP이므로 _head는 Producer 전용
		// -> Relaxed로 읽기 가능
		const size_t head = _head.value.load(std::memory_order_relaxed);
		const size_t tail = _tail.value.load(std::memory_order_acquire);

		if ((head - tail) >= Capacity)
			return false;

		_buffer[head & mask] = item;
		_head.value.store(head + 1, std::memory_order_release);

		return true;
	}

	bool TryPush(T&& item)
	{
		const size_t head = _head.value.load(std::memory_order_relaxed);
		const size_t tail = _tail.value.load(std::memory_order_acquire);

		if ((head - tail) >= Capacity)
			return false;

		_buffer[head & mask] = std::move(item);
		_head.value.store(head + 1, std::memory_order_release);

		return true;
	}

	bool TryPop(T& out)
	{
		// SC이므로 _tail는 Consumer 전용
		// -> Relaxed로 읽기 가능
		const size_t head = _head.value.load(std::memory_order_acquire);
		const size_t tail = _tail.value.load(std::memory_order_relaxed);

		if (head == tail)
			return false;

		out = std::move(_buffer[tail & mask]);
		_tail.value.store(tail + 1, std::memory_order_release);

		return true;
	}

	bool Empty() const 
	{
		return _tail.value.load(std::memory_order_acquire) == _head.value.load(std::memory_order_acquire);
	}

	bool Full() const
	{
		const size_t head = _head.value.load(std::memory_order_acquire);
		const size_t tail = _tail.value.load(std::memory_order_acquire);
		return (head - tail) >= Capacity;
	}

private:
	struct alignas(64) AlignedAtomicSizeT
	{
		std::atomic<size_t> value{ 0 };
	};

	AlignedAtomicSizeT _head{ 0 };
	AlignedAtomicSizeT _tail{ 0 };

	std::vector<T> _buffer;
};

// 참고자료/SPSC Queues 참고
//
// 1. 자료에서 말하는 "SPSC에서 중요한 원칙"
//  - 단일 Writer원칙을 지키면 원자 연산/락을 크게 줄일 수 있음
//  - 성능의 적은 "동기화 자체"보다 캐시/메모리 배리어/false sharing
// 
// 2. 구현의 출발점: Lamport 원형 버퍼
//  - 공유 배열: buf[CAP]
//  - 생산자 전용 인덱스: head(write index)
//  - 소비자 전용 인덱스: tail(read index)
//  - full/empty 판정: head와 tail의 거리로 판정
// 
// 3. 실전 최적화 포인트
//  1) 인덱스는 CacheLine 분리
//   - head와 tail이 같은 CacheLine에 있으면, 생산자/소비자간 false sharing 발생
// 
//  2) 공유 인덱스 읽기를 매번 하지 않도록 로컬 캐시를 둠
//   - 상대 인덱스(생산자는 tail, 소비자는 head)를 로컬에 캐시하고,
//     필요할 때만(거의 찼을 때) 다시 읽는 전략
// 
//  3) Capacity는 2의 거듭제곱으로 설정
//   - idx & (Capacity - 1) 마스킹으로 빠르게 인덱싱
//
//  4) 배리어는 "publish 지점"에만 최소화
//   - 생산자는 버퍼에 데이터를 다 쓴 뒤 head publish
//   - 소비자는 버퍼에서 데이터를 다 읽은 뒤 tail publish
// 
// 4. uSPSC(unbounded wait-free SPSC)로의 확장
//  - 고정 크기 링버퍼는 full되면 실패/드랍/백오프 정책 필요
//  - 동적 할당을 최소화하면서도 "사실상 무한"으로 동작하는 uSPSC 설계
// 
//  1) 작은 고정 크기 원형 버퍼를 chunk 단위로 연결
//  2) 대부분의 시간은 현재 chunk에서 동작
//  3) chunk full or empty 시에만 새 chunk로 이동
//  4) 동적 할당/해제를 블록 단위로 처리하여 최적화
//

// C++ Memory Order
// 
// 1. std::memory_order_relaxed
//  - 원자성만 보장
//  - 스레드 간 동기화/가시성/순서 보장 안함
//  - 통계 카운터, 성능 측정 등 "값 자체만 맞으면 되는 경우" 사용
// 
// 2. std::memory_order_release
//  - 쓰기 연산에 사용
//  - 해당 쓰기 이전의 모든 메모리 연산이 다른 스레드에서 관측될 수 있음
// 
// 3. std::memory_order_acquire
//  - 읽기 연산에 사용
//  - 해당 읽기 이후의 모든 atomic 메모리 연산이 이전에 관측된 메모리 연산을 볼 수 있음
// 
// 4. memory_order_acq_rel
//  - RMW연산에 사용
//  - acquire + release
// 
// 5. memory_order_seq_cst
//  - Default 모드
//  - 가장 강력한 순서 보장 제공