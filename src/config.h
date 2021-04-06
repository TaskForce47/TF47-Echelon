#pragma once

#include "intercept.hpp"
#include "../intercept/src/host/common/singleton.hpp"
#include "types.hpp"
#include <filesystem>
#include <string>
#include <fstream>
#include <streambuf>
#include <nlohmann/json.hpp>

#include "../intercept/src/client/headers/shared/types.hpp"

// for convenience
using json = nlohmann::json;

namespace echelon {

	class Config : public intercept::singleton<Config> {
		
	private:
		std::string apiKey;
		std::string hostname;
		int missionId = 0;
		int missionType = 0;
		int sessionId = 0;
		bool sessionRunning = false;
	public:
		void setApiKey(std::string& apiKey)
		{
			this->apiKey = apiKey;
		}
		std::string getApiKey() const
		{
			return this->apiKey;
		}

		void setHostname(std::string& hostname)
		{
			this->hostname = hostname;
		}
		std::string getHostname() const
		{
			return this->hostname;
		}
		void setMissionId(int missionId)
		{
			this->missionId = missionId;
		}
		int getMissionId() const
		{
			return this->getMissionId();
		}
		void setMissionType(int missionType)
		{
			this->missionType = missionType;
		}
		int getMissionType() const
		{
			return this->getMissionType();
		}
		void setSessionId(int sessionId)
		{
			this->sessionId = sessionId;
		}
		int getSessionId() const
		{
			return this->sessionId;
		}
		void setSessionRunning(bool running)
		{
			this->sessionRunning = running;
		}
		bool isSessionRunning() const
		{
			return this->sessionRunning;
		}


		void reloadConfig();

		static void initCommands();
		static inline intercept::types::registered_sqf_function handle_cmd_reloadConfig;
	};

}