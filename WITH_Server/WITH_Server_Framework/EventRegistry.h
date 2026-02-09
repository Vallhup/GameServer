#pragma once

#include <unordered_map>
#include <typeindex>
#include <memory>

#include "EventQueue.h"

class EventRegistry {
public:
	template<typename T>
	EventQueue<T, SingleThreadBuffer>& Queue()
	{
		using HolderT = QueueHolder<T, SingleThreadBuffer>;
		const std::type_index key = stD::type_index(typeid(T));

		auto it = _queues.find(key);
		if (it == _queues.end())
		{
			auto holder = std::make_unique<HolderT>();
			auto* ptr = holder.get();

			_queues.emplace(key, std::move(holder));
			return ptr->queue;
		}

		return static_cast<HolderT*>(it->second.get())->queue;
	}

	void SwapAllBuffers()
	{
		for (auto& [_, queue] : _queues)
		{
			queue->SwapBuffers();
		}
	}

	void ClearAll()
	{
		for (auto& [_, queue] : _queues)
		{
			queue->ClearAll();
		}
	}

private:
	struct IQueueHolder {
		virtual ~IQueueHolder() = default;

		virtual void SwapBuffers() = 0;
		virtual void ClearAll() = 0;
	};

	template<typename T, template<typename> class BufferPolicy>
	struct QueueHolder final : public IQueueHolder {
		EventQueue<T, BufferPolicy> queue;

		virtual ~QueueHolder() = default;

		virtual void SwapBuffers() override { queue.SwapBuffers(); }
		virtual void ClearAll() override { queue.ClearAll(); }
	};

	std::unordered_map<std::type_index, std::unique_ptr<IQueueHolder>> _queues;
};

