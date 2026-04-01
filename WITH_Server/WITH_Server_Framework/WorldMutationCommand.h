#pragma once

#include <concepts>
#include <memory>
#include <type_traits>
#include <utility>

class WorldRuntime;

template<typename T>
concept WorldMutationCommandCallable =
	std::move_constructible<std::decay_t<T>> &&
	requires(std::decay_t<T> fn, WorldRuntime& rt)
{
	{ fn(rt) } -> std::same_as<void>;
};

class WorldMutationCommand final {
public:
	WorldMutationCommand() = default;
	~WorldMutationCommand() = default;

	WorldMutationCommand(const WorldMutationCommand&) = delete;
	WorldMutationCommand& operator=(const WorldMutationCommand&) = delete;
	WorldMutationCommand(WorldMutationCommand&&) noexcept = default;
	WorldMutationCommand& operator=(WorldMutationCommand&&) noexcept = default;

	template<WorldMutationCommandCallable T>
	explicit WorldMutationCommand(T&& fn)
		: _impl(std::make_unique<CommandModel<std::decay_t<T>>>(std::forward<T>(fn)))
	{
	}

	void Apply(WorldRuntime& rt)
	{
		if (_impl)
			_impl->Apply(rt);
	}

	bool IsValid() const noexcept
	{
		return static_cast<bool>(_impl);
	}

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
			: _fn(std::forward<U>(fn))
		{
		}

		void Apply(WorldRuntime& rt) override
		{
			_fn(rt);
		}

		F _fn;
	};

	std::unique_ptr<ICommand> _impl;
};
