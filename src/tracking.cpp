#include "tracking.h"

echelon::JobItem echelon::Tracking::buildJobItem(std::string dataType, std::string& data)
{
	const auto gameTime = getGameTime();
	
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
	
	timeLastTrackingUnits = std::chrono::high_resolution_clock::now();
	timeLastTrackingVehicles = std::chrono::high_resolution_clock::now();
	timeLastTrackingProjectiles = std::chrono::high_resolution_clock::now();

	__SQF(
		addMissionEventHandler ["MarkerCreated", {
			params["_marker", "_channelNumber", "_owner", "_local"];
			tf47handleMarkerCreated [_marker, _channelNumber, _owner];
		}];
		addMissionEventHandler["MarkerUpdated", {
			params["_marker", "_local"];
			tf47handleMarkerUpdated [_marker];
		}];
		addMissionEventHandler["MarkerDeleted", {
			params["_marker", "_local"];
			tf47handleMarkerDeleted [_marker];
		}];
	);
}

void echelon::Tracking::stopTracking()
{
	stopWorkers = true;
	//posUpdateThread.join();
	//projectileUpdateThread.join();
}

void echelon::Tracking::handlePerFrame()
{
	auto timeDelta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - timeLastTrackingUnits);

	if (timeDelta.count() > 4000.0) {

		doPositionUpdateUnits();
		timeLastTrackingUnits = std::chrono::high_resolution_clock::now();
	}

	timeDelta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - timeLastTrackingVehicles);
	if (timeDelta.count() > 2000.0) {
		doPositionUpdateVehicles();
		timeLastTrackingVehicles = std::chrono::high_resolution_clock::now();
	}

	timeDelta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - timeLastTrackingProjectiles);
	if (timeDelta.count() > 1000.0)  {
		doProjectileUpdate();
		timeLastTrackingProjectiles = std::chrono::high_resolution_clock::now();
	}
}

void echelon::Tracking::trackNewObject(object newEntity)
{
	if (intercept::sqf::get_variable(newEntity, "TF47_tracking_disableTracking", false))
		return;

	JobItem item;

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
				trackFired(unit, projectile, weapon, ammo, gunner);
			});
		trackedUnit.getInManEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::GetInMan>(
			newEntity, [this](object unit, intercept::client::get_in_position role, object vehicle, rv_turret_path turret)
			{
				trackGetIn(unit, vehicle, role);
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
		trackedUnit.trackUnitDamaged = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Dammaged>(
			newEntity, [this](object unit, r_string, float damage, float hitPartIndex, r_string hitPoint, object shooter, object projectile)
			{
				trackUnitHit(unit, projectile, damage, hitPoint);
			});

		unitList.addUnit(trackedUnit);
		
		json j;
		j["UnitId"] = trackedUnit.id;
		j["Side"] = trackedUnit.side;
		j["Name"] = trackedUnit.name;
		j["SteamUid"] = trackedUnit.steamUid;

		item = buildJobItem("UnitCreated", j.dump());

		client.addToQueue(item);
		
		Logger::WriteLog("Added new unit to tracking");
	}
	if (objectType == "Vehicle")
	{
		auto trackedVehicle = TrackedVehicle();
		trackedVehicle.id = static_cast<long>(idCounter.fetch_add(1));
		const auto vehicleEntry = intercept::sqf::config_entry(intercept::sqf::config_file()) >> "CfgVehicles" >> intercept::sqf::type_of(newEntity);
		trackedVehicle.vehicle = newEntity;
		trackedVehicle.name = get_text(vehicleEntry >> "displayName");
		game_value vehicleType;
		__SQF(
			private _result = (_this call BIS_fnc_objectType) select 1;
			_result
		).capture("", vehicleType);
		
		trackedVehicle.vehicleType = vehicleType;
		trackedVehicle.fireEventhandler = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Fired>(
			newEntity, [this](object unit, r_string weapon, r_string muzzle, r_string mode, r_string ammo, r_string magazine, object projectile, object gunner)
			{
				trackFired(unit, projectile, weapon, ammo, gunner);
			});
		trackedVehicle.tackVehicleDeleted = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Deleted>(newEntity, [this](object entity)
			{
				trackVehicleDeleted(entity);
			});
		trackedVehicle.trackUnitDamaged = intercept::client::addEventHandler<intercept::client::eventhandlers_object::Dammaged>(
			newEntity, [this](object unit, r_string, float damage, float hitPartIndex, r_string hitPoint, object shooter, object projectile)
			{
				trackVehicleHit(unit, projectile, damage, hitPoint);
			});

		vehicleList.addVehicle(trackedVehicle);

		json j;
		j["VehicleId"] = trackedVehicle.id;
		j["Name"] = trackedVehicle.name;
		j["VehicleType"] = trackedVehicle.vehicleType;

		item = buildJobItem("VehicleCreated", j.dump());
		
		client.addToQueue(item);
		
		Logger::WriteLog("Added new vehicle to tracking");
	}

	
}

