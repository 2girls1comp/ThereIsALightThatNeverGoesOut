#include "script.h"
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <unordered_map>
#include <Windows.h>
#define bonfire


DWORD	vehUpdateTime;
DWORD	pedUpdateTime;
bool ModOn = true;
bool SnakeMode = false;
bool SwitchOn = false;
std::vector<Object> bonfires;

void spawnProp(Vector3 pos) {
	if (!SnakeMode) //bonfire
	{
		// Load the object model
		Hash thereIsALight = 0X25D88262;//bonfire model
		STREAMING::REQUEST_MODEL(thereIsALight, false);
		while (!STREAMING::HAS_MODEL_LOADED(thereIsALight)) {
			WAIT(0);
		}
		// Create the object
		Object obj = OBJECT::CREATE_OBJECT(thereIsALight, pos.x, pos.y, pos.z, true, true, false, false, true);

		// Add them to the bonfire list
		bonfires.push_back(obj);

		// Place it on the ground
		OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(obj, false);

		ENTITY::SET_ENTITY_AS_MISSION_ENTITY(obj, true, true);

		// Free model memory
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(thereIsALight);
	}
	else //snake
	{
		// Load the snake ped model
		Hash thereIsALight = 0x9547268E;//A_C_SnakeRedBoa_01
		STREAMING::REQUEST_MODEL(thereIsALight, false);
		while (!STREAMING::HAS_MODEL_LOADED(thereIsALight)) {
			WAIT(0);
		}
		// Create the ped
		Ped obj = PED::CREATE_PED(thereIsALight, pos.x, pos.y, pos.z, 0, true, true, true, true);

		// Add them to the bonfire list
		bonfires.push_back(obj);

		PED::SET_PED_RANDOM_COMPONENT_VARIATION(obj, true);

		ENTITY::SET_ENTITY_AS_MISSION_ENTITY(obj, true, true);

		// Free model memory
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(thereIsALight);
	}	
}

void WriteDB(Vector3 pos) {
	
	// Open file
	std::ofstream file("output.txt", std::ios::app);


	// Check if the file is open
	if (file.is_open()) {
		file << pos.x << "," << pos.y << "," << pos.z << "\n";
		file.close();  // Close the file
	}
}


void ReadDB() {

	// Open file
	std::ifstream file("output.txt");


	// Check if the file is open
	if (file.is_open()) {
		std::string line;

		while (std::getline(file, line)) {
			std::istringstream iss(line);
			std::string token;
			Vector3 pedPos;

			if (!std::getline(iss, token, ',')) continue;
			pedPos.x = std::stof(token);
			if (!std::getline(iss, token, ',')) continue;
			pedPos.y = std::stof(token);
			if (!std::getline(iss, token, ',')) continue;
			pedPos.z = std::stof(token);

			spawnProp(pedPos);

		}

		file.close();  // Close the file
	}
	else {
		std::ofstream file("output.txt");
		file.close();
	}

}

struct TrackedPed {
	Ped ped;
	bool hasSpawnedProp = false;


	TrackedPed(Ped p, bool spawned = false) : ped(p), hasSpawnedProp(spawned) {}
};
std::vector<TrackedPed> trackedPeds;


void TrackAllWorldPeds() {
	
	Ped peds[1024];
	int count = worldGetAllPeds(peds, 1024);
	Ped playerPed = PLAYER::PLAYER_PED_ID();

	//printout count of allpeds
	/*UI::SET_TEXT_SCALE(1, 1);
	UI::SET_TEXT_COLOR_RGBA(255, 255, 255, 255);
	UI::SET_TEXT_CENTRE(1);
	std::string str = std::to_string(count);
	UI::DRAW_TEXT(GAMEPLAY::CREATE_STRING(10, "LITERAL_STRING", const_cast<char*>(str.c_str())), 0.5f, 0.9f);*/


	for (int i = 0; i < count; i++) {
		Ped ped = peds[i];


		if (ped == playerPed || ENTITY::IS_ENTITY_DEAD(ped))
			continue;


		bool alreadyTracked = false;
		for (const auto& tracked : trackedPeds) {
			if (tracked.ped == ped) {
				alreadyTracked = true;
				break;
			}
		}
		if (!alreadyTracked) {
			trackedPeds.push_back(TrackedPed(ped, false));
		}
	}
}


