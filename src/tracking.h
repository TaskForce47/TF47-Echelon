#pragma once

#include "config.h"
#include "future"
#include "thread"
#include "client.h"
#include "logger.h"
#include <iostream>
#include "helper.h"

namespace echelon
{
	struct TrackingElement
	{
		int id;
		std::string jsonData;
	};

	struct TrackedUnit
	{
		std::string id;
		object unit;
		bool isPlayer;
		std::string name;
		std::string side;
		bool inVehicle;
		std::string vehicleId;
		vector3 lastPos;
		intercept::client::EHIdentifierHandle fireEventhandler;
		intercept::client::EHIdentifierHandle getInManEventhandler;
		intercept::client::EHIdentifierHandle getOutEventhandler;
		intercept::client::EHIdentifierHandle damagedEventhandler;
		intercept::client::EHIdentifierHandle trackUnitDeleted;
	};

	struct TrackedVehicle
	{
		std::string id;
		object vehicle;
		std::string name;
		std::string vehicleType;
		vector3 lastPos;
		intercept::client::EHIdentifierHandle fireEventhandler;
		intercept::client::EHIdentifierHandle damagedEventhandler;
		intercept::client::EHIdentifierHandle tackVehicleDeleted;
	};
	
	class Tracking : public intercept::singleton<Tracking> 
	{
	private:
		echelon::Client client;
		std::list<object> projectileList;
		std::map<object, TrackedUnit> trackedUnits;
		std::map<object, TrackedVehicle> trackedVehicles;
		std::thread workerThread;

		mutable std::mutex projectileListMutex;
		mutable std::mutex trackedUnitsMutex;
		mutable std::mutex trackedVehiclesMutex;
		
		int classEventhandlerId = -1;
		bool stopWorker = false;
		double precision;
		
		void worker();
		void updateProjectiles();
		
	
	public:
		Tracking();
		void startTracking();
		void stopTracking();
		void trackNewObject(object newEntity);
		void trackUnitFire(object& shooter, std::string& weapon, std::string& ammo, object& projectile);
		void trackUnitHit(object& unit, object& shooter, object& projectile, float& damage, std::string& hitSelection);
		void trackUnitDeleted(object& unit);
		void trackVehicleDeleted(object& vehicle);
		void trackKill();
		void trackUnitDeleted(object& unit);
		void trackProjectile(object& projectile, object& shooter);
		void trackGetIn(object& unit, object& vehicle, std::string& role);
		void trackGetOut(object& unit, object& vehicle, std::string& role);
		void doPositionUpdateUnits();
		void doPositionUpdateVehicles();

		static inline registered_sqf_function cmd_init_callback;
		static void initCommands();
	};
}