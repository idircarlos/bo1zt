#ifndef BIND_MANAGER_H_
#define BIND_MANAGER_H_

typedef struct BindManager BindManager;
typedef struct Controller Controller;

BindManager *bindManagerCreate(Controller *controller);
void bindManagerDestroy(BindManager *manager);
void bindManagerReload(BindManager *manager);
void bindManagerUpdate(BindManager *manager);

#endif // BIND_MANAGER_H_
