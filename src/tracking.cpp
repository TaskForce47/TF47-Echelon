#include "tracking.h"

void echelon::Tracking::worker()
{
	while(!stopWorker)
	{
		
	}
}

void echelon::Tracking::updateProjectiles()
{
	//intercept::client::addEventHandler<intercept::client::eventhandlers_object::Fired>();
	//intercept::client::addMissionEventHandler<intercept::client::eventhandlers_mission::()
}

echelon::Tracking::Tracking()
{
}



void echelon::Tracking::startTracking()
{
	intercept::client::invoker_lock lock;
	Logger::WriteLog("Started tracking!");
	//try to read settings
	intercept::sqf::config_entry entry = intercept::sqf::config_entry(intercept::sqf::mission_config_file()) >> ("TF47TrackingSystems");
	game_value eventhandlerId;
	__SQF(
		_eventhandlerId = [
			"All",
			"init",
			{ tf47handleObjectCreated _this },
			true,
			true
		] call CBA_fnc_addClassEventHandler;
		_eventhandlerId
	).capture("", eventhandlerId);
}

void echelon::Tracking::stopTracking()
{
	
}

void echelon::Tracking::trackNewObject(object newEntity)
{
	if (intercept::sqf::get_variable(newEntity, "TF47_tracking_disableTracking", false))
		return;
	
	if (intercept::sqf::is_kind_of(newEntity, "CAManBase"))
	{
		auto trackedUnit = TrackedUnit();
		trackedUnit.isPlayer = intercept::sqf::is_player(newEntity);
		trackedUnit.name = intercept::sqf::name(newEntity);
		trackedUnit.unit = newEntity;
		trackedUnit.side = intercept::sqf::side_get(intercept::sqf::get_group(newEntity));
		trackedUnit.fireEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Fired>(
			newEntity, [this](object unit, std::string weapon, std::string muzzle, std::string mode, std::string ammo, std::string magazine, object projectile, object gunner)
			{
				
			});
		trackedUnit.getInManEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::GetInMan>(
			newEntity, [this](object unit, std::string role, object vehicle, std::vector<std::string> turret)
			{
				trackGetIn(unit, vehicle, role);
			});
		trackedUnit.getOutEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::GetOutMan>(
			newEntity, [this](object unit, std::string role, object vehicle, std::vector<std::string> turret)
			{
				trackGetOut(unit, vehicle, role);
			});
		trackedUnit.damagedEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Dammaged>(
			newEntity, [this](object unit, std::string hitSelection, float damage, float hitPartIndex, std::string hitPoint, object shooter, object projectile)
			{
				trackUnitHit(unit, shooter, projectile, damage, hitSelection);
			});
		trackedUnit.trackUnitDeleted = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Deleted>(
			newEntity, [this](object entity)
			{
				trackUnitDeleted(entity);
			});
		
		std::lock_guard<std::mutex> lock(trackedUnitsMutex);
		trackedUnits.insert({ newEntity, trackedUnit });
		Logger::WriteLog("Added new unit to tracking");
	}
	if (intercept::sqf::is_kind_of(newEntity, "AllVehicles"))
	{
		auto trackedVehicle = TrackedVehicle();
		auto vehicleEntry = intercept::sqf::config_entry(intercept::sqf::config_file()) >> "CfgVehicles" >> intercept::sqf::type_of(newEntity);
		trackedVehicle.vehicle = newEntity;
		trackedVehicle.name = get_text(vehicleEntry >> "displayName");
		game_value vehicleType;
		__SQF(
			private _result = (_this call BIS_fnc_objectType) select 1;
			_result
		).capture("", vehicleType);
		trackedVehicle.vehicleType = vehicleType;
		trackedVehicle.fireEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Fired>(
			newEntity, [this](object unit, std::string weapon, std::string muzzle, std::string mode, std::string ammo, std::string magazine, object projectile, object gunner)
			{

			});
		trackedVehicle.damagedEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Dammaged>(newEntity,
			[this](object unit, std::string hitSelection, float damage, float hitPartIndex, std::string hitPoint, object shooter, object projectile)
			{

			});
		trackedVehicle.tackVehicleDeleted = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Deleted>(newEntity, [this](object entity)
			{
			
			});

		std::lock_guard<std::mutex> lock(trackedUnitsMutex);
		trackedVehicles.insert({ newEntity, trackedVehicle });
		Logger::WriteLog("Added new vehicle to tracking");
	}
}

void echelon::Tracking::trackUnitFire(object& shooter, std::string& weapon, std::string& ammo, object& projectile)
{
	std::lock_guard<std::mutex> lock(trackedUnitsMutex);
	auto unitSearch = trackedUnits.find(shooter);
	if (unitSearch == trackedUnits.end())
	{
		Logger::WriteLog("Cannot find the unit that has shot in cache", Warning);
		return;
	}
		

	auto unit = unitSearch->second;

	auto gameLock = intercept::client::invoker_lock(true);
	gameLock.lock();
	
	auto tickTime = intercept::sqf::time();
	auto gameTime = intercept::sqf::date();
	std::stringstream ss;
	ss << gameTime.hour << ":" << gameTime.minute;

	json j;
	j["ShooterId"] = unit.id;
	j["Pos"] = vectorToArray(intercept::sqf::get_pos(shooter));
	j["Velocity"] = vectorToArray(intercept::sqf::velocity(projectile));
	j["Ammo"] = ammo;
	j["Weapon"] = weapon;

	gameLock.unlock();
	
	JobItem item;
	item.TickTime = tickTime;
	item.GameTime = ss.str();
	item.Data = "UnitFired";
	item.Data = j.dump();
	
	client.addToQueue(item);
}

