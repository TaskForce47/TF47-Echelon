#pragma once

#include <future>
#include <queue.h>
#include <config.h>
#include <nlohmann/json.hpp>
#include <cpr/cpr.h>

using json = nlohmann::json;

namespace echelon {

	class Client
	{
	private:
		echelon::JobQueue jobQueue;
		void backgroundWorkerTask();
		void sendJob(JobItem& item);
		void sendJobs(std::list<JobItem> jobItems);
		std::thread* backgroundWorker;
		bool stopBackgroundWorkerFlag = false;
	public:
		Client();
		~Client();
		void createSession(std::string worldName);
		void endSession();
		void updateOrCreatePlayer(std::string playerUid, std::string playerName);
		void startBackgroundWorker();
		void stopBackgroundWorker();
		void addToQueue(JobItem& item);
	};

}