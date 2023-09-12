#include "pch.h"
#include "Dashboard.h"
#include <bakkesmod/wrappers/GameWrapper.h>
#include <bakkesmod/wrappers/GameEvent/ServerWrapper.h>
#include <bakkesmod/wrappers/GameEvent/GameEventWrapper.h>
#include <bakkesmod/wrappers/GameObject/TeamWrapper.h>
#include <bakkesmod/wrappers/GameObject/TeamInfoWrapper.h>
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

#define UPDATE_PRIDATA_EVENT_NAME "Function TAGame.GFxHUD_TA.UpdatePRIData"

BAKKESMOD_PLUGIN(Dashboard, "Rocket League Game Dashboard", "1.0", PLUGINTYPE_FREEPLAY)



void Dashboard::log(std::string msg) {
    if (!cvarManager) return;
	cvarManager->log(msg);
}


void Dashboard::loadHooks() {

  //  gameWrapper->HookEventPost(UPDATE_PRIDATA_EVENT_NAME, 
  //       [this] (std::string eventName){ getGameData(); });
    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartEvent",
          std::bind(&Dashboard::getGameData, this));
     gameWrapper->HookEvent("Function ProjectX.GRI_X.EventGameStarted",
         std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartCountDown",
      std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame",
         std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.EndState",
         std::bind(&Dashboard::isGamePaused, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartEvent",
          std::bind(&Dashboard::isGamePlaying, this));
     gameWrapper->HookEvent("Function ProjectX.GRI_X.EventGameStarted",
         std::bind(&Dashboard::isGamePlaying, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartCountDown",
      std::bind(&Dashboard::isGamePlaying, this));
    gameWrapper->HookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame",
         std::bind(&Dashboard::isGamePlaying, this));
}


void Dashboard::onLoad() {
	this->log("Dashboard plugin started..");
    elapsedIntervals = 0;
	dynamoDbOps();
	this->loadHooks();
	
}


bool Dashboard::isGamePaused() {
    this->log("Game play paused..");
    gamePaused = true; // Change this to member variable
    return gamePaused;
}


bool Dashboard::isGamePlaying() {
    this->log("Game play resumed..");
    gamePaused = false; // Change this to member variable
    return !gamePaused;
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

      //ShutdownAPI(options); 
	  
	}
    
}

void Dashboard::uploadToDynamoDB(
const std::string& gameID, const std::string& elapsedTimeString, 
const std::string& team0Name, const std::string& team1Name, 
const std::string& team0Score, const std::string& team1Score, 
const std::string& team0PlayerName1, const std::string& team0PlayerName2, 
const std::string& team1PlayerName1, const std::string& team1PlayerName2,
const std::string& team0Player1FlipReset, const std::string& team0Player2FlipReset, 
const std::string& team1Player1FlipReset, const std::string& team1Player2FlipReset) {
    using namespace Aws::DynamoDB::Model;

    PutItemRequest putItemRequest;

    // Set table name
    putItemRequest.SetTableName("rocket_league_continuous_events");

    // Create an item with string and number attributes
    Aws::Map<Aws::String, AttributeValue> item;
    item["Game_ID"] = AttributeValue(gameID);
    item["Elapsed_Time"] = AttributeValue(elapsedTimeString);
    item["Team0_Name"] = AttributeValue(team0Name);
    item["Team1_Name"] = AttributeValue(team1Name);
    item["Team0_Score"] = AttributeValue(team0Score);
    item["Team1_Score"] = AttributeValue(team1Score);
    item["team0_PlayerName1"] = AttributeValue(team0PlayerName1);
    item["team0_PlayerName2"] = AttributeValue(team0PlayerName2);
    item["team1_PlayerName1"] = AttributeValue(team1PlayerName1);
    item["team1_PlayerName2"] = AttributeValue(team1PlayerName2);
    item["team0_Player1_Flip_Reset"] = AttributeValue(team0Player1FlipReset);
    item["team0_Player2_Flip_Reset"] = AttributeValue(team0Player2FlipReset);
    item["team1_Player1_Flip_Reset"] = AttributeValue(team1Player1FlipReset);
    item["team1_Player2_Flip_Reset"] = AttributeValue(team1Player2FlipReset);


    // Set the item in the put request
    putItemRequest.SetItem(item);

    // Send the PutItem request
    auto outcome = dynamoClient->PutItem(putItemRequest);

    if (outcome.IsSuccess()) {
        this->log("Item successfully put into table.");
    } else {
        this->log("Error putting item into table: " + outcome.GetError().GetMessage());
    }
}