void echelon::Tracking::trackUnitDeleted(object& unit)
{
	std::lock_guard<std::mutex> lock(trackedUnitsMutex);
	auto gameLock = intercept::client::invoker_lock(true);

	auto unitSearch = trackedUnits.find(unit);
	if (unitSearch == trackedUnits.end())
	{
		Logger::WriteLog("Cannot find the unit to delete in map", Warning);
		return;
	}
	
	gameLock.lock();
	auto tickTime = intercept::sqf::time();
	auto gameTime = intercept::sqf::date();

	std::stringstream ss;
	ss << gameTime.hour << ":" << gameTime.minute;
	
	JobItem item;
	item.DataType = "UnitDeleted";
	item.GameTime = ss.str();
	item.TickTime = tickTime;
	json j;
	j["Id"] = unitSearch->second.id;
	j["IsPlayer"] = unitSearch->second.isPlayer;
	item.Data = j.dump();

	trackedUnits.erase(unitSearch);
	client.addToQueue(item);
}

void echelon::Tracking::trackVehicleDeleted(object& vehicle)
{
	std::lock_guard<std::mutex> lock(trackedVehiclesMutex);
	auto gameLock = intercept::client::invoker_lock(true);

	auto vehicleSearch = trackedVehicles.find(vehicle);
	if (vehicleSearch == trackedVehicles.end())
	{
		Logger::WriteLog("Cannot find the vehicle to delete in map", Warning);
		return;
	}

	gameLock.lock();
	auto tickTime = intercept::sqf::time();
	auto gameTime = intercept::sqf::date();

	std::stringstream ss;
	ss << gameTime.hour << ":" << gameTime.minute;

	JobItem item;
	item.DataType = "VehicleDeleted";
	item.GameTime = ss.str();
	item.TickTime = tickTime;
	json j;
	j["Id"] = vehicleSearch->second.id;
	item.Data = j.dump();

	trackedVehicles.erase(vehicleSearch);
	client.addToQueue(item);
}


game_value handle_cmd_init_callback(game_state& gs, game_value_parameter right_args)
{
	echelon::Tracking::get().trackNewObject(right_args);
}


void echelon::Tracking::doPositionUpdateUnits()
{
	std::list<JobItem> posUpdateList;
	std::lock_guard<std::mutex> lock(trackedUnitsMutex);
	auto gameLock = intercept::client::invoker_lock(true);

	gameLock.lock();
	auto tickTime = intercept::sqf::time();
	auto gameTime = intercept::sqf::date();
	std::stringstream ss;
	ss << gameTime.hour << ":" << gameTime.minute;
	
	for (auto trackedUnit : trackedUnits)
	{
		trackedUnit.second.lastPos = intercept::sqf::get_pos(trackedUnit.first);
		
		JobItem item;
		item.DataType = "UnitPosUpdate";
		item.GameTime = ss.str();
		item.TickTime = tickTime;
		json j;
		j["Id"] = trackedUnit.second.id;
		j["Pos"] = vectorToArray(trackedUnit.second.lastPos);
		j["Velocity"] = vectorToArray(intercept::sqf::velocity(trackedUnit.first));
		j["Heading"] = intercept::sqf::get_dir(trackedUnit.first);
		j["InVehicle"] = trackedUnit.second.inVehicle;
		item.Data = j.dump();
		posUpdateList.push_back(item);
	}
	gameLock.unlock();
	for (auto item : posUpdateList)
	{
		client.addToQueue(item);
	}
}

void echelon::Tracking::doPositionUpdateVehicles()
{
	std::list<JobItem> posUpdateList;
	std::lock_guard<std::mutex> lockVehicleList(trackedVehiclesMutex);
	auto gameLock = intercept::client::invoker_lock(true);

	gameLock.lock();
	auto tickTime = intercept::sqf::time();
	auto gameTime = intercept::sqf::date();
	std::stringstream ss;
	ss << gameTime.hour << ":" << gameTime.minute;
	for (auto trackedVehicle : trackedVehicles)
	{
		trackedVehicle.second.lastPos = intercept::sqf::get_pos(trackedVehicle.first);

		JobItem item;
		item.DataType = "UnitPosUpdate";
		item.GameTime = ss.str();
		item.TickTime = tickTime;
		json j;
		j["Id"] = trackedVehicle.second.id;
		j["Pos"] = vectorToArray(trackedVehicle.second.lastPos);
		j["Velocity"] = vectorToArray(intercept::sqf::velocity(trackedVehicle.first));
		j["Heading"] = intercept::sqf::get_dir(trackedVehicle.first);
		
		auto crewResult = intercept::sqf::full_crew(trackedVehicle.first);
		
		std::lock_guard<std::mutex> lockUnitList(trackedUnitsMutex);

		std::list<json> crewJsonList;
		for (auto crewMember : crewResult)
		{
			auto unit = trackedUnits.find(crewMember.unit);
			if (unit == trackedUnits.end())
				Logger::WriteLog("Unknown unit in vehicle pos update crew slot", Warning);

			json crewJson;
			crewJson["UnitId"] = unit->second.id;
			crewJson["Role"] = crewMember.role;
			crewJson["IsPlayer"] = unit->second.isPlayer;
			crewJson["UnitName"] = unit->second.name;
			crewJsonList.push_back(crewJson);
		}
		j["Crew"] = crewJsonList;

		item.Data = j.dump();
		posUpdateList.push_back(item);
	}
	
}

void echelon::Tracking::initCommands()
{
	cmd_init_callback = intercept::client::host::register_sqf_command("tf47handleObjectCreated", "intern use only", handle_cmd_init_callback, game_data_type::NOTHING, game_data_type::ARRAY);
}
