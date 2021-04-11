#include <vector>
#include <intercept.hpp>

namespace echelon
{
	static std::vector<float> vectorToArray(vector3& pos)
	{
		std::vector<float> result;
		result.push_back(pos.x);
		result.push_back(pos.y);
		result.push_back(pos.z);
	};
}
