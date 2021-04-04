#pragma once

#include <cpr/cpr.h>
#include "config.h"
#include "sstream"
#include "intercept.hpp"
#include <nlohmann/json.hpp>

// for convenience
using json = nlohmann::json;

namespace echelon {

	class Client
	{
	private:

	public:
		Client();
		void createSession(std::string worldName);
		void endSession();
		void updateOrCreatePlayer(std::string playerUid, std::string playerName);

		static void initCommands();
		static inline registered_sqf_function handle_cmd_createSession;
		static inline registered_sqf_function handle_cmd_stopSession;
		static inline registered_sqf_function handle_cmd_update_player;
	};

}