#pragma once
#include "intercept.hpp"
#include "config.h"
#include "client.h"
#include "logger.h"

using namespace intercept;
using namespace echelon;

namespace echelon
{
	class Whitelist : public singleton<Whitelist>
	{
		bool initialized = false;
		

	public:

		std::map<std::string, std::tuple<std::list<int>, std::list<int>>> slotWhitelist;
		
		Whitelist()
		{
			
		}

		void startWhitelist();
		
		void handleSlotPermissionCheck(object& unit, std::string playerUid, std::list<int> requiredPermissions,
		                           std::list<int> minimalPermissions);

		
		void kickPlayerToLobby(object player) const;

		void handleUnitEventhandler();
		
		static inline registered_sqf_function cmd_register_slot;
		static void initCommands();
	};
}
