#include <intercept.hpp>
#include <string>
#include <sstream>
#include <client.h>
#include <config.h>
#include <tracking.h>
#include <thread>
#include <iostream>
#include <chrono>

#include "logger.h"
#include "../addons/main/script_version.hpp"

using namespace intercept;

using SQFPar = game_value_parameter;

echelon::Client* echelonClient;



int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::register_interfaces() {}


game_value createSession (game_state& gs)
{
    auto worldName = intercept::sqf::world_name();
    try {
        echelonClient->createSession(worldName);
    } catch (std::exception& ex) {
        gs.set_script_error(game_state::game_evaluator::evaluator_error_type::foreign, r_string("failed to create session") + ex.what());
    }
    return "session created";
}

game_value endSession(game_state& gs)
{
    try {
        echelonClient->endSession();
    } catch(std::exception& ex) {
        gs.set_script_error(game_state::game_evaluator::evaluator_error_type::foreign, r_string("failed to stop session") + ex.what());
    }
    return "session stopped";
}

game_value updateClient(game_state& gs, game_value_parameter right_args)
{
    try {
        echelonClient->updateOrCreatePlayer(right_args[0], right_args[1]);
    } catch (std::exception& ex) {
        gs.set_script_error(game_state::game_evaluator::evaluator_error_type::foreign, r_string("failed to update client") + ex.what());
    }
    return "updated user";
}


void intercept::pre_init() {
    std::stringstream strVersion;
    strVersion << "Running (" << MAJOR << "." << MINOR << "." << PATCHLVL << "." << BUILD << ")";
    intercept::sqf::system_chat(strVersion.str());
    intercept::sqf::diag_log(strVersion.str());

}

void intercept::pre_start() {
	echelon::Logger::WriteLog("starting to initialise...");
    echelon::Config::get().reloadConfig();
    echelonClient = new echelon::Client();
    echelon::Config::initCommands();
    echelon::Tracking::get().initCommands();
	
    //static auto cmd_connect = intercept::client::host::register_sqf_command("tf47connect", "connects to signalR hub", connect, game_data_type::STRING);
    static auto cmd_createSession = intercept::client::host::register_sqf_command("tf47createSession", "creates a new session in the database", createSession, game_data_type::STRING);
    static auto cmd_stopSession = intercept::client::host::register_sqf_command("tf47stopsession", "Stops a session", endSession, game_data_type::STRING);
    static auto cmd_updateClient = intercept::client::host::register_sqf_command("tf47updateClient", "Updating a playername and connection in the database", updateClient, game_data_type::STRING, game_data_type::ARRAY);
    echelon::Logger::WriteLog("loading completed");
}

void intercept::on_frame()
{
    echelon::Tracking::get().handlePerFrame();
}


void intercept::post_init() {
    echelon::Tracking::get().startTracking();
}

void intercept::handle_damage(object& unit_, r_string selection_name_, float damage_, object& source_, r_string projectile_, int hit_part_index_)
{
    std::stringstream ss;
    ss << "Eventhandler damaged fired! " << intercept::sqf::name(unit_) << ": " << selection_name_ << ": " << damage_ << ": " << intercept::sqf::name(source_) << ": " << projectile_;
    echelon::Logger::WriteLog(ss.str());
    std::cout << ss.str();
}


void intercept::mission_ended()
{
    echelon::Tracking::get().stopTracking();
    echelonClient->endSession();
}


