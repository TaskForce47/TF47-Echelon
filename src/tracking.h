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
		intercept::client::EHIdentifierHandle hitEventhandler;
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

	class ProjectileList
	{
	private:
		mutable std::mutex lock;
		std::list<TrackedProjectile> projectileList;
	public:
		void removeDeletedProjectiles()
		{
			std::lock_guard<std::mutex> lockList(lock);
			auto i = projectileList.begin();
			while (i != projectileList.end())
			{
				if (i->projectile.is_null())
				{
					projectileList.erase(i++);
				}
				else {
					++i;
				}
			}
		}
		void addProjectile(TrackedProjectile& trackedProjectile)
		{
			std::lock_guard<std::mutex> lockList(lock);
			projectileList.push_back(trackedProjectile);
		}
		TrackedProjectile* findProjectile(object& projectile)
		{
			std::scoped_lock<std::mutex> lockList(lock);
			for (auto& projectileElement : projectileList)
			{
				if (projectileElement.projectile == projectile)
					return &projectileElement;
			}
			return nullptr;
		}

		std::list<TrackedProjectile> getProjectileList()
		{
			std::lock_guard<std::mutex> lockList(lock);
			return projectileList;
		}
	};

	class UnitList
	{
	private:
		std::mutex lock;
		std::list<TrackedUnit> unitList;
	public:
		void removeUnit(object& unit)
		{
			std::lock_guard<std::mutex> lockList(lock);
			auto i = unitList.begin();
			while (i != unitList.end())
			{
				if (i->unit == unit)
				{
					unitList.erase(i);
					return;
				}
				++i;
			}
		}
		void addUnit(TrackedUnit& unit)
		{
			std::lock_guard<std::mutex> lockList(lock);
			unitList.push_back(unit);
		}
		TrackedUnit* findUnit(object& unit)
		{
			std::lock_guard<std::mutex> lockList(lock);
			for (auto& unitElement : unitList)
			{
				if (unitElement.unit == unit)
					return &unitElement;
			}
			return nullptr;
		}
		std::list<TrackedUnit> getUnitList()
		{
			std::lock_guard<std::mutex> lockList(lock);
			return unitList;
		}
	};

	class VehicleList
	{
	private:
		std::mutex lock;
		std::list<TrackedVehicle> vehicleList;
	public:
		void removeVehicle(object& vehicle)
		{
			std::lock_guard<std::mutex> lockList(lock);
			auto i = vehicleList.begin();
			while (i != vehicleList.end())
			{
				if (i->vehicle == vehicle)
				{
					vehicleList.erase(i);
					return;
				}
				++i;
			}
		}
		void addVehicle(TrackedVehicle& vehicle)
		{
			std::lock_guard<std::mutex> lockList(lock);
			vehicleList.push_back(vehicle);
		}
		TrackedVehicle* findVehicle(object& vehicle)
		{
			std::lock_guard<std::mutex> lockList(lock);
			for (auto& vehicleElement : vehicleList)
			{
				if (vehicleElement.vehicle == vehicle)
					return &vehicleElement;
			}
			return nullptr;
		}

		std::list<TrackedVehicle> getVehicleList()
		{
			std::lock_guard<std::mutex> lockList(lock);
			return vehicleList;
		}
	};
	
	class Tracking : public intercept::singleton<Tracking> 
	{
	private:
		echelon::Client client;
		VehicleList vehicleList;
		UnitList unitList;
		ProjectileList projectileList;

		std::thread posUpdateThread;
		std::thread projectileUpdateThread;


		std::atomic_int32_t idCounter = 0;
		std::atomic_int32_t packageCounter = 0;
		
		bool stopWorkers = false;
		intercept::client::EHIdentifierHandle killedMissionEventhandler;


		JobItem buildJobItem(std::string dataType, std::string& data, GameTime& gameTime);
		
		static GameTime getGameTime();
	
	public:
		Tracking();
		~Tracking();
		void startTracking();
		void stopTracking();
		
		void trackNewObject(object newEntity);
		
		void trackFired(object& shooter, object& projectile, std::string& weapon, std::string& ammo, object& gunner);
		void trackUnitHit(object& unit, object& projectile, bool& directHit, std::vector<std::string> selection);
		void trackUnitDeleted(object& unit);
		void trackVehicleDeleted(object& vehicle);
		void trackGetIn(object& unit, object& vehicle, intercept::client::get_in_position& role);
		void trackGetOut(object& unit, object& vehicle, intercept::client::get_in_position& role);

		void doPositionUpdateUnits();
		void doPositionUpdateVehicles();
		void doProjectileUpdate();

		

		static inline registered_sqf_function cmd_init_callback;
		static inline registered_sqf_function cmd_unit_hit_callback;
		static void initCommands();
	};
}