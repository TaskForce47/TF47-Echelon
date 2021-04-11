#include "client.h"
#include "logger.h"

void echelon::Client::backgroundWorkerTask()
{
	while (!stopBackgroundWorkerFlag)
	{
		std::this_thread::sleep_for(std::chrono::microseconds(500 ));

		if (jobQueue.size() > 200)
		{
			for (int i = 0; i < 200; i++)
			{
				sendJob(jobQueue.pop().value());
			}
		}
		
	}
	for (int i = 0; i < jobQueue.size(); i++)
		sendJob(jobQueue.pop().value());
}

void echelon::Client::sendJob(JobItem& item)
{
	auto* config = &echelon::Config::get();

	std::stringstream route;
	route << config->getHostname() << "/api/tracking/" << config->getSessionId();

	json j;

	j["Id"] = item.Data;
	j["DataType"] = item.DataType;
	j["Data"] = item.Data;
	j["TickTime"] = item.TickTime;
	j["GameTime"] = item.GameTime;


	auto response = cpr::Post(cpr::Url{ route.str() }, cpr::Header{
			{ "Content-Type", "application/json" },
			{ "TF47AuthKey", config->getApiKey() }
		}, cpr::Body(j.dump()));

	if (response.status_code != 200)
	{
		std::stringstream ss;
		ss << "Failed to upload data to api! [Statuscode:  " << response.status_code << " ] [Message: " << response.text << " ] [Packet: " << j.dump() << "]";
		Logger::WriteLog("Failed to upload data to api", Error);
	}
}

echelon::Client::Client()
{
}

echelon::Client::~Client()
{
	stopBackgroundWorker();
}

void echelon::Client::createSession(std::string worldName)
{
	auto* config = &Config::get();

	if (config->isSessionRunning()) return;
	
	json j;
	j["missionId"] = config->getMissionId();
	j["missionType"] = config->getMissionType();
	j["worldName"] = worldName;

	std::stringstream route;
	route << Config::get().getHostname() << "/api/session";

	
	auto response = cpr::Post(cpr::Url{ route.str() }, cpr::Header{
			{ "Content-Type", "application/json" },
			{ "TF47AuthKey", config->getApiKey() }
		},  
		cpr::Body(j.dump()));

	if (response.status_code == 200 || response.status_code == 201)
	{
		json resJ = json::parse(response.text);
		config->setSessionId(resJ["sessionId"].get<int>());
		config->setSessionRunning(true);
	} else {
		std::stringstream ss;
		ss << "Failed to start session, Statuscode: " << response.status_code << "Body: " << response.text;
		throw ss.str();
	}
}

void echelon::Client::endSession()
{	
	auto* config = &Config::get();

	if (!config->isSessionRunning()) return;

	std::stringstream route;
	route << config->getHostname() << "/api/Session/" << config->getSessionId() << "/endSession";


	auto response = cpr::Put(cpr::Url{ route.str() }, cpr::Header{
			{ "Content-Type", "application/json" },
			{ "TF47AuthKey", config->getApiKey() }
		});
	
	if (response.status_code == 200) {
		config->setSessionId(0);
		config->setSessionRunning(false);
	} else {
		std::stringstream ss;
		ss << "Failed to stop session, Statuscode: " << response.status_code << "Body: " << response.text;
		throw ss.str();
	}
}

void echelon::Client::updateOrCreatePlayer(std::string playerUid, std::string playerName)
{
	auto* config = &Config::get();

	if (!config->isSessionRunning()) return;

	json j;
	j["playerName"] = playerName;

	std::stringstream route;
	route << config->getHostname() << "/api/Player/" << playerUid << "/refresh";

	auto response = cpr::Put(cpr::Url{ route.str() }, cpr::Header{
			{ "Content-Type", "application/json" },
			{ "TF47ApiKey", config->getApiKey() }
		}, cpr::Body(j.dump()));
	
		if (response.status_code != 200)
		{
			std::stringstream ss;
			ss << "Failed to create session, statuscode: " << response.status_code << "Body: " << response.text;
			throw ss.str();
		}
}

void echelon::Client::startBackgroundWorker()
{
	if (backgroundWorker == nullptr) {
		backgroundWorker = new std::thread(&backgroundWorkerTask);
	}
}

void echelon::Client::stopBackgroundWorker()
{
	if (backgroundWorker != nullptr) {
		stopBackgroundWorkerFlag = true;
		backgroundWorker->join();
		stopBackgroundWorkerFlag = false;
		backgroundWorker = nullptr;
	}
}

void echelon::Client::addToQueue(JobItem& item)
{
	jobQueue.push(item);
}
