#pragma once

#include <concepts>

struct Component {};
struct TagComponent : Component {};

template<typename T>
concept CompT = std::derived_from<T, Component>;