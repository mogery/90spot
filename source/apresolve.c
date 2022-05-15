#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "version.h"
#include "apresolve.h"
#include "cJSON.h"
#include "log.h"

struct in_addr * lookup_host(const char* host)
{
    struct addrinfo hints, *res;
    int errcode;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // lock to ipv4. sucks
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags |= AI_CANONNAME;

    if ((errcode = getaddrinfo(host, NULL, &hints, &res)) != 0)
    {
        log("[APRESOLVE] ERR lookup_host: getaddrinfo failed with error %d\n", errcode);
        return NULL;
    }

    if (res == NULL)
    {
        return NULL;
    }

    struct in_addr *addr = malloc(sizeof(struct in_addr));
    memcpy(addr, &((struct sockaddr_in *) res->ai_addr)->sin_addr, sizeof(struct in_addr));

    freeaddrinfo(res);

    return addr;
}

struct sockaddr_in* apresolve()
{
    cJSON* json = NULL;
    cJSON *ap_list, *ap_first;
    struct sockaddr_in server;
    struct in_addr *ap_addr = NULL;
    struct sockaddr_in* ap_server = NULL;
    int socket_desc = -1;

    struct in_addr *addr = lookup_host("apresolve.spotify.com");
    if (addr == NULL) {
        log("[APRESOLVE] Could not resolve apresolve.spotify.com");
        goto cleanup;
    }

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        log("[APRESOLVE] Could not create socket\n");
        goto cleanup;
    }

    server.sin_addr.s_addr = addr->s_addr;
    server.sin_family = AF_INET;
    server.sin_port = htons(80);

    if (connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        log("[APRESOLVE] Connection error\n");
        goto cleanup;
    }

    char* request_message = "GET / HTTP/1.1\r\nHost: apresolve.spotify.com\r\nConnection: close\r\nUser-Agent: " SPOTTIE_UA "\r\nAccept: application/json\r\n\r\n";

    if (send(socket_desc, request_message, strlen(request_message), 0) < 0)
    {
        log("[APRESOLVE] Failed to send HTTP request\n");
        goto cleanup;
    }

    char reply[4096];

    if (recv(socket_desc, reply, 4096, 0) < 0)
    {
        log("[APRESOLVE] Failed to receive HTTP response\n");
        goto cleanup;
    }

    char* body = strstr(reply, "\r\n\r\n");

    if (body == NULL) {
        log("[APRESOLVE] Malformed HTTP response\n");
        goto cleanup;
    }

    body = body + 4;

    json = cJSON_Parse(body);

    if (json == NULL) {
        log("[APRESOLVE] Malformed JSON\n%s\n", body);
        goto cleanup;
    }

    if (!cJSON_HasObjectItem(json, "ap_list")) {
        log("[APRESOLVE] JSON doesn't have ap_list key :(\n%s\n", body);
        goto cleanup;
    }

    ap_list = cJSON_GetObjectItem(json, "ap_list");

    if (cJSON_GetArraySize(ap_list) == 0) {
        log("[APRESOLVE] No APs available :(\n%s\n", body);
        goto cleanup;
    }

    ap_first = cJSON_GetArrayItem(ap_list, 0);

    if (!cJSON_IsString(ap_first)) {
        log("[APRESOLVE] First AP is not a string :(\n%s\n", body);
        goto cleanup;
    }

    char* ap = ap_first->valuestring;

    if (ap == NULL) {
        log("[APRESOLVE] First AP seems like a string, but has nullptr value :(\n%s\n", body);
        goto cleanup;
    }

    char* colon = strchr(ap, ':');

    if (colon == NULL)  {
        log("[APRESOLVE] First AP doesn't specify port :(\n%s\n", body);
        goto cleanup;
    }

    char* hostname = malloc(colon - ap + 1);
    memset(hostname, 0, colon - ap + 1);
    memcpy(hostname, ap, colon - ap);

    log("[APRESOLVE] Hostname: %s Port: %s ", hostname, colon + 1);

    ap_addr = lookup_host(hostname);
    if (ap_addr == NULL) {
        log("\n[APRESOLVE] Failed to resolve hostname :(\n");
        goto cleanup;
    }

    char ap_addr_str[100];
    inet_ntop(AF_INET, ap_addr, ap_addr_str, 100);
    log("IP: %s\n", ap_addr_str);

    ap_server = malloc(sizeof(struct sockaddr_in));
    ap_server->sin_family = AF_INET;
    ap_server->sin_port = htons(atoi(colon+1));
    ap_server->sin_addr = *ap_addr;

cleanup:
    if (json != NULL) {
        cJSON_Delete(json);        
    }

    if (socket_desc != -1)
    {
        close(socket_desc);
    }

    if (addr != NULL)
    {
        free(addr);
    }

    return ap_server;
}