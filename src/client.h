#pragma once

#include "config.h"
#include <iostream>
#include <future>

#include "connection.h"


// for convenience
using json = nlohmann::json;
using namespace intercept::types;


namespace echelon {

	

	class Client : public intercept::singleton<Client>
	{
	private:
		
	public:
		static game_value Client::cmd_updatePlayer(game_state& gs, game_value_parameter right_arg);
		static game_value Client::cmd_stopSession(game_state& gs);
		static game_value Client::cmd_startSession(game_state& gs);
		
		static inline registered_sqf_function handle_cmd_createSession;
		static inline registered_sqf_function handle_cmd_stopSession;
		static inline registered_sqf_function handle_cmd_update_player;

		static inline Connection connection = Connection();
		
		void initCommands();
	};
	
}