
#pragma once

#include <chrono>
#include "MultichannelTexture.h"

using CompactNanoSecond = std::chrono::duration<float, std::nano>;

using TextureType = MultiChannelTexture<fisk::tools::V3f, CompactNanoSecond, unsigned int, unsigned int, unsigned int>;

static constexpr size_t ColorChannel = 0;
static constexpr size_t TimeChannel = 1;
static constexpr size_t ObjectIdChannel = 2;
static constexpr size_t SubObjectIdChannel = 3;
static constexpr size_t RendererChannel = 4;