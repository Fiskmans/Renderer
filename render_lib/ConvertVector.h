#pragma once

#include <typeinfo>
#include <vector>
#include <coroutine>
#include <chrono>
#include <cassert>

struct promise;

struct ConvertVectorCoroutine : std::coroutine_handle<promise>
{
	using promise_type = ::promise;
};

struct promise
{
	ConvertVectorCoroutine get_return_object()
	{
		return { ConvertVectorCoroutine::from_promise(*this) };
	}

	std::suspend_always initial_suspend() noexcept
	{
		return {};
	}

	std::suspend_always final_suspend() noexcept
	{
		return {};
	}
	std::suspend_always yield_value(float aProgress) noexcept
	{
		myProgress = aProgress;
		return {};
	}

	void return_void()
	{
	}

	void unhandled_exception()
	{
	}

	float myProgress = 0;
};

template<class... T1, class Functor, class T2 = decltype(std::declval<Functor>()(std::declval<T1...>()))>
ConvertVectorCoroutine ConvertVectorsAsync(Functor aFunctor, std::vector<T2>& aOutVector, std::chrono::microseconds aTimePerOp, std::vector<T1>... aVectors)
{
	assert(((aVectors.size() == aOutVector.size()) && ...));

	using clock = std::chrono::steady_clock;

	clock::time_point lastYield = clock::now();

	for (size_t i = 0; i < (aVectors, ...).size(); i++)
	{
		aOutVector[i] = (aFunctor(aVectors[i]...));

		if (i % 1000 == 0 && clock::now() - lastYield > aTimePerOp)
		{
			co_yield static_cast<float>(i) / static_cast<float>((aVectors, ...).size());
			lastYield = clock::now();
		}
	}

	co_return;
}