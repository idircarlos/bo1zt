#include "logic/server.h"
#include "logger.h"
#include "utils/common.h"
#include "controller/controller_internal.h"
#include "engine.h"

#include <stdio.h>
#include <stdlib.h>

#define SV_CMD_FORMAT_LENGTH 256

#define SV_CMD_MSG_FORMAT "%c \"^0[^1bo1zt^0]^7 %s\""
#define SV_CMD_MSG_CENTER_KEY 'c'
#define SV_CMD_MSG_CHAT_KEY 'h'
#define SV_CMD_MSG_KILLFEED_KEY 'e'

#define SV_CMD_BROADCAST 0xFFFFFFFF
#define SV_CMD_RELIABLE 0
#define SV_CMD_UNRELIABLE 1

struct Server {
    Controller *controller;
    Engine *engine;
};

static char *serverBuildMessage(const char key, const char *message) {
    char *serverCommand = (char*)malloc(SV_CMD_FORMAT_LENGTH);
    snprintf(serverCommand, SV_CMD_FORMAT_LENGTH, SV_CMD_MSG_FORMAT, key, message);
    return serverCommand;
}

Server *serverCreate(Controller *controller) {
    if (!controller) return NULL;
    Server *server = (Server*)malloc(sizeof(Server));
    if (!server) return NULL;
    server->controller = controller;
    server->engine = _controllerGetEngine(controller);
    return server;
}

void serverDestroy(Server *server) {
    if (!server) return;
    free(server);
}

bool serverExecuteCommand(Server *server, const char *command) {
    if (!server) return false;
    bool result = engineCBuffAddText(server->engine, command);
    return result;
}

bool serverSendServerCommand(Server *server, const char *command) {
    if (!server) return false;
    LOG_DEBUG("Sending Server Command: %s", command);
    bool result = engineSVSendServerCommand(server->engine, SV_CMD_RELIABLE, SV_CMD_BROADCAST, command);
    return result;
}

bool serverCenterMessage(Server *server, const char *message) {
    if (!server) return false;
    char *command = serverBuildMessage(SV_CMD_MSG_CENTER_KEY, message);
    bool result = serverSendServerCommand(server, command);
    free(command);
    return result;
}

bool serverChatMessage(Server *server, const char *message) {
    if (!server) return false;
    char *command = serverBuildMessage(SV_CMD_MSG_CHAT_KEY, message);
    bool result = serverSendServerCommand(server, command);
    free(command);
    return result;
}

bool serverKillfeedMessage(Server *server, const char *message) {
    if (!server) return false;
    char *command = serverBuildMessage(SV_CMD_MSG_KILLFEED_KEY, message);
    bool result = serverSendServerCommand(server, command);
    free(command);
    return result;
}

char *serverBuildDVar(const char *dVar, void *value, CType type) {
    char *serverCommand = (char*)malloc(SV_CMD_FORMAT_LENGTH);
    switch (type) {
        case CT_INT:
            snprintf(serverCommand, SV_CMD_FORMAT_LENGTH, "v %s \"%d\"", dVar, *(int*)value);
            break;
        case CT_FLOAT:
            snprintf(serverCommand, SV_CMD_FORMAT_LENGTH, "v %s \"%f\"", dVar, *(float*)value);
            break;
        case CT_STRING:
            snprintf(serverCommand, SV_CMD_FORMAT_LENGTH, "v %s \"%s\"", dVar, (char*)value);
            break;
        case CT_BOOL:
            snprintf(serverCommand, SV_CMD_FORMAT_LENGTH, "v %s \"%d\"", dVar, (*(bool*)value) ? 1 : 0);
            break;
        default:
            LOG_ERROR("Unknown CType %d", type);
            free(serverCommand);
            return NULL;
    }
    return serverCommand;
}

bool serverSetDVarBool(Server *server, const char *dVar, bool value) {
    if (!server) return false;
    char *message = serverBuildDVar(dVar, &value, CT_BOOL);
    bool result = serverSendServerCommand(server, message);
    free(message);
    return result;
}

bool serverSetDVarInt(Server *server, const char *dVar, int value) {
    if (!server) return false;
    char *message = serverBuildDVar(dVar, &value, CT_INT);
    bool result = serverSendServerCommand(server, message);
    free(message);
    return result;
}

bool serverSetDVarFloat(Server *server, const char *dVar, float value) {
    if (!server) return false;
    char *message = serverBuildDVar(dVar, &value, CT_FLOAT);
    bool result = serverSendServerCommand(server, message);
    free(message);
    return result;
}

bool serverSetDVarString(Server *server, const char *dVar, const char* value) {
    if (!server) return false;
    char *message = serverBuildDVar(dVar, (void*)value, CT_STRING);
    bool result = serverSendServerCommand(server, message);
    free(message);
    return result;
}

uintptr_t getDVarPointer(Server *server, const char *dVar) {
    if (!server) return 0;
    uintptr_t dVarPointer = engineGetDVarPointer(server->engine, dVar);
    if (!dVarPointer) {
        LOG_ERROR("DVar %s not found", dVar);
        return 0;
    }
    return dVarPointer;
}

bool serverGetDVarBool(Server *server, const char *dVar) {
    if (!server) return false;
    uintptr_t dVarPointer = getDVarPointer(server, dVar);
    Process *process = controllerGetProcess(server->controller);
    uint32_t value;
    bool success = processRead(process, dVarPointer + 0x18, &value, sizeof(uint32_t));
    if (!success) {
        LOG_ERROR("Failed to read DVar %s value", dVar);
        return false;
    }
    return value != 0;    
}


int serverGetDVarInt(Server *server, const char *dVar) {
    if (!server) return 0;
    uintptr_t dVarPointer = getDVarPointer(server, dVar);
    Process *process = controllerGetProcess(server->controller);
    int32_t value;
    bool success = processRead(process, dVarPointer + 0x18, &value, sizeof(int32_t));
    if (!success) {
        LOG_ERROR("Failed to read DVar %s value", dVar);
        return 0;
    }
    return value;    
}
float serverGetDVarFloat(Server *server, const char *dVar) {
    if (!server) return 0.0f;
    uintptr_t dVarPointer = getDVarPointer(server, dVar);
    Process *process = controllerGetProcess(server->controller);
    float value;
    bool success = processRead(process, dVarPointer + 0x18, &value, sizeof(float));
    if (!success) {
        LOG_ERROR("Failed to read DVar %s value", dVar);
        return 0.0f;
    }
    return value;    
}
char* serverGetDVarString(Server *server, const char *dVar) {
    if (!server) return NULL;
    uintptr_t dVarPointer = getDVarPointer(server, dVar);
    Process *process = controllerGetProcess(server->controller);
    char *result = (char*)malloc(128);
    bool success = processRead(process, dVarPointer + 0x18, result, 128);
    if (!success) {
        LOG_ERROR("Failed to read DVar %s value", dVar);
        free(result);
        return NULL;
    }
    return result;    
}
