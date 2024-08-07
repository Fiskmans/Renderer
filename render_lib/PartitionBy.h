#pragma once

#include <concepts>
#include <vector>

template<class T, class Functor>
std::vector<std::vector<T>> PartitionBy(const std::vector<T>& aValues, Functor&& aFunctor)
{
	std::vector<std::vector<T>> partitions;

	if (aValues.size() == 0)
		return partitions;

	partitions.reserve(aValues.size() / 8); // best guess for number of partitions

	partitions.push_back({});

	partitions.back().push_back(aValues[0]);

	for (size_t i = 1; i < aValues.size(); i++)
	{
		const T& val = aValues[i];
		if (aFunctor(partitions.back().back(),val))
			partitions.push_back({});
		

		partitions.back().push_back(val);
	}

	return partitions;
}