void UpdateTrackedPeds() {
	for (auto it = trackedPeds.begin(); it != trackedPeds.end();)
	{
		Ped ped = it->ped;


		if (!ENTITY::DOES_ENTITY_EXIST(ped))
		{
			it = trackedPeds.erase(it);
			continue;
		}


		if (ENTITY::IS_ENTITY_DEAD(ped))
		{
			if (!it->hasSpawnedProp)
			{
				//get the coordinates of the ped
				Vector3 pedPos = ENTITY::GET_ENTITY_COORDS(ped, true, false);
				//write them to the database
				WriteDB(pedPos);
				spawnProp(pedPos);


				it->hasSpawnedProp = true;
			}


			//tell the game that the entity is under the script's control preventing the engine to automatically deleting it or managing it
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, true, true);


			//delete ped
			ENTITY::DELETE_ENTITY(&ped);


			//remove from tracking
			it = trackedPeds.erase(it);
		}
		else
		{
			++it;
		}
	}
}


void update()
{
	// update player information
	Player player = PLAYER::PLAYER_ID();
	Ped playerPed = PLAYER::PLAYER_PED_ID();


	// check if player ped exists and control is on (e.g. not in a cutscene)
	if (!ENTITY::DOES_ENTITY_EXIST(playerPed) || !PLAYER::IS_PLAYER_CONTROL_ON(player))
		return;
}

void clearAll() {
	//delete all objects
	for (int i = 0; i < bonfires.size(); i++)
	{
		ENTITY::DELETE_ENTITY(&bonfires[i]);
	}
	//clear the vector od objects
	bonfires.clear();

	//clear the vector of tracked peds
	trackedPeds.clear();
}



bool IsKeyJustUp(int vKey)
{
	static std::unordered_map<int, bool> keyStates;
	bool isDown = (GetAsyncKeyState(vKey) & 0x8000) != 0;
	bool wasDown = keyStates[vKey];
	keyStates[vKey] = isDown;
	return wasDown && !isDown; //Released this frame
}


void main()
{		

	while (true)
	{

		//toggle the mod on and off
		if (IsKeyJustUp(VK_F10)) 
		{
			ModOn = !ModOn;
			//if snake mode was active and we press the mod key, we turn off snake mode but keep the mod on and read the DB
			if (SnakeMode && !ModOn) {
				SnakeMode = false;
				clearAll();
				SwitchOn = false;
				ModOn = true;
			}
		}
		//toggle snake mode
		if (IsKeyJustUp(VK_F11)) 
		{
			SnakeMode = !SnakeMode;

			//turn ON snake mode
			if (SnakeMode) 
			{
				//if the mod is off we turn it on but avoid reading the DB
				if (!ModOn) {
					ModOn = true;
					SwitchOn = true;
				}
				//if the mod if already on we clear the list of bonfires
				else{
					clearAll();
				}
			}
				
			else {
				clearAll();
				SwitchOn = false;
			}
		}

		if (ModOn)
		{
			//we read the DB only the first time we load the 
			if (!SwitchOn) 
			{
				ReadDB();
				SwitchOn = true;
			}
			update();
			TrackAllWorldPeds();
			UpdateTrackedPeds();
		}
		WAIT(0);

		//when we turn off the mod we clear the bonfires and reset the switch bool
		if (!ModOn && SwitchOn) 
		{
			clearAll();
			SwitchOn = false;
		}
		WAIT(0);	
	}
}


void ScriptMain()
{	
	srand(GetTickCount());
	main();
}


