#pragma once

#include <vector>
#include <atomic>
#include <memory>

#include "Entry.hpp"
#include "WaitStrategy.hpp"
#include "AtomicCounter.hpp"

namespace disruptor {
	class RingBuffer {
	public:
		RingBuffer(int64_t size) : _size(size), _entries(size)
		{
			// TODO : size 2의 제곱 아니면 crash
			_waitStrategy = std::make_unique<YieldWaitStrategy>();
		}

		RingBuffer(int64_t size, std::unique_ptr<WaitStrategy> wait)
			: _size(size), _entries(size), _waitStrategy(std::move(wait)) {}

		Entry& GetEntry(int64_t seq)
		{
			int64_t idx = seq & (_size - 1);
			return _entries[idx];
		}

		const int64_t& GetSize() const { return _size; }

		AtomicCounter& GetCursor() { return _cursor; }
		void SetCursor(int64_t seq) { _cursor.value = seq; }

		int64_t GetMinGatingSequence() const
		{
			int64_t min = _gatingSequences[0]->value;
			for (int64_t i = 1; i < _gatingSequences.size(); ++i)
			{
				if (min > _gatingSequences[i]->value)
					min = _gatingSequences[i]->value;
			}
			return min;
		}

		void AddGatingSequence(AtomicCounter* seq)
		{
			_gatingSequences.push_back(seq);
		}

	private:
		const int64_t _size;
		std::vector<Entry> _entries;

		AtomicCounter _cursor;
		AtomicCounter _nextSequence;
		std::vector<AtomicCounter*> _gatingSequences;

		std::unique_ptr<WaitStrategy>  _waitStrategy;
	};
}

// LMAX Disruptor 기반 동기화 버퍼
// MPSC Ring Buffer

// 핵심 설계 기준
// 
// 1. 메모리 할당
//  - 링 버퍼의 모든 메모리는 시작 시 미리 할당된다 -> 런타임에 memory allocation 없음
// 
// 2. 기존 큐의 문제점
//  - 연결 리스트 기반 큐는 Cache Locality가 매우 안 좋다
// 
// 3. 인덱스 계산
//  - 대부분의 자료구조는 시퀀스 번호에서 나머지 계산을 해서 인덱스 계산 -> 너무 무거움
//  - 링의 크기를 2의 거듭제곱으로 만들고 (size - 1)의 비트 마스크를 사용해서 최적화
//
// 4. 시퀀싱
//  - 동기화를 위해 전역 시퀀스를 사용해서 링버퍼의 슬롯을 순서대로 접근한다
//  - (멀티 Producer의 경우) atomic increment or CAS로 다음 시퀀스 확보 -> Data Write
//  - Producer는 Data Write가 끝나면 커서를 갱신해 Consumer에게 공개
//  - Consumer는 cursor가 자신이 기다리는 시퀀스까지 올라올 때까지 기다린다
//  - Consumer도 자기만의 시퀀스 커서를 가진다
//
// 4.1. 시퀀스 / 커서에 대한 이해
//  - buffer index = sequence % buffer_size
//  - sequence = epoch counter와 유사
// 
//  - 시퀀스 관리 흐름
//    1) Producer는 Sequence를 atomic하게 증가시키면서 자신의 슬롯을 할당받아 Write 수행
//    2) Write를 완료하면 Publish해서 cursor를 앞으로 전진
// 
//    3) Consumer는 cursor가 모든 Consumer들 중 가장 작은 Sequence보다 커질 때까지 대기
//    4) 커지면 읽기 시퀀스를 할당받아서(자신의 시퀀스 + 1) 읽기 수행
//
//  - 커서는 읽을 수 있는 최대 시퀀스를 의미함
//    -> 앞선 시퀀스가 모두 publish됐는지 확인해야함
//

// 클래스 구성 및 설명
//
// 1. RingBuffer
//  - 고정 크기 객체 배열 관리
//  - 가장 최근에 쓰인 시퀀스를 나타내는 cursor 관리
// 
//  - claim() : 다음 쓸 슬롯의 시퀀스 확보
//  - publish() : 데이터를 쓴 후 cursor 갱신
//  - waitFor() : 특정 시퀀스가 소비 가능할 때까지 대기
// 
// 2. Entry & EntryFactory
//  - Entry : RingBuffer에 들어갈 객체
//  - EntryFactory : 객체 풀
// 
// 3. ProducerBarrier
//  - 쓰기 가능한 시퀀스 확보 / 데이터를 쓴 후 커서 전진
//  - Producer Thread가 사용하는 인터페이스
//  
// 4. ConsumerBarrier
//  - 읽기 가능한 시퀀스 확보 / 확보 못하면 대기
//  - Consumer Thread가 사용하는 인터페이스
// 
// 5. Consumer
//  - 소비자 객체
//  - 각 Consumer는 자신만의 시퀀스 보유
// 
// 6. ClaimStrategy
//  - 쓰기 가능한 시퀀스 확보 방법 정의하는 인터페이스
//  - Multi Producer의 경우 CAS를 사용하여 시퀀스 확보
// 
// 7. WaitStrategy
//  - Consumer의 대기 정책 정의하는 인터페이스
//  - Busy-Wait / Yield / Condition-Variable / Semapore etc...
// 
// 8. BatchHandler
//  - Consumer가 이벤트를 받아 처리할 때 실행되는 Callback 인터페이스
// 
//  - onAvailable(Entry entry) : 새 데이터가 도착할 때 호출
//  - onEndOfBatch() : 일괄 처리 끝날 때 호출
//  - onCompletion() : 종료 전 정리 작업