// Copyright 2015-2020 Mobilinkd LLC.

#pragma once

namespace mobilinkd
{

template <typename FloatType>
struct FilterBase
{
	virtual FloatType operator()(FloatType input) = 0;
};

} // mobilinkd
