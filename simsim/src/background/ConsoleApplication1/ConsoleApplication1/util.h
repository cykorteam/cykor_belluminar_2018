#include "sqlite3.h"
#include "route.h"

static int urldecode(unsigned char *source, unsigned char *dest);

struct struc_param {
	char * key;
	char * value;
};

static const char * create_table = "create table chat_table (usernum INTEGER, input varchar(264), output varchat(264));"
"create table user_table (num INTEGER PRIMARY KEY, id varchar(256), pw varchar(256));";

extern sqlite3 *db;
extern struct struc_param struc_params[];

static const char *http_200 = "HTTP/1.1 200 OK\r\n"
"Connection: keep-alive\r\n";

static const char *http_404 = "HTTP/1.1 404 Not Found\r\n"
"Content-Type: text/plain\r\n"
"Connection: keep-alive\r\n";

static const char *http_400 = "HTTP/1.1 400 Bad Request\r\n"
"Content-Type: text/plain\r\n"
"Connection: keep-alive\r\n"
"Content-Length: 15\r\n\r\n400 Bad Request\r\n";


static const char * param[] = {
	"enc",
	"id",
	"pw",
	"msg"
};


#define ENC 0
#define ID 1
#define PW 2
#define MSG 3

int parse_query(char *query, char delimiter);
void genRandomDBfile(void);
extern char DBpath[];

int id_pw_valid(void);
int remember_valid(void);
int search_valid(void);

static int urldecode(unsigned char *source, unsigned char *dest)
{
	int num = 0, i, index = 0;
	int retval = 0;
	while (*source)
	{
		if (*source == '%')
		{
			num = 0;
			retval = 0;
			for (i = 0; i < 2; i++)
			{
				*source++;
				if (*(source) < ':')
				{
					num = *(source)-48;
				}
				else if (*(source) > '@' && *(source) < '[')
				{
					num = (*(source)-'A') + 10;
				}
				else
				{
					num = (*(source)-'a') + 10;
				}

				if ((16 * (1 - i)))
					num = (num * 16);
				retval += num;
			}
			dest[index] = retval;
			index++;
		}
		else
		{
			dest[index] = *source;
			index++;
		}
		*source++;
	}
	return index;
}


static int error400(SOCKET cli_socket) {
	send(cli_socket, http_400, strlen(http_400), 0);
	return 0;
}