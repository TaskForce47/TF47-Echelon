#include "client.h"

#include <future>

#include "../intercept/src/client/headers/shared/client_types.hpp"
using namespace echelon;
using namespace intercept::types;

game_value Client::cmd_startSession(game_state& gs)
{
	const auto worldName = intercept::sqf::world_name();
	try
	{
		connection.createSession(worldName);
	} catch (std::runtime_error& ex) {
		return r_string("Failed to create session: ") + ex.what();
	}
	std::stringstream ss;
	ss << "Created new session! Id: " << Config::get().getSessionId();
	return r_string(ss.str());
}

game_value Client::cmd_stopSession(game_state& gs)
{
	try
	{
		connection.endSession();
	}
	catch (std::runtime_error& ex) {
		return r_string("Failed to stop session: ") + ex.what();
	}
	return r_string("Stopped session successfully");
}

game_value Client::cmd_updatePlayer(game_state& gs, game_value_parameter right_arg)
{
	if (!intercept::sqf::is_player(right_arg))
		return "Error: No player provided";

	const auto playerUid = intercept::sqf::get_player_uid(right_arg);
	const auto playerName = intercept::sqf::name(static_cast<object>(right_arg));
	
	try
	{
		connection.updateOrCreatePlayer(playerUid, playerName);
	} catch (std::runtime_error&) {
		std::stringstream ss;
		ss << "Failed to update  player! Id: " << playerUid << " Name: " << playerName;
		return r_string(ss.str());
	}
	return r_string("Ok");
}




void Client::initCommands()
{
	handle_cmd_createSession = intercept::client::host::register_sqf_command("tf47createsession", "Creates a new session in the database", cmd_startSession, game_data_type::STRING);
	handle_cmd_stopSession = intercept::client::host::register_sqf_command("tf47stopsession", "Stops a running session", cmd_stopSession, game_data_type::STRING);
	handle_cmd_update_player = intercept::client::host::register_sqf_command("tf47updateplayer", "Updates the number of connections, name and last time seen in the database. Will create a new user if it doesn't exist.", cmd_updatePlayer, game_data_type::STRING, game_data_type::OBJECT);
}
