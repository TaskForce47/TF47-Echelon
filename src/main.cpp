#include <intercept.hpp>
#include <client.h>
#include <config.h>
#include <string>
#include <sstream>


#include "../addons/main/script_version.hpp"

using namespace intercept;
using namespace echelon;
using SQFPar = game_value_parameter;

echelon::Client echelonClient;

r_string toLogString(std::string info) {
    return r_string("[TF47] (echelon) INFO: ") + info;
}

int intercept::api_version() {
    return INTERCEPT_SDK_API_VERSION;
}

void intercept::register_interfaces() {}

void intercept::pre_init() {
    std::stringstream strVersion;
    strVersion << "Running (" << MAJOR << "." << MINOR << "." << PATCHLVL << "." << BUILD << ")";
    intercept::sqf::system_chat(strVersion.str());
    intercept::sqf::diag_log(strVersion.str());
}

void intercept::pre_start() {
	
    Config::get().reloadConfig();
    Config::get().initCommands();
    echelonClient.get().initCommands();
    
    intercept::sqf::system_chat(toLogString("loading completed"));
}

void intercept::post_init() {

}

