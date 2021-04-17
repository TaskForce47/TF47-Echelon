#include "tracking.h"

echelon::JobItem echelon::Tracking::buildJobItem(std::string dataType, std::string& data, GameTime& gameTime)
{
	JobItem item;
	item.Id = packageCounter.fetch_add(1);
	item.DataType = dataType;
	item.Data = data;
	item.GameTime = gameTime.Time;
	item.TickTime = gameTime.TickTime;
	return item;
}

echelon::GameTime echelon::Tracking::getGameTime()
{
	intercept::client::invoker_lock lock;
	
	GameTime gameTime;
	gameTime.TickTime = intercept::sqf::time();
	const auto date = intercept::sqf::date();
	std::stringstream ss;
	ss << date.hour << ":" << date.minute;
	gameTime.Time = ss.str();
	return gameTime;
}

echelon::Tracking::Tracking()
{
}

echelon::Tracking::~Tracking()
{
	stopTracking();
}


void echelon::Tracking::startTracking()
{
	Logger::WriteLog("Started tracking!");
	stopWorkers = false;

	intercept::client::invoker_lock lock;
	
	if (!Config::get().isSessionRunning())
		client.createSession(intercept::sqf::world_name());

	client.startBackgroundWorker();
	
	//try to read settings
	intercept::sqf::config_entry entry = intercept::sqf::config_entry(intercept::sqf::mission_config_file()) >> ("TF47TrackingSystems");
	
	__SQF(
		[
			"All",
			"init",
			{
				[{tf47handleObjectCreated (_this select 0)}, _this, 1] call CBA_fnc_waitAndExecute;
			},
			true,
			[],
			true
		] call CBA_fnc_addClassEventHandler;
	);

	posUpdateThread = std::thread([this]
		{
			while (!stopWorkers)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Config::get().getPosUpdateIntervalUnits()));
				
				intercept::client::invoker_lock lock;
				doPositionUpdateUnits();
				doPositionUpdateVehicles();
			}
		});
	projectileUpdateThread = std::thread([this]
		{
			while(!stopWorkers)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(Config::get().getPosUpdateIntervalProjectiles()));

				intercept::client::invoker_lock lock;
				doProjectileUpdate();
			}
		});
}

void echelon::Tracking::stopTracking()
{
	stopWorkers = true;
	posUpdateThread.join();
	projectileUpdateThread.join();
}

