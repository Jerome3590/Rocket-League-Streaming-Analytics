#include "pch.h"
#include "Dashboard.h"
#include <bakkesmod/wrappers/GameWrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>
#include <bakkesmod/wrappers/GameEvent/GameEventWrapper.h>
#include <bakkesmod/wrappers/GameObject/TeamWrapper.h>
#include <bakkesmod/wrappers/GameObject/PriWrapper.h>
#include <bakkesmod/wrappers/GameEvent/TeamGameEventWrapper.h>
#include <bakkesmod/wrappers/Engine/ActorWrapper.h>
#include <bakkesmod/wrappers/Engine/ObjectWrapper.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/ListTablesRequest.h>
#include <aws/dynamodb/model/ListTablesResult.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/PutItemResult.h>
#include <aws/dynamodb/model/AttributeValue.h>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

BAKKESMOD_PLUGIN(Dashboard, "Rocket League Game Dashboard", "1.0", PLUGINTYPE_FREEPLAY)

void Dashboard::log(std::string msg) {
	cvarManager->log(msg);
}


void Dashboard::loadHooks() {
   
    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartEvent",
        std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function ProjectX.GRI_X.EventGameStarted",
        std::bind(&Dashboard::getGameData, this));
     gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartCountDown",
        std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame",
        std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.EndState",
        std::bind(&Dashboard::onUnload, this));
}


void Dashboard::onLoad() {
	this->log("Dashboard plugin started..");
	dynamoDbOps();
	this->loadHooks();
	
}


void Dashboard::dynamoDbOps() {
	{   
		Aws::SDKOptions options;
        InitAPI(options);
        
		{
		
		  dynamoClient = std::make_shared<Aws::DynamoDB::DynamoDBClient>();
		
		  Aws::DynamoDB::Model::ListTablesRequest req;
		  auto outcome = dynamoClient->ListTables(req);

		  if (outcome.IsSuccess()) {
		    	this->log("Your DynamoDB tables:");

		    	Aws::Vector<Aws::String> table_list = outcome.GetResult().GetTableNames();

			    for (auto const& table_name : table_list) {
				   this->log(table_name);
			    }
		  }
		   else {
			  this->log("ListTables error: " + outcome.GetError().GetExceptionName() + " - " + outcome.GetError().GetMessage());
		   }
	  }
	  
	  dynamoClient = std::make_shared<Aws::DynamoDB::DynamoDBClient>();

      ShutdownAPI(options); // 
	  
	}
    
}

// Function to upload JSON to DynamoDB (implement specific logic)
void Dashboard::uploadToDynamoDB(const std::string& gameID, const std::string& elapsedTime, nlohmann::json& gameJSON) {
    {

      using namespace Aws::DynamoDB::Model;

      PutItemRequest putItemRequest;

       // Set table name
       putItemRequest.SetTableName("arn:aws:dynamodb:us-east-1:432178471498:table/rocket_league_continuous_events");

       // Convert JSON to DynamoDB AttributeValue format
       Aws::Map<Aws::String, AttributeValue> item;
       item["Game_ID"] = AttributeValue(gameID);
       item["Elapsed_Time"] = AttributeValue(elapsedTime);

       for (auto& [key, value] : gameJSON.items()) {
           item[key] = AttributeValue(value.dump()); // Assuming all values are strings
        }

        // Set the item
        putItemRequest.SetItem(item);

        // Send the PutItem request
        auto outcome = dynamoClient->PutItem(putItemRequest);

        if (outcome.IsSuccess()) {
            this->log("Item successfully put into table.");
        } else {
           this->log("Error putting item into table: " + outcome.GetError().GetMessage());
        }
    }
}


void Dashboard::getGameData() {
    // Print output to console
    this->log("Get Game Data starting..\n");

    bool isInOnlineGame = gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame();

    if (isInOnlineGame) {
        // Retrieve Match GUID, elapsed time, and scores
        ServerWrapper server = gameWrapper->GetGameEventAsServer();
        
        // Check if server is null and exit function if true. Retry after 0.5 seconds
        if (server.IsNull()) {
            return;
        }

        // Log that a game server was found
        this->log("Game server found..\n");

        std::string gameID = server.GetMatchGUID();
        auto elapsedTime = server.GetTotalGameTimePlayed();
        std::string elapsedTimeString = std::to_string(elapsedTime); // Convert to string
        
        std::string score1 = std::to_string(gameWrapper->GetCurrentGameState().GetTeams().Get(0).GetScore());
        std::string score2 = std::to_string(gameWrapper->GetCurrentGameState().GetTeams().Get(1).GetScore());

        // Print output to console
        this->log("Game ID: " + gameID + "\n");
        this->log("Elapsed Time: " + elapsedTimeString + "\n");
        this->log("Score 1: " + score1 + "\n");
        this->log("Score 2: " + score2 + "\n");

        // Create JSON object every 0.5 seconds
        nlohmann::json gameJSON;
        gameJSON["Score Team 1"] = score1; 
        gameJSON["Score Team 2"] = score2;

        // Retrieve Player IDs, names, and goals
        ArrayWrapper<PriWrapper> pris = server.GetPRIs();
        for (int i = 0; i < pris.Count(); i++) {
            PriWrapper player = pris.Get(i);
            // Get player information
            std::string player_ID = std::to_string(player.GetPlayerID());
            std::string player_Name = player.GetPlayerName().ToString();
            std::string player_Goals = std::to_string(player.GetMatchGoals());
            // Retrieve the Flip Reset for player's car
            auto flipReset = std::to_string(player.GetCar().GetFlipComponent().CanActivate());
            // Log to console
            this->log("Player ID: " + player_ID);
            this->log("Player Name: " + player_Name);
            this->log("Player Goals: " + player_Goals);
            this->log("Flip Reset: " + flipReset);
            // Add to gameJSON
            nlohmann::json playerJSON;
            playerJSON["Player ID"] = player_ID;
            playerJSON["Player Name"] = player_Name;
            playerJSON["Player Goals"] = player_Goals;
            playerJSON["Flip Reset"] = flipReset;
            gameJSON["Players"].push_back(playerJSON);
        }
        
        // Upload JSON to DynamoDB (implement function to handle upload)
        uploadToDynamoDB(gameID, elapsedTimeString, gameJSON);

        // Increment the elapsed intervals
        elapsedIntervals++;

        if (elapsedIntervals < 700) {
            // Schedule this method to run again after 0.5 seconds
            gameWrapper->SetTimeout([this](GameWrapper* gw) {
                getGameData();
            }, 0.5f);
        }
    } // End of if(isInOnlineGame)
} // End of function



void Dashboard::onUnload() {
    elapsedIntervals = 0;
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.StartEvent");
    gameWrapper->UnhookEvent("Function ProjectX.GRI_X.EventGameStarted");
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.StartCountDown");
    gameWrapper->UnhookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame");
    gameWrapper->UnhookEvent("unction GameEvent_Soccar_TA.Active.EndState");
	this->log("Dashboard plugin unloaded..");
}
