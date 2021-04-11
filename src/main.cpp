#include <intercept.hpp>
#include <string>
#include <sstream>
#include <client.h>
#include <config.h>
#include <thread>
#include <iostream>
#include <chrono>
#include "../addons/main/script_version.hpp"

using namespace intercept;

using SQFPar = game_value_parameter;

echelon::Client* echelonClient;
std::thread test_thread;



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



void do_test()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        client::invoker_lock track_lock;
        auto pos = intercept::sqf::get_pos(intercept::sqf::player());
        auto velocity = intercept::sqf::velocity(intercept::sqf::player());
        std::stringstream ss;

        ss << "Pos: [" << pos.x << ", " << pos.y << ", " << pos.z << "]";
        intercept::sqf::system_chat(toLogString(ss.str()));

        ss.clear();

        ss << "Velocity: [" << velocity.x << ", " << velocity.y << ", " << velocity.z << "]";
        intercept::sqf::system_chat(toLogString(ss.str()));
    }
}

game_value startTest(game_state& gs)
{
    test_thread = std::thread(&do_test);
    return "";
}

void intercept::pre_init() {
    std::stringstream strVersion;
    strVersion << "Running (" << MAJOR << "." << MINOR << "." << PATCHLVL << "." << BUILD << ")";
    intercept::sqf::system_chat(strVersion.str());
    intercept::sqf::diag_log(strVersion.str());

}

void intercept::pre_start() {
    intercept::sqf::system_chat(toLogString("starting to initialise..."));
    echelon::Config::get().reloadConfig();
    echelonClient = new echelon::Client();
    echelon::Config::initCommands();
    //static auto cmd_connect = intercept::client::host::register_sqf_command("tf47connect", "connects to signalR hub", connect, game_data_type::STRING);
    static auto cmd_startTest = intercept::client::host::register_sqf_command("tf47starttest", "", startTest, game_data_type::STRING);
    static auto cmd_createSession = intercept::client::host::register_sqf_command("tf47createSession", "creates a new session in the database", createSession, game_data_type::STRING);
    static auto cmd_stopSession = intercept::client::host::register_sqf_command("tf47stopsession", "Stops a session", endSession, game_data_type::STRING);
    static auto cmd_updateClient = intercept::client::host::register_sqf_command("tf47updateClient", "Updating a playername and connection in the database", updateClient, game_data_type::STRING, game_data_type::ARRAY);
    intercept::sqf::system_chat(toLogString("loading completed"));
}

void intercept::post_init() {

}

