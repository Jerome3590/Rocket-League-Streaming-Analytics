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
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <sstream>
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>

BAKKESMOD_PLUGIN(Dashboard, "Rocket League Game Dashboard", "1.0", PLUGINTYPE_FREEPLAY)


Dashboard::Dashboard() {
    // Initialize AWS SDK
    Aws::SDKOptions options;
    Aws::InitAPI(options);
}


void Dashboard::log(std::string msg) {
    if (!cvarManager) return;
	cvarManager->log(msg);
}


void Dashboard::loadHooks() {

    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartEvent",
          std::bind(&Dashboard::getGameData, this));
     gameWrapper->HookEvent("Function ProjectX.GRI_X.EventGameStarted",
         std::bind(&Dashboard::getGameData, this));
  //  gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartCountDown",
  //    std::bind(&Dashboard::getGameData, this));
  //  gameWrapper->HookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame",
   //      std::bind(&Dashboard::getGameData, this));
    gameWrapper->HookEvent("Function GameEvent_Soccar_TA.Active.EndState",
         std::bind(&Dashboard::isGamePaused, this));
    gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartEvent",
          std::bind(&Dashboard::isGamePlaying, this));
   //  gameWrapper->HookEvent("Function ProjectX.GRI_X.EventGameStarted",
   //      std::bind(&Dashboard::isGamePlaying, this));
   // gameWrapper->HookEvent("Function TAGame.GameEvent_TA.StartCountDown",
   //   std::bind(&Dashboard::isGamePlaying, this));
   // gameWrapper->HookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame",
   //      std::bind(&Dashboard::isGamePlaying, this));
}


void Dashboard::onLoad() {
	this->log("Dashboard plugin started..");
    elapsedIntervals = 0;
	AWSOps();
	this->loadHooks();
	
}


bool Dashboard::isGamePaused() {
    this->log("Game play paused..");
    gamePaused = false; 
    return gamePaused;
}


bool Dashboard::isGamePlaying() {
    this->log("Game play resumed..");
    gamePaused = false; 
    return !gamePaused;
}


bool Dashboard::saveGameID(const std::string& gameID) {

    Aws::S3::S3Client s3_client;
    std::string bucketName = "rocket-league-lookup";
    std::string objectKey = gameID;  // Assuming Game_ID itself can be used as the object key

    Aws::S3::Model::PutObjectRequest put_object_request;
    put_object_request.WithBucket(bucketName).WithKey(objectKey);

    // Set up the request body, assuming you want to upload the gameID string
    std::shared_ptr<Aws::IOStream> input_data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
    *input_data << gameID;
    put_object_request.SetBody(input_data);

    auto put_object_outcome = s3_client.PutObject(put_object_request);

    if (put_object_outcome.IsSuccess()) {
        this->log("Successfully uploaded Game_ID " + gameID);
        return true;
    } else {
        this->log("Failed to upload Game_ID " + gameID + ": " + put_object_outcome.GetError().GetMessage());
        return false;
    }
    
}


void Dashboard::AWSOps() {
    // DynamoDB Operations
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

    // S3 Operations
    {   
        using namespace Aws;
        S3::S3Client client;
        auto outcome = client.ListBuckets();

        if (outcome.IsSuccess()) {
            this->log("Your S3 buckets:");

            Aws::Vector<Aws::S3::Model::Bucket> bucket_list = outcome.GetResult().GetBuckets();

            for (auto const& bucket : bucket_list) {
                this->log(bucket.GetName().c_str());
            }
        }
        else {
            this->log("ListBuckets error: " + outcome.GetError().GetExceptionName() + " - " + outcome.GetError().GetMessage());
        }
    }

}


void Dashboard::uploadToDynamoDB(
const std::string& gameID, const std::string& timeRemainingString, 
const std::string& team0Name, const std::string& team1Name, 
const std::string& team0Score, const std::string& team1Score, 
const std::string& team0PlayerName1, const std::string& team0PlayerName2, 
const std::string& team1PlayerName1, const std::string& team1PlayerName2,
const std::string& team0Player1FlipReset, const std::string& team0Player2FlipReset, 
const std::string& team1Player1FlipReset, const std::string& team1Player2FlipReset) {
    using namespace Aws::DynamoDB::Model;

    PutItemRequest putItemRequest;

    // Set table name
    putItemRequest.SetTableName("rocket_league_events");

    // Create an item with string and number attributes
    Aws::Map<Aws::String, AttributeValue> item;
    item["Game_ID"] = AttributeValue(gameID);
    item["Time_Remaining"] = AttributeValue(timeRemainingString); 
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
        // Retrieve Match GUID, game count down time, team names, and scores
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
        if (!isNewGameFlag) {
            saveGameID(gameID);
            isNewGameFlag = true; // Set the flag to true
        }

        const int timeRemaining = server.GetbOverTime() ? -server.GetSecondsRemaining() : server.GetSecondsRemaining();
        std::string timeRemainingString = std::to_string(timeRemaining); // Convert Time to string

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
        this->log("Time Remaining: " + timeRemainingString + "\n"); 
        this->log("Team: " + team0Name + " Score: " + team0Score + " | Team: " + team1Name + " Score: "  + team1Score + "\n");
        this->log("Team1|Player1: " + team0PlayerName1 + " | FlipReset:" + team0Player1FlipReset + "\n" );
        this->log("Team1|Player2: " + team0PlayerName2 + " | FlipReset:" + team0Player2FlipReset + "\n" ); 
        this->log("Team2|Player1: " + team1PlayerName1 + " | FlipReset:" + team1Player1FlipReset + "\n" );
        this->log("Team2|Player2: " + team1PlayerName2 + " | FlipReset:" + team1Player2FlipReset + "\n" );
        
        // Upload JSON to DynamoDB (implement function to handle upload)
        uploadToDynamoDB(gameID, timeRemainingString, 
                        team0Name, team1Name,
                        team0Score, team1Score, 
                        team0PlayerName1, team0PlayerName2, 
                        team1PlayerName1, team1PlayerName2,
                        team0Player1FlipReset, team0Player2FlipReset, 
                        team1Player1FlipReset, team1Player2FlipReset);

        // Increment the elapsed intervals
        elapsedIntervals++;

        if (elapsedIntervals < 700) {
            // Schedule this method to run again after 0.75 seconds
            gameWrapper->SetTimeout([this](GameWrapper* gw) {
                getGameData();
            }, 0.75f);
        }
        
    } // End of if(isInOnlineGame)
} // End of function


void Dashboard::onUnload() {
    gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.StartEvent");
    gameWrapper->UnhookEvent("Function ProjectX.GRI_X.EventGameStarted");
    //gameWrapper->UnhookEvent("Function TAGame.GameEvent_TA.StartCountDown");
    //gameWrapper->UnhookEvent("Function ProjectX.OnlinePlayer_X.OnNewGame");
    gameWrapper->UnhookEvent("Function GameEvent_Soccar_TA.Active.EndState");
	this->log("Dashboard plugin unloaded..");
}


Dashboard::~Dashboard() {
    // Shut down the AWS SDK
    Aws::SDKOptions options;
    Aws::ShutdownAPI(options);
}

