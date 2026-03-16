#pragma once

#include <memory>
#include <mutex>
#include <type_traits>
#include <utility>
#include <vector>

#include "ComponentStorage.h"

class WorldRuntime;

template<typename T>
concept CommandT = 
std::move_constructible<std::decay_t<T>> &&
	requires(std::decay_t<T> fn, WorldRuntime& rt)
{
	{ fn(rt) } -> std::same_as<void>;
};

class CommandBuffer final {
public:
	CommandBuffer() = default;
	~CommandBuffer() = default;

	CommandBuffer(const CommandBuffer&) = delete;
	CommandBuffer& operator=(const CommandBuffer&) = delete;

	template<CommandT T>
	void Enqueue(T&& fn)
	{
		using Fn = std::decay_t<T>;

		auto cmd = std::make_unique<CommandModel<Fn>>(std::forward<T>(fn));
		{
			std::lock_guard lock{ _mtx };
			_pendingCommands.push_back(std::move(cmd));
		}
	}

	void Commit(WorldRuntime& rt);
	void Clear();
	bool Empty() const;

private: 
	struct ICommand
	{
		virtual ~ICommand() = default;
		virtual void Apply(WorldRuntime& rt) = 0;
	};

	template<typename F>
	struct CommandModel final : ICommand
	{
		template<typename U>
		explicit CommandModel(U&& fn) 
			: fn(std::forward<U>(fn)) { }

		virtual void Apply(WorldRuntime& rt) override
		{
			fn(rt);
		}

		F fn;
	};

	mutable std::mutex _mtx;
	std::vector<std::unique_ptr<ICommand>> _pendingCommands;
};