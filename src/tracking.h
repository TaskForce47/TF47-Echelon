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
		intercept::client::EHIdentifierHandle trackUnitDamaged;
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
		intercept::client::EHIdentifierHandle trackUnitDamaged;
	};

	struct TrackedProjectile
	{
		long id;
		object projectile;
		object shooter;
		vector3 startPos;
		vector3 velocity;
		r_string weapon;
		r_string ammo;
		bool shooterIsVehicle;
		object vehicle;
		TrackedVehicle trackedVehicle;
	};

	struct TrackedMarker
	{
		long id;
		marker marker;
		vector3 pos;
		std::string type;
		std::string color;
		int channelId;
		vector2 size;
		std::string shape;
		std::string text;
		std::chrono::time_point<std::chrono::steady_clock> timeCreated;
	};

	struct GameTime
	{
		float TickTime;
		std::string Time;
	};


	class MarkerList
	{
	private:
		mutable std::mutex lock;
		std::list<TrackedMarker> markerList;
	public:
		void removeMarker(marker& markerToDelete)
		{
			std::lock_guard<std::mutex> lockList(lock);
			auto i = markerList.begin();
			while (i != markerList.end())
			{
				if (i->marker == markerToDelete)
				{
					markerList.erase(i);
					return;
				}
				++i;
			}
		}
		void addMarker(TrackedMarker& trackedMarker)
		{
			std::lock_guard<std::mutex> lockList(lock);
			markerList.push_back(trackedMarker);
		}
		TrackedMarker* findMarker(marker& gameMarker)
		{
			std::scoped_lock<std::mutex> lockList(lock);
			for (auto& projectileElement : markerList)
			{
				if (projectileElement.marker == gameMarker)
					return &projectileElement;
			}
			return nullptr;
		}

		std::list<TrackedMarker> getMarkerList()
		{
			std::lock_guard<std::mutex> lockList(lock);
			return markerList;
		}
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
		MarkerList markerList;
		
		std::thread posUpdateThread;
		std::thread projectileUpdateThread;


		std::atomic_int32_t idCounter = 0;
		std::atomic_int32_t packageCounter = 0;

		std::chrono::time_point<std::chrono::steady_clock> timeLastTrackingUnits;
		std::chrono::time_point<std::chrono::steady_clock> timeLastTrackingVehicles;
		std::chrono::time_point<std::chrono::steady_clock> timeLastTrackingProjectiles;
		
		bool stopWorkers = false;
		intercept::client::EHIdentifierHandle killedMissionEventhandler;

		JobItem buildJobItem(std::string dataType, std::string& data);
		
		static GameTime getGameTime();
	
	public:
		Tracking();
		~Tracking();
		void startTracking();
		void stopTracking();

		void handlePerFrame();
		
		
		void trackNewObject(object newEntity);
		
		void trackFired(object& shooter, object& projectile, r_string& weapon, r_string& ammo, object& gunner);
		void trackUnitHit(object& unit, object& projectile, float& damage, r_string& hitPoint);
		void trackUnitDeleted(object& unit);
		void trackVehicleDeleted(object& vehicle);
		void trackVehicleHit(object& vehicle, object& projectile, float& damage, r_string& hitPart);
		void trackGetIn(object& unit, object& vehicle, intercept::client::get_in_position& role);
		void trackGetOut(object& unit, object& vehicle, intercept::client::get_in_position& role);

		void trackMarkerCreated(marker& marker, int& channelNumber, object& owner);
		void trackMarkerDeleted(marker& marker);
		void trackMarkerUpdated(marker& marker);

		void doPositionUpdateUnits();
		void doPositionUpdateVehicles();
		void doProjectileUpdate();

		

		static inline registered_sqf_function cmd_init_callback;
		static inline registered_sqf_function cmd_unit_hit_callback;
		static inline registered_sqf_function cmd_vehicle_hit_callback;
		static inline registered_sqf_function cmd_marker_created_callback;
		static inline registered_sqf_function cmd_marker_updated_callback;
		static inline registered_sqf_function cmd_marker_deleted_callback;
		static void initCommands();
	};
}