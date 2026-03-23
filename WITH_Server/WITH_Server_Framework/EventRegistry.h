#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>

#include "EventQueue.h"
#include "WorldDesc.h"

class EventRegistry {
public:
	template<typename T>
	EventQueue<T, SingleThreadBuffer>& Queue()
	{
		using HolderT = QueueHolder<T, SingleThreadBuffer>;
		const std::type_index key{ typeid(HolderT) };

		auto it = _queues.find(key);
		if (it == _queues.end())
		{
			auto holder = std::make_unique<HolderT>();
			auto* ptr = holder.get();

			_queues.emplace(key, std::move(holder));
			return ptr->queue;
		}

		HolderT* holder = static_cast<HolderT*>(it->second.get());
		return holder->queue;
	}

	void ClearAll()
	{
		for (auto& [_, queue] : _queues)
		{
			queue->Clear();
		}
	}

private:
	struct IQueueHolder {
		virtual ~IQueueHolder() = default;
		virtual void Clear() = 0;
	};

	template<typename T, template<typename> class BufferPolicy>
	struct QueueHolder final : public IQueueHolder {
		EventQueue<T, BufferPolicy> queue;

		virtual ~QueueHolder() = default;
		virtual void Clear() override { queue.Clear(); }
	};

	std::unordered_map<std::type_index, std::unique_ptr<IQueueHolder>> _queues;
};

//static void RegisterWorldEvents(EventRegistry& registry, const WorldDesc& desc)
//{
//	(void)registry.Queue<CombatCollisionEvent>();
//	(void)registry.Queue<ActionRequestEvent>();
//}
