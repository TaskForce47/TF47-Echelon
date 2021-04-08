#include "client.h"
#include "client.hpp"



#include "chat.hpp"
#include "containers.hpp"
#include "debug.hpp"

echelon::Client::Client()
{
	auto newConnection = signalr::hub_connection_builder::create("https://beta.taskforce47.com/stats").build();
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

			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection_->on("sessionStopped", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;
		
			ss << "[TF47-Echelon] " << "stopped session: ";

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
		auto clientConfig = signalr::signalr_client_config();
		clientConfig.set_http_headers({ { "TF47ApiKey", ""} });

		connection_->set_client_config(clientConfig);



		std::promise<void> start_task;
		connection_->start([&start_task](std::exception_ptr exception) {
			start_task.set_value();
			});
		start_task.get_future().wait();
	}
}


void echelon::Client::createSession(std::string worldName)
{

	const std::vector<signalr::value> arr{  };
	const signalr::value arg(arr);
	connection_->invoke("createSession", arg);
}

void echelon::Client::endSession()
{
	connection_->invoke("stopSession", signalr::value(0.00));
}

void echelon::Client::updateOrCreatePlayer(std::string playerUid, std::string playerName)
{
	const std::vector<signalr::value> arr{ playerUid, playerName };
	const signalr::value arg(arr);
	connection_->invoke("stopSession", arr);
}