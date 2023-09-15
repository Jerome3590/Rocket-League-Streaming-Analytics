#pragma once
#pragma comment(lib, "pluginsdk.lib")
#include "bakkesmod/wrappers/includes.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentialsProvider.h> 
#include <memory> // For std::shared_ptr// If used in the method declarations
#include <string>

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
    bool saveGameID(const std::string& gameID);
    bool isNewGameFlag = false;
    bool gamePaused = false;
    void getGameData();
    static void logAndCall(std::function<void()> func, const std::string& message); 
    ~Dashboard(); 
private:
    Aws::SDKOptions options;
    std::shared_ptr<Aws::DynamoDB::DynamoDBClient> dynamoClient;
    void log(std::string msg);
    void AWSOps();
    void uploadToDynamoDB(const std::string& gameID, const std::string& timeRemainingString, 
        const std::string& team0Name, const std::string& team1Name,
        const std::string& team0Score, const std::string& team1Score,
        const std::string& team0PlayerName1, const std::string& team0PlayerName2, 
        const std::string& team1PlayerName1, const std::string& team1PlayerName2, 
        const std::string& team0Player1FlipReset, const std::string& team0Player2FlipReset,
        const std::string& team1Player1FlipReset, const std::string& team1Player2FlipReset);
    void loadHooks();
    float elapsedIntervals = 0.0f;

};

