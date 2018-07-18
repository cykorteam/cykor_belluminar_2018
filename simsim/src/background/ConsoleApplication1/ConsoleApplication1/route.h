#pragma once
#include <stdio.h>
#include <stdlib.h>
#include<winsock2.h>
#include <stddef.h>
#include <memory.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <Wincrypt.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#if defined _MSC_VER
#include <direct.h>
#elif defined __GNUC__
#include <sys/types.h>
#include <sys/stat.h>
#endif


#include "sqlite3.h"
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"sqlite3.lib")

#include <vector>


#define BUFSIZE 1024
#define MAX_PARAM 3


class Route;
std::vector<Route *> *getRoutes();
typedef int(*routeHandler)(SOCKET, char *, int);

class Route {
private:
	const char *method_;
	const char *path_;
	routeHandler handler_;
public:
	Route(const char *method, const char *path, routeHandler handler) {
		this->method_ = method;
		this->path_ = path;
		this->handler_ = handler;

		auto *routes = getRoutes();
		for (auto it = routes->begin(); it != routes->end(); it++) {
			if (strlen((*it)->path()) <= strlen(path))
			{
				routes->insert(it, this);
				break;
			}
		}
		routes->push_back(this);
	}

	const char *method() { return method_; }
	const char *path() { return path_; }
	routeHandler handler() { return handler_; }
};


#define ROUTE(method, path, handler) int handler(SOCKET, char*, int); static Route route##handler(method, path, handler)

int _notFound(SOCKET cli_socket);