void echelon::Tracking::trackFired(object& shooter, object& projectile, r_string& weapon, r_string& ammo, object& gunner)
{
	TrackedProjectile trackedProjectile;
	trackedProjectile.id = idCounter.fetch_add(1);
	trackedProjectile.projectile = projectile;
	trackedProjectile.weapon = weapon;
	trackedProjectile.startPos = intercept::sqf::get_pos(projectile);
	trackedProjectile.velocity = intercept::sqf::velocity(projectile);
	trackedProjectile.shooter = shooter;
	
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

		item = buildJobItem("VehicleFired", j.dump());
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

		item = buildJobItem("UnitFired", j.dump());
	}
	client.addToQueue(item);
}

void echelon::Tracking::trackUnitHit(object& unit, object& projectile, float& damage, r_string& hitPoint)
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
	j["Damage"] = damage;
	j["HitPoint"] = hitPoint.c_str();

	auto jobItem = buildJobItem("UnitHit", j.dump());

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

	auto jobItem = buildJobItem("UnitDeleted", j.dump());
	
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

void echelon::Tracking::trackVehicleHit(object& vehicle, object& projectile, float& damage, r_string& hitPart)
{
	auto gameLock = intercept::client::invoker_lock(true);
	
	gameLock.lock();
	if (!intercept::sqf::alive(vehicle)) return;

	auto gameTime = getGameTime();

	auto* const trackedProjectile = projectileList.findProjectile(projectile);
	if (trackedProjectile == nullptr)
	{
		Logger::WriteLog("Failed to track hit on vehicle, cannot find projectile", Warning);
		gameLock.unlock();
		return;
	}

	auto* const trackedVehicle = vehicleList.findVehicle(vehicle);
	if (trackedVehicle == nullptr)
	{
		Logger::WriteLog("Failed to track hit on vehicle, cannot find hit unit", Warning);
		gameLock.unlock();
		return;
	}

	auto posVehicle = intercept::sqf::get_pos(vehicle);

	json j;
	j["VehicleId"] = trackedVehicle->id;
	j["Pos"] = vectorToArray(posVehicle);
	j["Distance"] = posVehicle.distance(trackedProjectile->startPos);
	j["Ammo"] = trackedProjectile->ammo;
	j["Weapon"] = trackedProjectile->weapon;
	j["Damage"] = damage;
	j["HitPart"] = hitPart.c_str();

	auto jobItem = buildJobItem("VehicleHit", j.dump());

	client.addToQueue(jobItem);
	gameLock.unlock();
}

void echelon::Tracking::trackGetIn(object& unit, object& vehicle, intercept::client::get_in_position& role)
{
	auto* const trackedVehicle = vehicleList.findVehicle(vehicle);
	if (trackedVehicle == nullptr)
	{
		Logger::WriteLog("Failed to get vehicle unit tried to get into", Warning);
		return;
	}
	auto* const trackedUnit = unitList.findUnit(unit);
	if (trackedUnit == nullptr)
	{
		Logger::WriteLog("Failed to get unit the tried to get into", Warning);
		return;
	}
	
	json j;
	j["UnitId"] = trackedUnit->id;
	j["Role"] = role;
	j["VehicleId"] = trackedVehicle->id;

	trackedUnit->inVehicle = true;

	auto jobItem = buildJobItem("UnitEnteredVehicle", j.dump());
	client.addToQueue(jobItem);
}

