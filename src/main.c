#include <stdio.h>
#include "gui/gui.h"
#include "memory/memory.h"
#include "controller/controller.h"
#include "logger/logger.h"
#include "thread/thread.h"
#include <windows.h>

static Controller *controller = NULL;

typedef struct {
    DWORD pid;
    HWND hwnd;
} EnumData;

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    EnumData *data = (EnumData*)lParam;
    DWORD windowPid = 0;
    GetWindowThreadProcessId(hWnd, &windowPid);

    // buscaremos la ventana principal: visible y sin owner
    if (windowPid == data->pid) {
        if (IsWindowVisible(hWnd) && GetWindow(hWnd, GW_OWNER) == NULL) {
            data->hwnd = hWnd;
            return FALSE; // hemos encontrado la ventana, detener enumeración
        }
    }
    return TRUE; // continuar
}

HWND GetMainWindowHandle(DWORD pid) {
    EnumData data;
    data.pid = pid;
    data.hwnd = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hwnd;
}

void MakeWindowBorderless(HWND hwnd) {
    if (!IsWindow(hwnd)) {
        fprintf(stderr, "MakeWindowBorderless: hwnd no válido\n");
        return;
    }

    // Obtener estilos (usar Ptr para compatibilidad x64)
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    exStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);

    // Forzar recálculo del marco
    SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
}


void testMakeBorderless() {
    DWORD pid = 13536;
    HWND hwnd = GetMainWindowHandle(pid);
    if (hwnd == NULL) {
        printf("No se encontró ventana principal para PID %u\n", pid);
        return;
    }

    printf("HWND encontrado: 0x%p\n", (void*)hwnd);

    // Intentar hacer borderless
    MakeWindowBorderless(hwnd);

    // Verificar errores
    DWORD err = GetLastError();
    if (err != 0) {
        printf("GetLastError despues de MakeWindowBorderless = %u\n", err);
    } else {
        printf("Operación completada. Revisa la ventana.\n");
    }

}

int processRunningThread(void *data) {
    (void)data;
    while (true) {
        while(!controllerIsGameRunning(controller)) {
            LOG_INFO("Waiting for game running...\n");
            threadSleep(3000);
        }
        if (!controllerIsGameAttached(controller)) {
            controllerAttachGame(controller);
        }
        LOG_INFO("Game attached! Waiting until game is closed...\n");
        testMakeBorderless();
        controllerWaitUntilGameCloses(controller);
        LOG_INFO("Game has been closed\n");
        controllerDetachGame(controller);
    }
    return 0;
}

int refreshWindowThread(void *data) {
    (void)data;
    while (true) {
        while(!controllerIsGameRunning(controller)) {
            threadSleep(3000);
        }
        guiUpdate();
        threadSleep(1000);
    }
}

int main(void) {
    loggerInit(NULL);
    controller = controllerCreate();
    threadCreate(processRunningThread, NULL);
    guiInit(controller);
    threadCreate(refreshWindowThread, NULL);
    guiRun();
    guiCleanup();
    
    return 0;
}