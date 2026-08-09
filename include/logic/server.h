#ifndef SERVER_H_
#define SERVER_H_

#include "controller.h"

#define SERVER_BO1ZT_MSG_PREFIX "^0[^1bo1zt^0]^7 "
#define SERVER_CHAT_COLOR_FORMAT "^%c"

typedef enum {
    CHAT_COLOR_BLACK = '0',
    CHAT_COLOR_RED = '1',
    CHAT_COLOR_GREEN = '2',
    CHAT_COLOR_YELLOW = '3',
    CHAT_COLOR_DARK_BLUE = '4',
    CHAT_COLOR_CYAN = '5',
    CHAT_COLOR_PINK = '6',
    CHAT_COLOR_WHITE = '7',
    CHAT_COLOR_GRAY = '8',
    CHAT_COLOR_BROWN = '9',
    CHAT_COLOR_DARK_RED = ':',
    CHAT_COLOR_DARK_GREEN = ';',
    CHAT_COLOR_ORANGE = '<',
    CHAT_COLOR_BLUE = '=',
    CHAT_COLOR_LIGHT_GRAY = '>',
    CHAT_COLOR_PURPLE = '?',
    CHAT_COLOR_DARK_ORANGE = '@',
} ChatColor;

typedef struct Server Server;

Server *serverCreate(Controller *controller);
void serverDestroy(Server *server);

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
