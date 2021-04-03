#include "config.h"


void Config::reloadConfig()
{
	std::filesystem::path configFilePath("@InterceptDB/config.json");

	if (!exists(configFilePath))
		throw std::filesystem::filesystem_error("File not found", configFilePath, std::error_code());

	auto stringPath = configFilePath.string();

	std::ifstream i(configFilePath);
	json j;
	i >> j;

	apiKey = j["ApiKey"].get<std::string>();
	hostname = j["Hostname"].get<std::string>();
	missionId = j["MissionId"].get<int>();
	missionType = j["MissionType"].get<int>();
}

game_value cmd_reloadConfig(game_state&)
{
	try
	{
		Config::get().reloadConfig();
	}
	catch (std::runtime_error& ex) {
		return r_string("error ") + ex.what();
	}
	return {};
}

void Config::initCommands()
{
	handle_cmd_reloadConfig = intercept::client::host::register_sqf_command("tf47reloadconfig", "Reloads tf47 echelon api configuration", cmd_reloadConfig, game_data_type::STRING);
}
