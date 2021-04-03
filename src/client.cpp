#include "client.h"

client::client()
{
}

void client::createSession(std::string worldName)
{
	if (Config::get().isSessionRunning())
	{
		std::stringstream error;
		error << "Failed to start new session, already running";
		throw &error;
	}

	json requestBody;
	requestBody["missionId"] = Config::get().getMissionId();
	requestBody["missionType"] = Config::get().getMissionType();
	requestBody["worldName"] = worldName;

	std::stringstream route;
	route << Config::get().getHostname() << "/api/Session/";
	
	auto res = cpr::Post(cpr::Url(route.str()), cpr::Header({
		{"TF47ApiKey", Config::get().getApiKey()},
		{"Content-Type", "application\\json"}
		}), cpr::Body(requestBody.dump()));
	
	if (res.status_code == 200)
	{
		json j = res.text;
		Config::get().setSessionRunning(true);
		Config::get().setSessionId(j["sessionId"].get<int>());
	} else {
		std::stringstream error;
		error << "Failed to start session";
		throw &error;
	}
}

void client::endSession()
{
	if (!Config::get().isSessionRunning())
	{
		std::stringstream error;
		error << "Failed to stop new session, already stopped";
		throw& error;
	}

	std::stringstream route;
	route << Config::get().getHostname() << "/api/Session/" << Config::get().getSessionId() << "/endsession";

	auto res = cpr::Patch(cpr::Url(route.str()), cpr::Header({
		{"TF47ApiKey", Config::get().getApiKey()},
		{"Content-Type", "application\\json"}
		}));
	if (res.status_code == 200)
	{
		Config::get().setSessionRunning(false);
		Config::get().setSessionId(0);
	}
	else {
		std::stringstream error;
		error << "Failed to end session";
		throw& error;
	}
}

void client::updateOrCreatePlayer(std::string playerUid, std::string playerName)
{
	//first try to get the player to check if he already exists
	std::stringstream route;
	route << Config::get().getHostname() << "/api/Player/" << playerUid;

	auto res = cpr::Post(cpr::Url(route.str()), cpr::Header({
		{"TF47ApiKey", Config::get().getApiKey()},
		{"Content-Type", "application\\json"}
		}));

	if (res.status_code == 400)
	{
		json j = {
			{ "PlayerUid", playerUid },
			{ "PlayerName", playerName }
		};

		route.clear();
		route << Config::get().getHostname() << "/api/Player/";

		res = cpr::Post(cpr::Url(route.str()), cpr::Header({
			{"TF47ApiKey", Config::get().getApiKey()},
			{"Content-Type", "application\\json"}
			}), cpr::Body(j.dump()));

		if (res.status_code != 200)
		{
			std::stringstream error;
			error << "Failed to create new player " << playerName << " uid: " << playerUid;
			throw& error;
		}
	}
	else if (res.status_code == 200) {

		json j = {
			{"PlayerName", playerName }
		};
		route.clear();
		route << Config::get().getHostname() << "/api/Player/" << playerUid << "/refresh";

		res = cpr::Patch(cpr::Url(route.str()), cpr::Header({
			{"TF47ApiKey", Config::get().getApiKey()},
			{"Content-Type", "application\\json"}
			}), cpr::Body(j.dump()));

		if (res.status_code != 200)
		{
			std::stringstream error;
			error << "Failed to update player " << playerName << " uid: " << playerUid;
			throw& error;
		}
	}
}

game_value cmd_startSession(game_state& gs)
{
	const auto worldName = intercept::sqf::world_name();
	auto httpClient = client();
	try
	{
		httpClient.createSession(worldName);
	} catch (std::runtime_error& ex) {
		return r_string("Failed to create session: ") + ex.what();
	}
	std::stringstream ss;
	ss << "Created new session! Id: " << Config::get().getSessionId();
	return r_string(ss.str());
}

game_value cmd_stopSession(game_state& gs)
{
	auto httpClient = client();
	try
	{
		httpClient.endSession();
	}
	catch (std::runtime_error& ex) {
		return r_string("Failed to stop session: ") + ex.what();
	}
	return r_string("Stopped session successfully");
}

game_value cmd_updatePlayer(game_state& gs, game_value_parameter right_arg)
{
	if (!intercept::sqf::is_player(right_arg))
		return "Error: No player provided";

	const auto playerUid = intercept::sqf::get_player_uid(right_arg);
	const auto playerName = intercept::sqf::name(static_cast<object>(right_arg));
	
	auto httpClient = client();
	try
	{
		httpClient.updateOrCreatePlayer(playerUid, playerName);
	} catch (std::runtime_error& ex) {
		std::stringstream ss;
		ss << "Failed to update  player! Id: " << playerUid << " Name: " << playerName;
		return r_string(ss.str());
	}
	return r_string("Ok");
}




void client::initCommands()
{
	handle_cmd_createSession = intercept::client::host::register_sqf_command("tf47createsession", "Creates a new session in the database", cmd_startSession, game_data_type::STRING);
	handle_cmd_stopSession = intercept::client::host::register_sqf_command("tf47stopsession", "Stops a running session", cmd_stopSession, game_data_type::STRING);
	handle_cmd_update_player = intercept::client::host::register_sqf_command("tf47updateplayer", "Updates the number of connections, name and last time seen in the database. Will create a new user if it doesn't exist.", cmd_updatePlayer, game_data_type::STRING, game_data_type::OBJECT);
}
