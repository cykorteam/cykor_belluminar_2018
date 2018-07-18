#include "util.h"

int id_pw_valid(void)
{

	int idlen = strlen(struc_params[0].value);
	int pwlen = strlen(struc_params[1].value);

	int i, j;
	char allow[76] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz.,!@#$^&|*&\x00";
	int tick = 0;
	struct struc_param tmp;

	if (!strcmp(struc_params[0].key, "id"))
	{
		if (strcmp(struc_params[1].key, "pw"))return 0;
	}
	else if (!strcmp(struc_params[0].key, "pw"))
	{
		if (strcmp(struc_params[1].key, "id"))return 0;
		tmp = struc_params[0];
		struc_params[0] = struc_params[1];
		struc_params[1] = tmp;
	}
	else
	{
		return 0;
	}

	for (i = 0; i<idlen; i++)
	{
		for (j = 0; j<strlen(allow); j++)
		{
			if (struc_params[0].value[i] == allow[j])
				tick = 1;
		}
		// printf("%d\n", struc_params[0].value[i]);
		// printf("%d\n", i);
		if (!tick)return 0;
		tick = 0;
	}

	for (i = 0; i<pwlen; i++)
	{
		for (j = 0; j<strlen(allow); j++)
		{
			if ((struc_params[1].value[i] == allow[j]))
				tick = 1;
		}
		// printf("%d\n", i);
		if (!tick)return 0;
		tick = 0;
	}

	return 1;

}

int remember_valid(void)
{
	int i;
	struct struc_param tmp;

	for(int j = 0; j < 2; j++)
		if (!strcmp(struc_params[j].key, "userid")) {
			tmp = struc_params[2];
			struc_params[2] = struc_params[j];
			struc_params[j] = tmp;
			break;
		}

	if (!strcmp(struc_params[0].key, "input"))
	{
		if (strcmp(struc_params[1].key, "output"))return 0;
	}
	else if (!strcmp(struc_params[0].key, "output"))
	{
		if (strcmp(struc_params[1].key, "input"))return 0;
		tmp = struc_params[0];
		struc_params[0] = struc_params[1];
		struc_params[1] = tmp;
	}
	else
	{
		return 0;
	}

	int len[2] = { strlen(struc_params[0].value), strlen(struc_params[1].value) };

	for (i = 0; i<256; i++)
	{
		if (i < len[0] && struc_params[0].value[i] == 0x27)return 0;
		if (i < len[1] && struc_params[1].value[i] == 0x27)return 0;

	}

	char c;
	for (i = 0; i < strlen(struc_params[2].value); i++) {
		c = struc_params[2].value[i];
		if (c < '0' || c > '9')
			return 0;
	}

	return 1;
}

int search_valid(void)
{
	int i;
	struct struc_param tmp;

	if (!strcmp(struc_params[0].key, "search"))
	{
		if (strcmp(struc_params[1].key, "userid"))return 0;
	}
	else if (!strcmp(struc_params[0].key, "userid"))
	{
		if (strcmp(struc_params[1].key, "search"))return 0;
		tmp = struc_params[0];
		struc_params[0] = struc_params[1];
		struc_params[1] = tmp;
	}
	else
	{
		return 0;
	}

	for (i = 0; i<256; i++)
	{
		if (struc_params[0].value[i] == 0x27)return 0;
	}

	char c;
	for (i = 0; i < strlen(struc_params[1].value); i++) {
		c = struc_params[1].value[i];
		if (c < '0' || c > '9')
			return 0;
	}

	return 1;
}

