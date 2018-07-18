#include "util.h"

std::vector<Route *> *routes;

std::vector<Route *> *getRoutes() {
	if (!routes) routes = new std::vector<Route *>();
	return routes;
}

sqlite3 * db;
struct struc_param struc_params[MAX_PARAM + 1];

int parse_query(char *query, char delimiter)
{
	int i = 0;
	char *tmp;

	if (NULL == query || '\0' == *query) {
		return -1;
	}

	struc_params[i++].key = query;
	while (i < MAX_PARAM && NULL != (query = strchr(query, delimiter))) {
		*query = '\0';
		struc_params[i].key = ++query;
		// struc_params[i].value = NULL;

		/* Go back and split previous param */
		if (i > 0) {
			if ((tmp = strchr(struc_params[i - 1].key, '=')) != NULL) {
				struc_params[i - 1].value = tmp;
				*(struc_params[i - 1].value)++ = '\0';
				auto pos = struc_params[i - 1].value;
				pos[urldecode((unsigned char *)pos, (unsigned char *)pos)] = '\x00';
			}
		}
		i++;
	}

	/* Go back and split last param */
	if ((tmp = strchr(struc_params[i - 1].key, '=')) != NULL) {
		struc_params[i - 1].value = tmp;
		*(struc_params[i - 1].value)++ = '\0';
		auto pos = struc_params[i - 1].value;
		pos[urldecode((unsigned char *)pos, (unsigned char *)pos)] = '\x00';
	}
	else --i;

	return i;
}

