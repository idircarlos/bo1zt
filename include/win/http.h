#ifndef WIN_HTTP_H_
#define WIN_HTTP_H_

typedef struct {
    const char *method;
    const char *path;
    const char *query;
    const char *body;
} HttpRequest;

typedef struct HttpResponse HttpResponse;

void httpResponseSet(HttpResponse *response, int status, const char *contentType, const char *body);
void httpResponseJson(HttpResponse *response, int status, const char *jsonBody);
void httpResponseStatus(HttpResponse *response, int status);

typedef void (*HttpHandler)(const HttpRequest *request, HttpResponse *response, void *userData);

int httpServe(int port, HttpHandler handler, void *userData);

typedef struct {
    int status;
    char *body;
} HttpClientResponse;

HttpClientResponse httpClientRequest(int port, const char *method, const char *path, const char *body);
void httpClientResponseFree(HttpClientResponse *response);

#endif // WIN_HTTP_H_
