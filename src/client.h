#pragma once

#include <future>
#include <config.h>
#include <nlohmann/json.hpp>
#include <restc-cpp/restc-cpp.h>

using json = nlohmann::json;
using namespace restc_cpp;

namespace echelon {

	class Client
	{
	private:
		std::unique_ptr<RestClient> restClient;
	public:
		Client();
		void createSession(std::string worldName);
		void endSession();
		void updateOrCreatePlayer(std::string playerUid, std::string playerName);
	};

}