void echelon::Tracking::trackNewObject(object newEntity)
{
	if (intercept::sqf::get_variable(newEntity, "TF47_tracking_disableTracking", false))
		return;

	JobItem item;

	auto gameTime = getGameTime();

	intercept::client::invoker_lock gameLock;

	game_value objectType;
	__SQF(
		(_this call BIS_fnc_objectType) select 0;
	).capture(newEntity, objectType);
	
	if (objectType == "Soldier")
	{
		auto trackedUnit = TrackedUnit();
		trackedUnit.id = static_cast<long>(idCounter.fetch_add(1));
		trackedUnit.isPlayer = intercept::sqf::is_player(newEntity);
		trackedUnit.name = intercept::sqf::name(newEntity);
		trackedUnit.steamUid = intercept::sqf::get_player_uid(newEntity);
		trackedUnit.unit = newEntity;
		trackedUnit.side = intercept::sqf::side_get(intercept::sqf::get_group(newEntity));
		trackedUnit.fireEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Fired>(
			newEntity, [this](object unit, r_string weapon, r_string muzzle, r_string mode, r_string ammo, r_string magazine, object projectile, object gunner)
			{
				trackFired(unit, projectile, std::string(weapon), std::string(ammo), gunner);
			});
		trackedUnit.getInManEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::GetInMan>(
			newEntity, [this](object unit, intercept::client::get_in_position role, object vehicle, rv_turret_path turret)
			{
				//trackGetIn(unit, vehicle, role);
			});
		trackedUnit.getOutEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::GetOutMan>(
			newEntity, [this](object unit, intercept::client::get_in_position role, object vehicle, rv_turret_path turret)
			{
				//trackGetOut(unit, vehicle, role);
			});
		trackedUnit.trackUnitDeleted = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Deleted>(
			newEntity, [this](object entity)
			{
				trackUnitDeleted(entity);
			});

		__SQF(
		[
			_this,
			{
				_this addEventhandler	[
					"HitPart",
					{
						{
							_x params["_target", "_shooter", "_projectile", "_position", "_velocity", "_selection", "_ammo", "_vector", "_radius", "_surfaceType", "_isDirect"];
							[
								[
									_target,
									_projectile,
									_isDirectHit,
									_selection
								]
							] remoteExec ["tf47handleUnitHit", 2];
						} foreach _this;
					}
				];
			}
		] remoteExec ["BIS_fnc_call", 0, true];
		).capture(newEntity);
		
		unitList.addUnit(trackedUnit);
		
		json j;
		j["UnitId"] = trackedUnit.id;
		j["Side"] = trackedUnit.side;
		j["Name"] = trackedUnit.name;
		j["SteamUid"] = trackedUnit.steamUid;

		item = buildJobItem("UnitCreated", j.dump(), gameTime);

		client.addToQueue(item);
		
		Logger::WriteLog("Added new unit to tracking");
	}
	if (objectType == "Vehicle")
	{
		auto trackedVehicle = TrackedVehicle();
		trackedVehicle.id = static_cast<long>(++idCounter);
		const auto vehicleEntry = intercept::sqf::config_entry(intercept::sqf::config_file()) >> "CfgVehicles" >> intercept::sqf::type_of(newEntity);
		trackedVehicle.vehicle = newEntity;
		trackedVehicle.name = get_text(vehicleEntry >> "displayName");
		game_value vehicleType;
		__SQF(
			private _result = (_this call BIS_fnc_objectType) select 1;
			_result
		).capture("", vehicleType);
		
		trackedVehicle.vehicleType = vehicleType;
		/*
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
			
			});*/

		vehicleList.addVehicle(trackedVehicle);

		json j;
		j["VehicleId"] = trackedVehicle.id;
		j["Name"] = trackedVehicle.name;
		j["VehicleType"] = trackedVehicle.vehicleType;

		item = buildJobItem("VehicleCreated", j.dump(), gameTime);
		
		client.addToQueue(item);
		
		Logger::WriteLog("Added new vehicle to tracking");
	}

	
}

void echelon::Tracking::trackFired(object& shooter, object& projectile, std::string& weapon, std::string& ammo, object& gunner)
{
	TrackedProjectile trackedProjectile;
	trackedProjectile.id = idCounter.fetch_add(idCounter);
	trackedProjectile.projectile = projectile;
	trackedProjectile.weapon = weapon;
	trackedProjectile.startPos = intercept::sqf::get_pos(projectile);
	trackedProjectile.velocity = intercept::sqf::velocity(projectile);
	trackedProjectile.shooter = shooter;

	auto gameTime = getGameTime();
	
	auto vehicleShooter = intercept::sqf::vehicle(shooter);
	if (vehicleShooter != shooter)
	{
		trackedProjectile.shooterIsVehicle = true;
		trackedProjectile.vehicle = shooter;
		trackedProjectile.shooter = gunner;

		auto* const trackedVehicle = vehicleList.findVehicle(vehicleShooter);
		if (trackedVehicle == nullptr)
		{
			Logger::WriteLog("Could not map shooter vehicle to projectile", Warning);
			return;
		}
		trackedProjectile.trackedVehicle = *trackedVehicle;
	}
	else
	{
		trackedProjectile.shooter = shooter;
		trackedProjectile.shooterIsVehicle = false;
	}
	
	projectileList.addProjectile(trackedProjectile);

	JobItem item;
	if (trackedProjectile.shooterIsVehicle)
	{
		auto* const trackedUnit = unitList.findUnit(shooter);
		if (trackedUnit == nullptr)
		{
			Logger::WriteLog("Failed to get unit who shot", Warning);
			return;
		}

		auto* const trackedVehicle = projectileList.findProjectile(trackedProjectile.vehicle);
		if (trackedVehicle == nullptr)
		{
			Logger::WriteLog("Failed to get vehicle who shot", Warning);
			return;
		}

		json j;
		j["ShooterId"] = trackedUnit->id;
		j["VehicleId"] = trackedVehicle->id;
		j["Pos"] = vectorToArray(trackedProjectile.startPos);
		j["Velocity"] = vectorToArray(trackedProjectile.velocity);
		j["Ammo"] = trackedProjectile.ammo;
		j["Weapon"] = trackedProjectile.weapon;

		item = buildJobItem("VehicleFired", j.dump(), gameTime);
	}
	else 
	{
		auto* const unit = unitList.findUnit(shooter);
		if (unit == nullptr)
		{
			Logger::WriteLog("Failed to get unit who shot", Warning);
			return;
		}

		json j;
		j["ShooterId"] = unit->id;
		j["Pos"] = vectorToArray(trackedProjectile.startPos);
		j["Velocity"] = vectorToArray(trackedProjectile.velocity);
		j["Ammo"] = trackedProjectile.ammo;
		j["Weapon"] = trackedProjectile.weapon;

		item = buildJobItem("UnitFired", j.dump(), gameTime);
	}
	client.addToQueue(item);
}

