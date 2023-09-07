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
    virtual void onLoad();
    virtual void onUnload();
    static void logAndCall(std::function<void()> func, const std::string& message); 
private:
    Aws::SDKOptions options;
    std::shared_ptr<Aws::DynamoDB::DynamoDBClient> dynamoClient;
    void log(std::string msg);
    void dynamoDbOps();
    void uploadToDynamoDB(const std::string& gameID, const std::string& elapsedTime, nlohmann::json& gameJSON);
    void loadHooks();
    void getGameData();
    float elapsedIntervals = 0.0f;

};

