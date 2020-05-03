#include "Network/Server.h"
#include "Network/Client.h"
#include <thread>
#include <atomic>

std::atomic<bool> isRunning = true;

void ClientListen(Client* p_client)
{
    unsigned char receivedData[255];
    while(isRunning)
    {

        int ret = Internal_ClientListen(p_client, receivedData, 255);
        if (ret > 0)
        {
            std::cout << "received msg: " << receivedData << "\n";
        }
    }
}
void ServListen(Server* p_server)
{
    unsigned char receivedData[255];
    while(isRunning)
    {
        int ret = Internal_ServerListen(p_server, receivedData, 0);
        if(ret > 0)
            std::cout << "received msg: " << receivedData << "\n";
    }
}

int main()
{
    Server* server = Internal_ServerCreate();
    Client* client = Internal_ClientCreate();
    std::thread servListen(ServListen, std::ref(server));
    std::thread clientListen(ClientListen, std::ref(client));

    Internal_ClientConnect(client);

    char test[5] = "toto";
    std::cin.get();
    Internal_ServerPropagateGameData(server, reinterpret_cast<unsigned char*>(test), 5);


    std::cin.get();
    isRunning = false;
    clientListen.join();
    servListen.join();

    Internal_ClientDestroy(client);
    Internal_ServerDestroy(server);
}