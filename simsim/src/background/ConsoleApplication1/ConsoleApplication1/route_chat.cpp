#include "route.h"
#include "util.h"

struct state {
	SOCKET s;
	int found;
};

ROUTE("PUT", "/remember", _remember);
int _remember(SOCKET cli_socket, char * params, int length)
{
	char * pos;
	char * buffer = (char *)alloca(1024);

	pos = (char *)malloc(1024);

	memset(pos, 0, 1024);
	strncpy_s(pos, 1024, params, strlen(params));

	if(parse_query(pos, '&') < 3) return error400(cli_socket);

	if(strlen(struc_params[0].value) >= 256)
		struc_params[0].value[256] = 0;
	if (strlen(struc_params[1].value) >= 256)
		struc_params[1].value[256] = 0;
	
	if (!remember_valid())
	{
		return error400(cli_socket);
	}

	memset(buffer, 0, 1024);
	snprintf(buffer, 1024, "insert into chat_table values (%d, '%s', '%s');", atoi(struc_params[2].value), struc_params[0].value, struc_params[1].value);
	printf("QUERY : %s\n", buffer);
	int rc;
	char * err_msg;
	rc = sqlite3_exec(db, buffer, 0, 0, &err_msg);
	if (rc != SQLITE_OK)
	{
		printf("Error : %s", err_msg);
		return error400(cli_socket);
	}

	char headers[] = "Content-Length: 2\r\n\r\nok";
	send(cli_socket, http_200, strlen(http_200), 0);
	send(cli_socket, headers, strlen(headers), 0);
	return 0;
}



static int callback(void *arg, int argc, char **argv, char **colName)
{
	struct state *state = (struct state *)arg;
	SOCKET cli_socket = state->s;

	char headers[100];

	if (argc != 1)
		return SQLITE_ABORT;

	state->found = true;

	printf("%s = %s\n", colName[0], argv[0]);

	send(cli_socket, http_200, strlen(http_200), 0);
	sprintf_s(headers, "Content-Length: %d\r\n\r\n", strlen(argv[0]));
	send(cli_socket, headers, strlen(headers), 0);
	send(cli_socket, argv[0], strlen(argv[0]), 0);


	return SQLITE_DONE;
}

ROUTE("GET", "/search", _search);
int _search(SOCKET cli_socket, char * params, int length)
{
	char * buffer = (char *)malloc(1024);
	char * buffer2 = (char *)malloc(1024);

	memset(buffer, 0, sizeof(buffer));
	memset(buffer2, 0, sizeof(buffer2));

	strncpy_s(buffer, 1024, params, length);

	if(parse_query(buffer, '&') < 2) return error400(cli_socket);
	if (strlen(struc_params[0].value) >= 256) struc_params[0].value[256] = 0;
	if (strlen(struc_params[1].value) >= 256) struc_params[1].value[256] = 0;
	if (!search_valid())
	{
		return error400(cli_socket);
	}
	snprintf(buffer2, 1024, "select output from chat_table where usernum=%d AND input like '%%%s%%' limit 1;", atoi(struc_params[1].value), struc_params[0].value);
	printf("QUERY : %s\n", buffer2);
	int rc;
	char * err_msg;
	struct state state = { cli_socket, 0 };
	rc = sqlite3_exec(db, buffer2, callback, &state, &err_msg);
	if (rc != SQLITE_OK)
	{
		printf("Error : %s", err_msg);
		send(cli_socket, http_400, strlen(http_400), 0);
		return -1;
	}
	if (!state.found)
	{
		char headers[] = "Content-Length: 5\r\n\r\n";
		send(cli_socket, http_200, strlen(http_200), 0);
		send(cli_socket, headers, strlen(headers), 0);
		send(cli_socket, "Nope\n", 5, 0);
	}

	return 0;

}


