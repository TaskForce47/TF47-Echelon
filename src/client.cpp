#include "client.h"
#include "client.hpp"



#include "chat.hpp"
#include "containers.hpp"
#include "debug.hpp"

echelon::Client::Client()
{
	auto newConnection = signalr::hub_connection_builder::create(Config::get().getHostname()).build();

	
	//auto clientConfig = signalr::signalr_client_config();
	//newConnection.set_client_config(clientConfig);
	//clientConfig.set_http_headers({ { "TF47ApiKey", "4wFlzJ4rKkHZRL7EwpsIHy3DQWLceg5nFYuvlc/vWhEDjUKg8PrIoGpaJBbt52W0rhA39QDyPHDTtLPF2zf8Xw==" } });
	
	connection_ = std::make_shared<signalr::hub_connection>(std::move(newConnection));

	connection_->on("Error", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;

			ss << "[TF47-Echelon] " << "Request failed: " << m.as_string();

			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection_->on("sessionCreated", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;
		
			ss << "[TF47-Echelon] " << "Got new session: " << m.as_double();

			Config::get().setSessionId(static_cast<int>(m.as_double()));
			Config::get().setSessionRunning(true);
		
			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection_->on("sessionStopped", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;
		
			ss << "[TF47-Echelon] " << "stopped session: ";

			Config::get().setSessionRunning(false);
			Config::get().setSessionId(0);
		
			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection_->on("playerUpdated", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;

			ss << "[TF47-Echelon] " << "player updated: " << m.as_string();
			
			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(intercept::types::r_string(ss.str()));
		});
}

void echelon::Client::connect()
{
	if (connection_->get_connection_state() == signalr::connection_state::disconnected) {
		
		std::promise<void> start_task;
		connection_->start([&start_task](std::exception_ptr exception) {
			start_task.set_value();
			});
		start_task.get_future().wait();
	}
}


void echelon::Client::createSession(std::string worldName)
{
	if (Config::get().isSessionRunning())
		throw "session is already running";
	
	
	const std::vector<signalr::value> arr{
		signalr::value(static_cast<double>(Config::get().getMissionId())),
		signalr::value(static_cast<double>(Config::get().getMissionType())),
		signalr::value(worldName)
	};
	const signalr::value arg(arr);
	connection_->invoke("createSession", arg);
}

void echelon::Client::endSession()
{
	if (!Config::get().isSessionRunning())
		throw "session is not running";
	connection_->invoke("stopSession", signalr::value(static_cast<double>(Config::get().getSessionId())));
}

void echelon::Client::updateOrCreatePlayer(std::string playerUid, std::string playerName)
{
	const std::vector<signalr::value> arr{ playerUid, playerName };
	const signalr::value arg(arr);
	connection_->invoke("stopSession", arr);
}