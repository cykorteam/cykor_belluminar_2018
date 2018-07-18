#include "route.h"
#include "sha.h"
#include "util.h"

struct state {
	SOCKET s;
	int found;
};

static int callback(void *arg, int argc, char **argv, char **colName)
{
	struct state *state = (struct state *)arg;
	SOCKET cli_socket = state->s;

	char headers[100];

	if (state->found) return SQLITE_OK;

	if (argc != 1)
		return SQLITE_ABORT;

	state->found = true;

	printf("%s = %s\n", colName[0], argv[0]);

	char buf[100];

	sprintf_s(buf, "%d", atoi(argv[0]));

	send(cli_socket, http_200, strlen(http_200), 0);
	sprintf_s(headers, "Content-Type: text/plain\r\nContent-Length: %d\r\n\r\n", strlen(buf));
	send(cli_socket, headers, strlen(headers), 0);
	send(cli_socket, buf, strlen(buf), 0);

	return SQLITE_OK;
}

ROUTE("GET", "/login", _login);
int _login(SOCKET cli_socket, char * params, int length)
{

	SHA256_CTX ctx;
	char hash_result[33] = {};
	char * pos;
	char * enc;
	char * id;
	char * pw;
	char * end;
	int is_id = 0, is_pw = 0, is_enc = 0;
	int id_len, pw_len;
	char * buffer2 = (char *)malloc(1024);

	puts("Login Page!");
	pos = (char *)alloca(1025);
	strncpy_s(pos, 1024, params, length);

	if(parse_query(pos, '&') < 2) return error400(cli_socket);
	if (strlen(struc_params[0].value) > 64)
		struc_params[0].value[64] = '\x00';
	if (strlen(struc_params[1].value) > 64)
		struc_params[1].value[64] = '\x00';

	if (!id_pw_valid()) return error400(cli_socket);

	snprintf(buffer2, 1024, "select num from user_table where id='%s' and pw='%s' limit 1;", struc_params[0].value, struc_params[1].value);
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
		char headers[] = "Content-Type: text/plain\r\nContent-Length: 5\r\n\r\n";
		send(cli_socket, http_200, strlen(http_200), 0);
		send(cli_socket, headers, strlen(headers), 0);
		send(cli_socket, "wrong", 5, 0);
	}

	return 0;
}

ROUTE("PUT", "/register", _register);
int _register(SOCKET cli_socket, char * params, int length)
{

	char * pos;
	char *buffer = (char *)alloca(1024);
	int i;

	puts("Register Page!");

	pos = (char *)malloc(1025);
	memset(pos, '\x00', 1025);
	strncpy_s(pos, 1024, params, length);
	if(parse_query(pos, '&') < 2) return error400(cli_socket);
	if (strlen(struc_params[0].value) > 64)
		struc_params[0].value[64] = '\x00';
	if (strlen(struc_params[1].value) > 64)
		struc_params[1].value[64] = '\x00';

	/*for (i = 0; i < 64; i++)
	{
		printf("%d ", struc_params[0].value[i]);
	}
	printf("\n");*/

	if (!id_pw_valid())
	{
		puts("IDPWVALID");
		send(cli_socket, http_400, strlen(http_400), 0);
		return -1;
	}

	memset(buffer, 0, 1024);
	snprintf(buffer, 1024, "insert into user_table VALUES(NULL, '%s','%s');",
		struc_params[0].value,
		struc_params[1].value);

	printf("QUERY : %s\n", buffer);

	int rc;
	char * err_msg;
	rc = sqlite3_exec(db, buffer, 0, 0, &err_msg);
	if (rc != SQLITE_OK)
	{
		printf("Error : %s", err_msg);
		send(cli_socket, http_400, strlen(http_400), 0);
		return -1;
	}

	char headers[] = "Content-Length: 2\r\n\r\nok";

	send(cli_socket, http_200, strlen(http_200), 0);
	send(cli_socket, headers, strlen(headers), 0);

	return 0;
}
