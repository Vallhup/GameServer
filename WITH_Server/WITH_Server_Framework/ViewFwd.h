#pragma once

template<typename... Ex>
struct Exclude {};

template<bool IsConst, typename GetTuple, typename ExTuple>
class BasicView;