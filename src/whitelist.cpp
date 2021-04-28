#include "whitelist.h"

#include "../intercept/src/host/invoker/invoker.hpp"

void echelon::Whitelist::startWhitelist()
{
	if (initialized) return;

	initialized = true;

	//client::addMissionEventHandler<client::eventhandlers_mission::PlayerViewChanged>([] )
	
}

void Whitelist::handleSlotPermissionCheck(object& unit, std::string playerUid, std::list<int> requiredPermissions,
	std::list<int> minimalPermissions)
{
	

	Client client;
	std::set<int> whitelists;
	try
	{
		whitelists = std::set(client.getWhitelist(playerUid));
	}
	catch(std::exception& ex)
	{
		std::stringstream ss;
		ss << "Failed to get permissions for player uid: " << playerUid << "! Reason:" << ex.what();
		Logger::WriteLog(ss.str(), Warning);
		return;
	}


	for (auto requiredPermission : requiredPermissions)
	{
		if (whitelists.find(requiredPermission) == whitelists.end())
		{
			kickPlayerToLobby(unit);
			return;
		}
	}

	//as soon we have one permission let the player stay on this slot
	for (auto minimalPermission : minimalPermissions)
	{
		if (whitelists.find(minimalPermission) != whitelists.end())
		{
			return;
		}
	}
	//player has no permission as in minimal permissions
	kickPlayerToLobby(unit);
}

void Whitelist::kickPlayerToLobby(object player) const
{
	client::invoker_lock lock;
	const auto params = auto_array<game_value>(game_value("end1"), game_value("false"));
	sqf::remote_exec(params, "BIS_fnc_endMission", player, false);
}

void Whitelist::kickPlayerOfVehicle(object player) const
{
	client::invoker_lock lock;
	std::vector paramVar = { game_value(player), game_value("Eject"), game_value(sqf::vehicle(player)) };
	const auto params = auto_array<game_value>(paramVar.begin(), paramVar.end());
	sqf::remote_exec(params, "action", player, false);
}

game_value handle_cmd_register_slot(game_state& gs, game_value_parameter left_args, game_value_parameter right_args) {
	if (right_args.size() != 2)
	{
		Logger::WriteLog("Failed to register slot to whitelist. Function tf47registerSlot is called with invalid arguments", Error);
		gs.set_script_error(game_state::game_evaluator::evaluator_error_type::foreign, r_string("Right side array must contain two sub arrays"));
		return false;
	}
	if (left_args[0].type_enum() != game_data_type::ARRAY ||right_args[1].type_enum() != game_data_type::ARRAY)
	{
		Logger::WriteLog("Failed to register slot to whitelist. Function tf47registerSlot is called with invalid arguments", Error);
		gs.set_script_error(game_state::game_evaluator::evaluator_error_type::foreign, r_string("Right side array must contain two sub arrays"));
		return false;
	}

	std::list<int> requiredPermissions;
	std::list<int> minimalPermissions;

	for (game_value requiredPermission : right_args[0])
	{
		requiredPermissions.push_back(requiredPermission);
	}

	for (game_value minimalPermission : right_args[1])
	{
		minimalPermissions.push_back(minimalPermission);
	}

	auto tuple = std::tuple(requiredPermissions, minimalPermissions);
	Whitelist::get().slotWhitelist.insert_or_assign(sqf::str(left_args), tuple);

	std::stringstream ss;
	ss << "Slot: " << sqf::str(left_args) << " has been registered!";
	Logger::WriteLog(ss.str(), Information);
	
	return true;
}


void Whitelist::initCommands()
{
	static inline registered_sqf_function cmd_register_slot;
	cmd_register_slot = intercept::client::host::register_sqf_command("tf47registerSlot", "registers a slot to be checked against the whitelist", handle_cmd_register_slot, game_data_type::BOOL, game_data_type::STRING, game_data_type::ARRAY);
}