void echelon::Tracking::trackGetOut(object& unit, object& vehicle, intercept::client::get_in_position& role)
{
	auto* const trackedVehicle = vehicleList.findVehicle(vehicle);
	if (trackedVehicle == nullptr)
	{
		Logger::WriteLog("Failed to get vehicle unit tried to get out of vehicle", Warning);
		return;
	}
	auto* const trackedUnit = unitList.findUnit(unit);
	if (trackedUnit == nullptr)
	{
		Logger::WriteLog("Failed to get unit the tried to out of vehicle", Warning);
		return;
	}

	json j;
	j["UnitId"] = trackedUnit->id;
	j["Role"] = role;
	j["VehicleId"] = trackedVehicle->id;

	trackedUnit->inVehicle = false;

	auto jobItem = buildJobItem("UnitLeftVehicle", j.dump());
	client.addToQueue(jobItem);
}

void echelon::Tracking::trackMarkerCreated(intercept::types::marker& marker, int& channelNumber, object& owner)
{
	auto gameTime = getGameTime();
	
	intercept::client::invoker_lock gameLock(true);
	gameLock.lock();
	
	TrackedMarker trackedMarker;
	trackedMarker.id = idCounter.fetch_add(1);
	trackedMarker.marker = marker;
	trackedMarker.channelId = channelNumber;
	trackedMarker.type = intercept::sqf::get_marker_type(marker);
	trackedMarker.shape = intercept::sqf::marker_shape(marker);
	trackedMarker.pos = intercept::sqf::get_marker_pos(marker);
	trackedMarker.size = intercept::sqf::get_marker_size(marker);
	trackedMarker.color = intercept::sqf::get_marker_color(marker);
	trackedMarker.text = intercept::sqf::marker_text(marker);
	trackedMarker.timeCreated = std::chrono::high_resolution_clock::now();

	gameLock.unlock();
	
	markerList.addMarker(trackedMarker);


	std::async(std::launch::async, [&]
		{
			std::this_thread::sleep_for(std::chrono::seconds(2));
			json j;
			j["MarkerId"] = trackedMarker.id;
			j["ChannelId"] = trackedMarker.channelId;
			j["Color"] = trackedMarker.color;
			j["Size"] = vectorToArray(trackedMarker.size);
			j["Shape"] = trackedMarker.shape;
			j["Pos"] = vectorToArray(trackedMarker.pos);
			j["Text"] = trackedMarker.text;
			j["Type"] = trackedMarker.type;

			JobItem item;
			item.Id = packageCounter.fetch_add(1);
			item.DataType = "MarkerCreated";
			item.Data = j.dump();
			item.GameTime = gameTime.Time;
			item.TickTime = gameTime.TickTime;

			client.addToQueue(item);
		});
}

void echelon::Tracking::trackMarkerDeleted(marker& marker)
{
	auto* const markerTracked = markerList.findMarker(marker);
	if (markerTracked == nullptr)
	{
		Logger::WriteLog("Failed to find marker tracked in list", Warning);
		return;
	}
	
	json j;
	j["MarkerId"] = markerTracked->id;
	
	markerList.removeMarker(marker);

	auto jobItem = buildJobItem("MarkerDeleted", j.dump());
	client.addToQueue(jobItem);
}

void echelon::Tracking::trackMarkerUpdated(marker& marker)
{
	auto gameTime = getGameTime();
	
	auto* const trackedMarker = markerList.findMarker(marker);
	trackedMarker->type = intercept::sqf::get_marker_type(marker);
	trackedMarker->shape = intercept::sqf::marker_shape(marker);
	trackedMarker->pos = intercept::sqf::get_marker_pos(marker);
	trackedMarker->size = intercept::sqf::get_marker_size(marker);
	trackedMarker->color = intercept::sqf::get_marker_color(marker);
	trackedMarker->text = intercept::sqf::marker_text(marker);

	const auto timeDelta = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - trackedMarker->timeCreated);
	;
	if (timeDelta.count() > 2000.0)
	{
		json j;
		j["MarkerId"] = trackedMarker->id;
		j["ChannelId"] = trackedMarker->channelId;
		j["Color"] = trackedMarker->color;
		j["Size"] = vectorToArray(trackedMarker->size);
		j["Shape"] = trackedMarker->shape;
		j["Pos"] = vectorToArray(trackedMarker->pos);
		j["Text"] = trackedMarker->text;
		j["Type"] = trackedMarker->type;

		auto jobItem = buildJobItem("MarkerUpdated", j.dump());
		client.addToQueue(jobItem);
	}
}

