#pragma once

#include <iostream>
#include <future>
#include <config.h>
#include <signalrclient/hub_connection.h>
#include <signalrclient/hub_connection_builder.h>
#include <signalrclient/signalr_value.h>
#include <nlohmann/json.hpp>

namespace echelon {

	class Client
	{
	private:
		std::shared_ptr<signalr::hub_connection> connection_;
	public:
		Client();
		void connect();
		void createSession(std::string worldName);
		void endSession();
		void updateOrCreatePlayer(std::string playerUid, std::string playerName);
	};

}