void Dashboard::getGameData() {

    // Print output to console
    this->log("Reached Get Game Data..\n");
    
    // Print output to console
    this->log("Get Game Data starting..\n");

    if(!gameWrapper) {
        return;
    }
    // Check if in online game
    bool isInOnlineGame = gameWrapper->IsInOnlineGame() || gameWrapper->IsSpectatingInOnlineGame();

    if (isInOnlineGame && !gamePaused) {
        // Retrieve Match GUID, elapsed time, and scores
        ServerWrapper server = gameWrapper->GetOnlineGame();
        
        this->log("Checking game server..\n"); 
         
        // Check if server is null and exit function if true. Retry after 0.5 seconds
        if (server.IsNull()) {
            this->log("Game server is null..\n");
            return;
        }

        // Log that a game server was found
        this->log("Game server found..\n");

        std::string gameID = server.GetMatchGUID();
        const int elapsedTime = server.GetbOverTime() ? -server.GetSecondsRemaining() : server.GetSecondsRemaining();
        std::string elapsedTimeString = std::to_string(elapsedTime); // Convert Time to string

        ServerWrapper gameState = gameWrapper->GetCurrentGameState();
        if(gameState.IsNull()) {
            return;
        }
        auto teams = gameState.GetTeams();
        if(teams.IsNull() && teams.Count() < 2) { 
            return;
        }

        TeamWrapper teams0 = teams.Get(0);
        TeamWrapper teams1 = teams.Get(1);

        // Log that a game server was found
        this->log("Teams found..\n"); 

        if(teams0.IsNull() || teams1.IsNull()) {
            return;
        }

        std::string team0Name = teams0.GetTeamName().ToString();
        std::string team1Name = teams1.GetTeamName().ToString();
        std::string team0Score = std::to_string(teams0.GetScore());
        std::string team1Score = std::to_string(teams1.GetScore());
        

        // Retrieve Player IDs, names, and goals
        ArrayWrapper<PriWrapper> pris = server.GetPRIs();

        std::string team0PlayerName1 = "";
        std::string team0PlayerName2 = "";
        std::string team1PlayerName1 = "";
        std::string team1PlayerName2 = "";
        std::string team0Player1FlipReset = "";
        std::string team0Player2FlipReset = "";
        std::string team1Player1FlipReset = "";
        std::string team1Player2FlipReset = "";
        int team0PlayerCount = 0;
        int team1PlayerCount = 0;


        for (int i = 0; i < pris.Count(); i++) {
            PriWrapper player = pris.Get(i);
            if (player.IsNull()) {
                continue;
            }

            TeamInfoWrapper teamInfo = player.GetTeam();
            
            if(teamInfo.IsNull()) { 
                continue;
            }

            int playerTeam = teamInfo.GetTeamIndex();
            std::string playerName = player.GetPlayerName().ToString();

            CarWrapper playerCar = player.GetCar();
               if (playerCar.IsNull())
               continue;
        
            // Log that a game server was found
            this->log("Cars found..\n");

            FlipCarComponentWrapper flipComponent = playerCar.GetFlipComponent();
               if (flipComponent.IsNull())
               continue;

            std::string flipReset = std::to_string(flipComponent.CanActivate());

            if (playerTeam == 0 && team0PlayerCount < 2) {
                if (team0PlayerCount == 0) {
                    team0PlayerName1 = playerName;
                    team0Player1FlipReset = flipReset;
                } else {
                    team0PlayerName2 = playerName;
                    team0Player2FlipReset = flipReset;
                }
                team0PlayerCount++;
                } else if (playerTeam == 1 && team1PlayerCount < 2) {
                if (team1PlayerCount == 0) {
                    team1PlayerName1 = playerName;
                    team1Player1FlipReset = flipReset;
                } else {
                    team1PlayerName2 = playerName;
                    team1Player2FlipReset = flipReset;
                }
                team1PlayerCount++;
            }

        }

        // Print output to console
        this->log("Game ID: " + gameID + "\n");
        this->log("Elapsed Time: " + elapsedTimeString + "\n");
        this->log("Team: " + team0Name + " Score: " + team0Score + " | Team: " + team1Name + " Score: "  + team1Score + "\n");
        this->log("Team1|Player1: " + team0PlayerName1 + " | FlipReset:" + team0Player1FlipReset + "\n" );
        this->log("Team1|Player2: " + team0PlayerName2 + " | FlipReset:" + team0Player2FlipReset + "\n" ); 
        this->log("Team2|Player1: " + team1PlayerName1 + " | FlipReset:" + team1Player1FlipReset + "\n" );
        this->log("Team2|Player2: " + team1PlayerName2 + " | FlipReset:" + team1Player2FlipReset + "\n" );
        
        // Upload JSON to DynamoDB (implement function to handle upload)
        uploadToDynamoDB(gameID, elapsedTimeString, 
                        team0Name, team1Name,
                        team0Score, team1Score, 
                        team0PlayerName1, team0PlayerName2, 
                        team1PlayerName1, team1PlayerName2,
                        team0Player1FlipReset, team0Player2FlipReset, 
                        team1Player1FlipReset, team1Player2FlipReset);

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
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.StartEvent");
    gameWrapper->UnhookEvent("Function ProjectX.GRI_X.EventGameStarted");
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.StartCountDown");
    gameWrapper->UnhookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame");
    gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.EndState");
	this->log("Dashboard plugin unloaded..");
}
