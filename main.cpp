#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

/* Description of select()
 * FD_CLA -> Remove from Set
 * FD_SET -> Add to Set
 * FD_ZERO -> Clean Set
 * FD_SET -> Array of Listener and Client (SET) [L][C][C][C] ...
 * No need for multithreading
 */

using namespace std;

int main() {

    // Initialize socket
    WSADATA wsData;
    WORD var = MAKEWORD(2,2);

    if (WSAStartup (var, &wsData) != 0)
    {
        MessageBox (NULL, TEXT("WSAStartup failed!"), TEXT("Error"), MB_OK);
        return FALSE;
    }

    // Create Socket
    SOCKET listening = socket(AF_INET,SOCK_STREAM,0);

    if(listening == INVALID_SOCKET){
        MessageBox (NULL, TEXT("Creation of Socket failed!"), TEXT("Error"), MB_OK);
        return FALSE;
    }

    // Bind the ip address and port to a socket
    sockaddr_in hint;
    hint.sin_family = AF_INET;
    hint.sin_port = htons(69000);
    hint.sin_addr.S_un.S_addr = INADDR_ANY;

    bind(listening,(sockaddr*)&hint,sizeof(hint));

    // Tell Winsock the socket is for listening
    listen(listening,SOMAXCONN);

    // Create set of socket
    fd_set master;
    FD_ZERO(&master);

    // add listening to set
    FD_SET(listening,&master);

    bool running{true};

    while(running){
       // do copy of master because function select destroy it.
       fd_set copy = master;

       int socketCount = select(0,&copy, nullptr, nullptr, nullptr);

        for (int i = 0; i < socketCount; ++i) {
            SOCKET socket = copy.fd_array[i];
            if(socket == listening){
               // Accept a new connection
               SOCKET client = accept(listening, nullptr, nullptr);
               // Add the new connection  to the list of connected clients
               FD_SET(client,&master);
               // Send a welcome message to the connected client
               string welcomeMsg = "Welcome to the Chat Server!\r\n";
               send(client,welcomeMsg.c_str(), welcomeMsg.size() + 1,0);
            }else{
                char buf[4096];
                ZeroMemory(buf, 4096);

                // Received message
                int bytesIn = recv(socket,buf, 4096,0);

                if(bytesIn <= 0){
                     // Drop the client
                    closesocket(socket);
                    FD_CLR(socket,&master);
                }else{
                    // Check to see if it's a command \quit kills the server
                    // Check to see if it's a command. \quit kills the server
                    if (buf[0] == '-')
                    {
                        // Is the command quit?
                        string cmd = string(buf, bytesIn);

                        cmd = cmd.substr(0,5);

                        if (cmd == "-quit")
                        {
                            running = false;
                            break;
                        }

                        // Unknown command
                        continue;
                    }

                    // Send message to other clients, and definiately NOT the listening socket

                    for (int i = 0; i < master.fd_count; i++)
                    {
                        SOCKET outSock = master.fd_array[i];
                        if (outSock != listening && outSock != socket)
                        {
                            ostringstream ss;
                            ss << "SOCKET #" << socket << ": " << buf << "\r\n";
                            string strOut = ss.str();

                            send(outSock, strOut.c_str(), strOut.size() + 1, 0);
                        }
                    }

                }

                // Accept new message
                // Send message to other clients, and NOT the listening socket
            }
        }
    }

    // Remove the listening socket from the master file descriptor set and close it
    // to prevent anyone else trying to connect.
    FD_CLR(listening,&master);
    closesocket(listening);

    // Message to let users known what's happening
    string msg = "Server is shutting down. Goodbye\r\n";

    while (master.fd_count > 0){
        // Get the socket number
        SOCKET sock = master.fd_array[0];

        // Send the goodbye message
        send(sock, msg.c_str(), msg.size() + 1, 0);

        FD_CLR(sock,&master);
        closesocket(sock);
    }


    WSACleanup();

    return 0;
}
