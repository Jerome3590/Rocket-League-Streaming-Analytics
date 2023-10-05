#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h> 
#include <memory> // For std::shared_ptr// If used in the method declarations
#include <string>
#include <map>
#include <chrono>
#include <ctime>

// Forward declarations
class ServerWrapper;
class ActorWrapper;
namespace Aws {
    namespace DynamoDB {
        class DynamoDBClient;
    }
}

class Dashboard: public BakkesMod::Plugin::BakkesModPlugin
{
public:
    Dashboard(); 
    virtual void onLoad();
    virtual void onUnload();
    bool isGamePaused();
    bool isGamePlaying();
    std::string getCurrentTime();
    bool saveGameID(const std::string& gameID);
    bool isNewGameFlag = false;
    bool gamePaused = false;
    void getCarLocationAndRotation(CarWrapper car);
    Vector Rotate(Vector aVec, double roll, double yaw, double pitch);
    void getGameData();
    static void logAndCall(std::function<void()> func, const std::string& message); 
    ~Dashboard(); 
private:
    Aws::SDKOptions options;
    std::shared_ptr<Aws::DynamoDB::DynamoDBClient> dynamoClient;
    void log(std::string msg);
    void AWSOps();
    std::map<std::string, std::map<std::string, double>> dataMap;
	std::pair<std::string, double> tableCalcs( 
        const std::string& timeRemainingString, 
		const std::string& team0Score, const std::string& team1Score);
    void uploadToDynamoDB(const std::string& gameID, const std::string& timeRemainingString, 
        const std::string& team0Name, const std::string& team1Name,
        const std::string& team0Score, const std::string& team1Score,
		const std::string& Predicted_Winner, const std::string& winProbString,
        const std::string& team0PlayerName1, const std::string& team0PlayerName2, 
        const std::string& team1PlayerName1, const std::string& team1PlayerName2, 
        double team0Player1CarLocationX, double team0Player1CarLocationY, double team0Player1CarLocationZ,
        double team0Player2CarLocationX, double team0Player2CarLocationY, double team0Player2CarLocationZ,
        double team1Player1CarLocationX, double team1Player1CarLocationY, double team1Player1CarLocationZ,
        double team1Player2CarLocationX, double team1Player2CarLocationY, double team1Player2CarLocationZ,
        double team0Player1CarRotationX, double team0Player1CarRotationY, double team0Player1CarRotationZ,  
        double team0Player2CarRotationX, double team0Player2CarRotationY, double team0Player2CarRotationZ,
        double team1Player1CarRotationX, double team1Player1CarRotationY, double team1Player1CarRotationZ,
        double team1Player2CarRotationX, double team1Player2CarRotationY, double team1Player2CarRotationZ,
        const std::string& team0Player1FlipReset, const std::string& team0Player2FlipReset,
        const std::string& team1Player1FlipReset, const std::string& team1Player2FlipReset);
    void loadHooks();
    float elapsedIntervals = 0.0f;

};

