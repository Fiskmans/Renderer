#pragma once

#include "RaytracerConcept.h"

#include "tools/MathVector.h"

#include <tuple>
#include <vector>

/// Mmmmm, yummy yummy c++ templates, look inside at your own risk, the monster stares back
template<class... ChannelTypes> 
class MultiChannelTexture
{
public:
	using PackedValues = std::tuple<ChannelTypes...>;

	template<size_t Channel>
	using TypeAt = std::tuple_element_t<Channel, PackedValues>;

	MultiChannelTexture(fisk::tools::V2ui aSize, const PackedValues& aInitialValues);

	void SetTexel(fisk::tools::V2ui aUV, const PackedValues& aValues);
	PackedValues GetTexel(fisk::tools::V2ui aUV) const;

	size_t GetVersion() const;
	fisk::tools::V2ui GetSize();

	template<size_t Channel>
	const auto& Channel() const;

private:
	template<size_t... IndexSequence>
	void InitializeData(const PackedValues& aInitialValues, std::index_sequence<IndexSequence...> /*Here for TAD*/);

	template<size_t... IndexSequence>
	void SetTexel_Internal(size_t aIndex, const PackedValues& aValues, std::index_sequence<IndexSequence...> /*Here for TAD*/);

	template<size_t... IndexSequence>
	PackedValues GetTexel_Internal(size_t aIndex, std::index_sequence<IndexSequence...> /*Here for TAD*/) const;


	size_t IndexFromUV(fisk::tools::V2ui aUV) const;

	std::tuple<std::vector<ChannelTypes>...> myChannels;
	size_t myVersion;
	fisk::tools::V2ui mySize;
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Implementation

template<class ...ChannelTypes>
inline MultiChannelTexture<ChannelTypes...>::MultiChannelTexture(fisk::tools::V2ui aSize, const PackedValues& aInitialValues)
	: myVersion(0)
	, mySize(aSize)
{
	InitializeData(aInitialValues, std::make_index_sequence<sizeof...(ChannelTypes)>{});
}

template<class ...ChannelTypes>
inline void MultiChannelTexture<ChannelTypes...>::SetTexel(fisk::tools::V2ui aUV, const PackedValues& aValues)
{
	size_t index = IndexFromUV(aUV);
	SetTexel_Internal(index, aValues, std::make_index_sequence<sizeof...(ChannelTypes)>{});
}

template<class ...ChannelTypes>
inline MultiChannelTexture<ChannelTypes...>::PackedValues MultiChannelTexture<ChannelTypes...>::GetTexel(fisk::tools::V2ui aUV) const
{
	return GetTexel_Internal(IndexFromUV(aUV), std::make_index_sequence<sizeof...(ChannelTypes)>{});
}

template<class ...ChannelTypes>
inline size_t MultiChannelTexture<ChannelTypes...>::GetVersion() const
{
	return myVersion;
}

template<class ...ChannelTypes>
inline fisk::tools::V2ui MultiChannelTexture<ChannelTypes...>::GetSize()
{
	return mySize;
}

template<class ...ChannelTypes>
inline size_t MultiChannelTexture<ChannelTypes...>::IndexFromUV(fisk::tools::V2ui aUV) const
{
	return aUV[0] + aUV[1] * mySize[0];
}

template<class ...ChannelTypes>
template<size_t Channel>
inline const auto& MultiChannelTexture<ChannelTypes...>::Channel() const
{
	return std::get<Channel>(myChannels);
}

template<class ...ChannelTypes>
template<size_t ...IndexSequence>
inline void MultiChannelTexture<ChannelTypes...>::InitializeData(const PackedValues& aInitialValues, std::index_sequence<IndexSequence...>)
{
	(std::get<IndexSequence>(myChannels)
		.resize(
			mySize[0] * mySize[1],
			std::get<IndexSequence>(aInitialValues))
		, ...);
}

template<class ...ChannelTypes>
template<size_t ...IndexSequence>
inline void MultiChannelTexture<ChannelTypes...>::SetTexel_Internal(size_t aIndex, const PackedValues& aValues, std::index_sequence<IndexSequence...>)
{
	((std::get<IndexSequence>(myChannels).at(aIndex) = std::get<IndexSequence>(aValues))
		, ...);

	myVersion++;
}

template<class ...ChannelTypes>
template<size_t ...IndexSequence>
inline MultiChannelTexture<ChannelTypes...>::PackedValues MultiChannelTexture<ChannelTypes...>::GetTexel_Internal(size_t aIndex, std::index_sequence<IndexSequence...>) const
{
	return PackedValues(std::get<IndexSequence>(myChannels)[aIndex]...);
}
