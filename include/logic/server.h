#ifndef SERVER_H_
#define SERVER_H_

#include "controller/controller.h"

typedef struct Server Server;

Server *serverCreate(Controller *controller);

bool serverExecuteCommand(Server *server, const char *command);
bool serverSendServerCommand(Server *server, const char *command);

bool serverCenterMessage(Server *server, const char *message);
bool serverChatMessage(Server *server, const char *message);
bool serverKillfeedMessage(Server *server, const char *message);

bool serverGetDVarBool(Server *server, const char *dVar);
int serverGetDVarInt(Server *server, const char *dVar);
float serverGetDVarFloat(Server *server, const char *dVar);
char* serverGetDVarString(Server *server, const char *dVar);

bool serverSetDVarBool(Server *server, const char *dVar, bool value);
bool serverSetDVarInt(Server *server, const char *dVar, int value);
bool serverSetDVarFloat(Server *server, const char *dVar, float value);
bool serverSetDVarString(Server *server, const char *dVar, const char* value);

#endif // SERVER_H_
