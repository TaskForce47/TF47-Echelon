#include "config.h"

using namespace echelon;
using namespace intercept::types;

void Config::reloadConfig()
{
	std::filesystem::path configFilePath("@tf47_echelon/config.json");

	if(!std::filesystem::exists(configFilePath))
		throw std::filesystem::filesystem_error("Config file not found", configFilePath, std::error_code());

	auto stringPath = configFilePath.string();

	std::ifstream i(configFilePath);
	json j;
	i >> j;

	apiKey = j["ApiKey"].get<std::string>();
	hostname = j["Hostname"].get<std::string>();
	missionId = j["MissionId"].get<int>();
	missionType = j["MissionType"].get<int>();
}

game_value cmd_reloadConfig(game_state& gs)
{
	try
	{
		Config::get().reloadConfig();
	} catch (std::runtime_error& ex) {
		gs.set_script_error(game_state::game_evaluator::evaluator_error_type::type, r_string("Failed to read config file") + ex.what());
	}
	return "reloaded";
}

void Config::initCommands()
{
	handle_cmd_reloadConfig = intercept::client::host::register_sqf_command(
		"tf47reloadconfig", "Reloads tf47 echelon api configuration", cmd_reloadConfig,
		game_data_type::STRING);
}