void echelon::Tracking::doPositionUpdateUnits()
{
	std::vector<json> posJsonList;
	
	for (auto trackedUnit : unitList.getUnitList())
	{
		trackedUnit.lastPos = intercept::sqf::get_pos(trackedUnit.unit);
		trackedUnit.inVehicle = !(intercept::sqf::vehicle(trackedUnit.unit) == trackedUnit.unit);
		
		json j;
		j["Id"] = trackedUnit.id;
		j["Pos"] = vectorToArray(trackedUnit.lastPos);
		j["Velocity"] = vectorToArray(intercept::sqf::velocity(trackedUnit.unit));
		j["Heading"] = intercept::sqf::get_dir(trackedUnit.unit);
		j["InVehicle"] = trackedUnit.inVehicle;
		posJsonList.push_back(j);
	}
	const json j = posJsonList;
	
	client.addToQueue(buildJobItem("PosUpdateUnits", j.dump()));
}

void echelon::Tracking::doPositionUpdateVehicles()
{	
	auto gameTime = getGameTime();

	std::vector<json> posJsonList;

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

		posJsonList.push_back(j);
	}

	json j;
	j = posJsonList;
	
	client.addToQueue(buildJobItem("PosUpdateVehicle", j.dump()));
}

void echelon::Tracking::doProjectileUpdate()
{
	projectileList.removeDeletedProjectiles();

	std::vector<json> projectileJsonList;
	
	for (auto projectile : projectileList.getProjectileList())
	{
		json j;
		j["Id"] = projectile.id;
		j["Pos"] = vectorToArray(intercept::sqf::get_pos(projectile.projectile));
		j["Velocity"] =	vectorToArray(intercept::sqf::velocity(projectile.projectile));
		
		projectileJsonList.push_back(j);
	}

	const json j = projectileJsonList;
	
	client.addToQueue(buildJobItem("PosUpdateProjectile", j.dump()));
}


game_value handle_cmd_init_callback(game_state& gs, game_value_parameter right_args)
{
	echelon::Tracking::get().trackNewObject(right_args);
	return "";
}

game_value handle_cmd_marker_created(game_state& gs, game_value_parameter right_args)
{
	marker newMarker = right_args[0];
	int channelNumber = right_args[1];
	object owner = right_args[2];
	echelon::Tracking::get().trackMarkerCreated(newMarker, channelNumber, owner);
	return "";
}

game_value handle_cmd_marker_updated(game_state& gs, game_value_parameter right_args)
{
	marker marker = right_args[0];
	echelon::Tracking::get().trackMarkerUpdated(marker);
	return "";
}

game_value handle_cmd_marker_deleted(game_state& gs, game_value_parameter right_args)
{
	marker marker = right_args[0];
	echelon::Tracking::get().trackMarkerDeleted(marker);
	return "";
}


void echelon::Tracking::initCommands()
{
	cmd_init_callback = intercept::client::host::register_sqf_command("tf47handleObjectCreated", "intern use only", handle_cmd_init_callback, game_data_type::NOTHING, game_data_type::OBJECT);
	cmd_marker_created_callback = intercept::client::host::register_sqf_command("tf47handleMarkerCreated", "intern use only", handle_cmd_marker_created, game_data_type::STRING, game_data_type::ARRAY);
	cmd_marker_updated_callback = intercept::client::host::register_sqf_command("tf47handleMarkerUpdated", "intern use only", handle_cmd_marker_updated, game_data_type::STRING, game_data_type::ARRAY);
	cmd_marker_deleted_callback = intercept::client::host::register_sqf_command("tf47handleMarkerDeleted", "intern use only", handle_cmd_marker_deleted, game_data_type::STRING, game_data_type::ARRAY);
}
