#pragma once

#include <vector>
#include <intercept.hpp>
#include "tracking.h"

namespace echelon
{
	static std::vector<float> vectorToArray(vector3& pos)
	{
		std::vector<float> result;
		result.push_back(pos.x);
		result.push_back(pos.y);
		result.push_back(pos.z);
		return result;
	};

	static std::vector<float> vectorToArray(vector2& pos)
	{
		std::vector<float> result;
		result.push_back(pos.x);
		result.push_back(pos.y);
		return result;
	};
}
