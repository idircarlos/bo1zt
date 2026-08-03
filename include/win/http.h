#ifndef WIN_HTTP_H_
#define WIN_HTTP_H_

typedef struct {
    const char *method;
    const char *path;
    const char *query;
    const char *body;
} HttpRequest;

typedef struct {
    int status;
    char *body;
} HttpClientResponse;

typedef struct HttpResponse HttpResponse;
typedef void (*HttpHandler)(const HttpRequest *request, HttpResponse *response, void *userData);

void httpResponseSet(HttpResponse *response, int status, const char *contentType, const char *body);
void httpResponseJson(HttpResponse *response, int status, const char *jsonBody);
void httpResponseStatus(HttpResponse *response, int status);
int httpServe(int port, HttpHandler handler, void *userData);
HttpClientResponse httpClientRequest(const char *host, int port, const char *method, const char *path, const char *headers, const char *body);
HttpClientResponse httpsClientRequest(const char *host, int port, const char *method, const char *path, const char *headers, const char *body);
void httpClientResponseFree(HttpClientResponse *response);

#endif // WIN_HTTP_H_
