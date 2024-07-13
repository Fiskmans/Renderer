#pragma once


#include <concepts>

template<class OriginalValue, class Mutator, class OutValue>
concept CanMutateInto = requires(OriginalValue val, Mutator mut, OutValue out)
{
	{ mut(val) } ->std::convertible_to<OutValue>;
};