void echelon::Tracking::trackUnitHit(object& unit, object& projectile, bool& directHit, std::vector<std::string> selection)
{
	auto gameLock = intercept::client::invoker_lock(true);
	gameLock.lock();
	if (! intercept::sqf::alive(unit)) return;

	auto gameTime = getGameTime();

	auto* const trackedProjectile = projectileList.findProjectile(projectile);
	if (trackedProjectile == nullptr)
	{
		Logger::WriteLog("Failed to track hit on unit, cannot find projectile", Warning);
		gameLock.unlock();
		return;
	}

	auto* const trackedUnit = unitList.findUnit(unit);
	if (trackedUnit == nullptr)
	{
		Logger::WriteLog("Failed to track hit on unit, cannot find hit unit", Warning);
		gameLock.unlock();
		return;
	}
	
	auto posUnit = intercept::sqf::get_pos(unit);
	
	json j;
	j["UnitId"] = trackedUnit->id;
	j["Pos"] = vectorToArray(posUnit);
	j["Distance"] = posUnit.distance(trackedProjectile->startPos);
	j["Ammo"] = trackedProjectile->ammo;
	j["Weapon"] = trackedProjectile->weapon;
	j["DirectHit"] = directHit;
	j["Selection"] = selection;

	auto jobItem = buildJobItem("UnitHit", j.dump(), gameTime);

	client.addToQueue(jobItem);
	gameLock.unlock();
}

void echelon::Tracking::trackUnitDeleted(object& unit)
{
	auto* const unitTracked = unitList.findUnit(unit);
	if (unitTracked == nullptr)
	{
		Logger::WriteLog("Cannot find the unit to delete in map", Warning);
		return;
	}

	auto gameTime = getGameTime();
	
	json j;
	j["Id"] = unitTracked->id;
	j["IsPlayer"] = unitTracked->isPlayer;

	unitList.removeUnit(unit);

	auto jobItem = buildJobItem("UnitDeleted", j.dump(), gameTime);
	
	client.addToQueue(jobItem);
}

void echelon::Tracking::trackVehicleDeleted(object& vehicle)
{
	auto* const vehicleTracked = vehicleList.findVehicle(vehicle);
	if (vehicleTracked == nullptr)
	{
		Logger::WriteLog("Cannot find the vehicle to delete in map", Warning);
		return;
	}

	const auto gameTime = getGameTime();

	JobItem item;
	item.DataType = "VehicleDeleted";
	item.GameTime = gameTime.Time;
	item.TickTime = gameTime.TickTime;
	json j;
	j["Id"] = vehicleTracked->id;
	item.Data = j.dump();

	vehicleList.removeVehicle(vehicle);
	
	client.addToQueue(item);
}

