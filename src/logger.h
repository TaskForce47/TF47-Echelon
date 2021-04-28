#pragma once

#include <intercept.hpp>
#include "../intercept/src/host/common/singleton.hpp"

namespace echelon
{
	enum LogType
	{
		Information,
		Warning,
		Error
	};
	
	class Logger
	{
	private:
		static r_string toInfoLogString(std::string logType, std::string message) {
			std::stringstream ss;
			ss << "[TF47] (echelon) " << logType << ": " << message;
			return r_string(ss.str());
		}
	
	public:
		static void WriteLog(std::string message, LogType logType = LogType::Information)
		{
			intercept::client::invoker_lock thread_lock;
			r_string output;
			if (logType == LogType::Information)
			{
				output = toInfoLogString("Info", message);
			}
			if (logType == LogType::Error)
			{
				output = toInfoLogString("Error", message);
			}
			if (logType == LogType::Warning)
			{
				output = toInfoLogString("Warning", message);
			}
			intercept::sqf::diag_log(output);
		}
	};
}
