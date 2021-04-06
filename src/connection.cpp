#include "connection.h"

#include <future>


void Connection::doWork()
{
	while (workerStopRequested)
	{
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

Connection::Connection()
{
	workerThread = nullptr;
	workerRunning = false;
	workerStopRequested = false;

	auto newConnection = signalr::hub_connection_builder::create(
		echelon::Config::get().getHostname()).build();

	connection = std::make_shared<signalr::hub_connection>(std::move(newConnection));

	connection->on("Error", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;

			ss << "[TF47-Echelon] " << "Request failed: " << m.as_string();

			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection->on("sessionCreated", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;

			Config::get().setSessionRunning(true);
			Config::get().setSessionId(static_cast<int>(m.as_double()));

			ss << "[TF47-Echelon] " << "Got new session: " << m.as_double();

			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection->on("sessionStopped", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;

			Config::get().setSessionRunning(false);
			Config::get().setSessionId(0);

			ss << "[TF47-Echelon] " << "stopped session: ";

			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
	connection->on("playerUpdated", [](const signalr::value& m)
		{
			intercept::client::invoker_lock thread_lock;
			std::stringstream ss;

			ss << "[TF47-Echelon] " << "player updated: " << m.as_string();

			intercept::sqf::system_chat(r_string(ss.str()));
			intercept::sqf::diag_log(r_string(ss.str()));
		});
}


void Connection::connectClient()
{

	if (connection->get_connection_state() == connection_state::disconnected) {
		auto clientConfig = signalr::signalr_client_config();
		clientConfig.set_http_headers({ { "TF47ApiKey", Config::get().getApiKey() } });

		connection->set_client_config(clientConfig);



		std::promise<void> start_task;
		connection->start([&start_task](std::exception_ptr exception) {
			start_task.set_value();
			});
		start_task.get_future().wait();
	}

	if (!workerRunning) {
		workerThread = new std::thread([this]
			{
				this->workerRunning = true;
				this->doWork();
			});
	}
}

void Connection::createSession(std::string worldName)
{
	if (Config::get().isSessionRunning())
	{
		std::stringstream error;
		error << "Failed to start new session, already running";
		throw& error;
	}

	const std::vector<signalr::value> arr{ signalr::value(static_cast<double>(Config::get().getMissionId())), signalr::value(static_cast<double>(Config::get().getMissionType())), worldName };
	const signalr::value arg(arr);
	connection->invoke("createSession", arg);
}

void Connection::endSession()
{
	if (!Config::get().isSessionRunning())
	{
		std::stringstream error;
		error << "Failed to stop new session, already stopped";
		throw& error;
	}
	connection->invoke("stopSession", signalr::value(static_cast<double>(Config::get().getSessionId())));
}

void Connection::updateOrCreatePlayer(std::string playerUid, std::string playerName)
{
	const std::vector<signalr::value> arr{ playerUid, playerName };
	const signalr::value arg(arr);
	connection->invoke("stopSession", arr);
}