void echelon::Tracking::doPositionUpdateUnits()
{
	std::list<JobItem> posUpdateList;

	auto gameTime = getGameTime();
	
	for (auto trackedUnit : unitList.getUnitList())
	{
		trackedUnit.lastPos = intercept::sqf::get_pos(trackedUnit.unit);
		
		JobItem item;
		item.DataType = "PosUpdateUnit";
		item.GameTime = gameTime.Time;
		item.TickTime = gameTime.TickTime;
		json j;
		j["Id"] = trackedUnit.id;
		j["Pos"] = vectorToArray(trackedUnit.lastPos);
		j["Velocity"] = vectorToArray(intercept::sqf::velocity(trackedUnit.unit));
		j["Heading"] = intercept::sqf::get_dir(trackedUnit.unit);
		j["InVehicle"] = trackedUnit.inVehicle;
		item.Data = j.dump();
		posUpdateList.push_back(item);
	}

	for (auto item : posUpdateList)
	{
		client.addToQueue(item);
	}
}

void echelon::Tracking::doPositionUpdateVehicles()
{
	std::list<JobItem> posUpdateList;
	
	auto gameTime = getGameTime();
	
	for (auto trackedVehicle : vehicleList.getVehicleList())
	{
		trackedVehicle.lastPos = intercept::sqf::get_pos(trackedVehicle.vehicle);

		json j;
		j["Id"] = trackedVehicle.id;
		j["Pos"] = vectorToArray(trackedVehicle.lastPos);
		j["Velocity"] = vectorToArray(intercept::sqf::velocity(trackedVehicle.vehicle));
		j["Heading"] = intercept::sqf::get_dir(trackedVehicle.vehicle);
		
		auto crewResult = intercept::sqf::full_crew(trackedVehicle.vehicle);

		std::list<json> crewJsonList;
		for (auto crewMember : crewResult)
		{
			auto* const unit = unitList.findUnit(crewMember.unit);
			if (unit == nullptr) {
				Logger::WriteLog("Unknown unit in vehicle pos update crew slot", Warning);
				continue;
			}

			json crewJson;
			crewJson["UnitId"] = unit->id;
			crewJson["Role"] = crewMember.role;
			crewJson["IsPlayer"] = unit->isPlayer;
			crewJson["UnitName"] = unit->name;
			crewJsonList.push_back(crewJson);
		}
		j["Crew"] = crewJsonList;

		auto jobItem = buildJobItem("PosUpdateVehicle", j.dump(), gameTime);
		
		posUpdateList.push_back(jobItem);
	}
	for (auto item : posUpdateList)
	{
		client.addToQueue(item);
	}
}

void echelon::Tracking::doProjectileUpdate()
{
	projectileList.removeDeletedProjectiles();

	auto gameTime = getGameTime();
	
	for (auto projectile : projectileList.getProjectileList())
	{
		json j;
		j["Id"] = projectile.id;
		j["Pos"] = vectorToArray(intercept::sqf::get_pos(projectile.projectile));
		j["Velocity"] =	vectorToArray(intercept::sqf::velocity(projectile.projectile));

		auto jobItem = buildJobItem("PosUpdateProjectile", j.dump(), gameTime);
		
		client.addToQueue(jobItem);
	}
}


game_value handle_cmd_init_callback(game_state& gs, game_value_parameter right_args)
{
	echelon::Tracking::get().trackNewObject(right_args);
	return "";
}

game_value handle_cmd_unit_hit(game_state& gs, game_value_parameter right_args)
{
	object target = right_args[0];
	object projectile = right_args[1];
	bool directHit = right_args[2];

	std::vector<std::string> selectionStrings;
	auto selections = right_args[3].to_array();
	for (game_value selection : selections)
	{
		selectionStrings.push_back(selection);
	}

	echelon::Tracking::get().trackUnitHit(target, projectile, directHit, selectionStrings);
	return "";
}

void echelon::Tracking::initCommands()
{
	cmd_init_callback = intercept::client::host::register_sqf_command("tf47handleObjectCreated", "intern use only", handle_cmd_init_callback, game_data_type::NOTHING, game_data_type::OBJECT);
	cmd_unit_hit_callback = intercept::client::host::register_sqf_command("tf47handleUnitHit", "intern use only", handle_cmd_unit_hit, game_data_type::STRING, game_data_type::ARRAY);
}
