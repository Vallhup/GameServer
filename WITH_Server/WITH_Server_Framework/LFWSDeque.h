#pragma once

#include <atomic>
#include <optional>
#include <cstdint>
#include <cassert> 

/*-----------------------------------------------------------------------------------
 Dynamic Circular Work-Stealing Deque (Chase & Lev, SPAA 2005)
 Correct and Efficient Work-Stealing for Weak Memory Models (Le et al., PPPoPP 2013)
 taskflow/work-stealing-queue for C++17

 위 2개의 논문과 1개의 구현체를 참고하여 Lock-Free Work-Stealing Deque 구현 시도
------------------------------------------------------------------------------------*/

template<typename T, int64_t Capacity>
class LFWSDeque {
	// Capacity는 반드시 2의 거듭제곱이어야 함 (마스킹 연산)
	//
	// Chase & Lev 원본은 동적 배열을 지원하기 때문에
	// grow(배열 교체) 로직과 메모리 회수 문제를 해결해야 하지만,
	// 현재는 DAG 빌드 시점에 ExecNode 수가 확정되기 때문에
	// 고정 크기 배열로 구현하여 해당 문제를 없앨 수 있음
	static constexpr int64_t kMask{ Capacity - 1 };
	static_assert((Capacity & kMask) == 0, "Capacity must be power of 2");

public:
	[[nodiscard]] bool TryPush(const T& item);
	[[nodiscard]] bool TryPush(T&& item);

	std::optional<T> TryPop();
	std::optional<T> TrySteal();

	// 모든 경쟁 스레드가 종료된 상태에서만 호출해야 한다.
	// 프레임 경계에서 deque를 초기화할 때 사용한다.
	void Reset() noexcept;

private:
	// top은 stealer들이 CAS 경쟁, _bottom은 owner만 접근
	// 따라서 top과 bottom을 같은 cache line에 두면 
	// false sharing으로 stealer들의 CAS가 owner의 bottom 업데이트와 충돌
	alignas(64) std::atomic<int64_t> _top{ 0 };
	alignas(64) std::atomic<int64_t> _bottom{ 0 };

	// 슬롯 단위 atomic 선언, 읽기/쓰기는 relaxed ordering 사용
	// 동기화 책임은 top / bottom atomic에 집중시키는 설계
	alignas(64) std::atomic<T>		 _buffer[Capacity];
};

template<typename T, int64_t Capacity>
inline bool LFWSDeque<T, Capacity>::TryPush(const T& item)
{
	int64_t top = _top.load(std::memory_order_acquire);
	int64_t bottom = _bottom.load(std::memory_order_relaxed);

	if (bottom - top >= Capacity - 1)
		return false;

	_buffer[bottom & kMask].store(item, std::memory_order_relaxed);
	_bottom.store(bottom + 1, std::memory_order_release);
	return true;
}

template<typename T, int64_t Capacity>
inline bool LFWSDeque<T, Capacity>::TryPush(T&& item)
{
	int64_t top = _top.load(std::memory_order_acquire);
	int64_t bottom = _bottom.load(std::memory_order_relaxed);

	if (bottom - top >= Capacity - 1)
		return false;

	_buffer[bottom & kMask].store(std::move(item), std::memory_order_relaxed);
	_bottom.store(bottom + 1, std::memory_order_release);
	return true;
}

template<typename T, int64_t Capacity>
inline std::optional<T> LFWSDeque<T, Capacity>::TryPop()
{
	int64_t bottom = _bottom.load(std::memory_order_relaxed) - 1;

	// relaxed로 낮추면 steal이 stale한 bottom을 보고 마지막 원소 경쟁에서 
	// ABA 문제 발생 가능
	_bottom.store(bottom, std::memory_order_seq_cst);

	// stealer의 CAS 결과 관찰
	int64_t top = _top.load(std::memory_order_acquire);

	// 원소가 여러 개 남아있음 - stealer와 경쟁 X
	if (bottom > top)
	{
		return _buffer[bottom & kMask].load(std::memory_order_relaxed);
	}

	// 이미 비어 있음 - bottom 복원
	else if (bottom < top)
	{
		// 이미 deque가 비어 있음이 확정된 상태
		// 다른 스레드가 이 값을 읽든 말든 경쟁 X
		_bottom.store(top, std::memory_order_relaxed);
		return std::nullopt;
	}

	// bottom == top: 마지막 원소, stealer와 CAS 경쟁
	else
	{
		T item = _buffer[bottom & kMask].load(std::memory_order_relaxed);

		if (_top.compare_exchange_strong(
			top, top + 1,
			std::memory_order_seq_cst,
			std::memory_order_relaxed))
		{
			_bottom.store(top + 1, std::memory_order_relaxed);
			return item;
		}

		else
		{
			// stealer가 먼저 가져감
			_bottom.store(top + 1, std::memory_order_relaxed);
			return std::nullopt;
		}
	}
}

template<typename T, int64_t Capacity>
inline std::optional<T> LFWSDeque<T, Capacity>::TrySteal()
{
	int64_t top = _top.load(std::memory_order_acquire);
	std::atomic_thread_fence(std::memory_order_seq_cst);
	int64_t bottom = _bottom.load(std::memory_order_acquire);

	if (top >= bottom)
	{
		return std::nullopt;
	}

	else
	{
		T item = _buffer[top & kMask].load(std::memory_order_relaxed);

		if (!_top.compare_exchange_strong(
			top, top + 1,
			std::memory_order_seq_cst,
			std::memory_order_relaxed))
		{
			return std::nullopt;
		}

		return item;
	}
}

template<typename T, int64_t Capacity>
inline void LFWSDeque<T, Capacity>::Reset() noexcept
{
	_top.store(0, std::memory_order_relaxed);
	_bottom.store(0, std::memory_order_relaxed);
}
