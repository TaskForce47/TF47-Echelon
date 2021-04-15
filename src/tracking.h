#pragma once

#include "config.h"
#include "future"
#include "client.h"
#include "logger.h"
#include "helper.h"
#include <semaphore>
#include <iostream>
#include <thread>
#include <chrono>


namespace echelon
{
	struct TrackedUnit
	{
		long id;
		object unit;
		bool isPlayer;
		std::string name;
		std::string side;
		std::string steamUid;
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
		long id;
		object vehicle;
		std::string name;
		std::string vehicleType;
		intercept::types::side sideOwner;
		vector3 lastPos;
		intercept::client::EHIdentifierHandle fireEventhandler;
		intercept::client::EHIdentifierHandle damagedEventhandler;
		intercept::client::EHIdentifierHandle tackVehicleDeleted;
	};

	struct TrackedProjectile
	{
		long id;
		object projectile;
		object shooter;
		vector3 startPos;
		vector3 velocity;
		std::string weapon;
		std::string ammo;
		bool shooterIsVehicle;
		object vehicle;
		TrackedVehicle trackedVehicle;
	};

	struct GameTime
	{
		float TickTime;
		std::string Time;
	};
	
	class Tracking : public intercept::singleton<Tracking> 
	{
	private:
		echelon::Client client;
		std::list<TrackedVehicle> vehicleList;
		std::list<TrackedUnit> unitList;
		std::list<TrackedProjectile> projectileList;

		std::thread posUpdateThread;
		std::thread projectileUpdateThread;

		mutable std::recursive_mutex projectileListMutex;
		mutable std::recursive_mutex unitListMutex;
		mutable std::recursive_mutex vehicleListMutex;

		std::atomic_int32_t idCounter = 0;
		std::atomic_int32_t packageCounter = 0;
		
		bool stopWorkers = false;
		intercept::client::EHIdentifierHandle killedMissionEventhandler;

		JobItem buildJobItem(std::string dataType, std::string& data, GameTime& gameTime);

		TrackedVehicle* findVehicle(object& vehicle);
		TrackedProjectile* findProjectile(object& projectile);
		TrackedUnit* findUnit(object& unit);

		void removeDeletedProjectiles();
		void removeUnit(object& unit);
		void removeVehicle(object& vehicle);
		
		static GameTime getGameTime();
	
	public:
		Tracking();
		void startTracking();
		void stopTracking();
		
		void trackNewObject(object newEntity);
		
		void trackFired(object& shooter, object& projectile, std::string& weapon, std::string& ammo, object& gunner);
		void trackUnitHit(object& unit, object& projectile, float& damage);
		void trackUnitDeleted(object& unit);
		void trackVehicleDeleted(object& vehicle);
		void trackGetIn(object& unit, object& vehicle, intercept::client::get_in_position& role);
		void trackGetOut(object& unit, object& vehicle, intercept::client::get_in_position& role);

		void doPositionUpdateUnits();
		void doPositionUpdateVehicles();
		void doProjectileUpdate();

		

		static inline registered_sqf_function cmd_init_callback;
		static void initCommands();
	};
}