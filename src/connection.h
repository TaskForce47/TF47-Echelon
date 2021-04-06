#pragma once

#include "config.h"
#include "signalrclient/hub_connection.h"
#include "signalrclient/hub_connection_builder.h"
#include "signalrclient/signalr_value.h"
#include <nlohmann/json.hpp>
#include "types.hpp"
#include <filesystem>
#include <string>
#include <fstream>
#include <streambuf>

using json = nlohmann::json;
using namespace signalr;
using namespace echelon;

namespace echelon
{
	
	
	class Connection
	{
	private:
		std::shared_ptr<signalr::hub_connection> connection;
		std::thread* workerThread;
		bool workerRunning;
		bool workerStopRequested;
		void doWork();

	public:
		Connection();

		void connectClient();
		void createSession(std::string worldName);
		void endSession();
		void updateOrCreatePlayer(std::string playerUid, std::string playerName);
	};
}
