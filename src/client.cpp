#include "client.h"

echelon::Client::Client()
{
	restClient = RestClient::CreateUseOwnThread();
	restClient->CloseWhenReady(false);
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
	
	restClient->ProcessWithPromise([&](Context& ctx)
		{
			auto reply = ctx.Post(route.str(), j.dump());
			if (reply->GetResponseCode() == 200 || reply->GetResponseCode() == 201)
			{
				json resJ = reply->GetBodyAsString();
				config->setSessionId(resJ["sessionId"].get<int>());
				config->setSessionRunning(true);
			} else {
				std::stringstream ss;
				ss << "Failed to start session, Statuscode: " << reply->GetResponseCode() << "Body: " << reply->GetBodyAsString();
				throw ss.str();
			}
		}).get();
}

void echelon::Client::endSession()
{	
	auto* config = &Config::get();

	if (!config->isSessionRunning()) return;

	std::stringstream route;
	route << config->getHostname() << "/api/Session/" << config->getSessionId() << "/endSession";

	restClient->ProcessWithPromise([&](Context& ctx)
		{
			auto reply = ctx.Put(route.str(), "");
			if (reply->GetResponseCode() == 200)
			{
				config->setSessionId(0);
				config->setSessionRunning(0);
			} else {
				std::stringstream ss;
				ss << "Failed to stop session, Statuscode: " << reply->GetResponseCode() << "Body: " << reply->GetBodyAsString();
				throw ss.str();
			}
		}).get();

}

void echelon::Client::updateOrCreatePlayer(std::string playerUid, std::string playerName)
{
	auto* config = &Config::get();

	if (!config->isSessionRunning()) return;

	json j;
	j["playerName"] = playerName;

	std::stringstream route;
	route << config->getHostname() << "/api/Player/" << playerUid << "/refresh";

	restClient->ProcessWithPromise([&](Context& ctx)
		{
			auto reply = ctx.Put(route.str(), j.dump());
			if (reply->GetResponseCode() != 200)
			{
				std::stringstream ss;
				ss << "Failed to create session, statuscode: " << reply->GetResponseCode() << "Body: " << reply->GetBodyAsString();
				throw ss.str();
			}
		}).get();
}
