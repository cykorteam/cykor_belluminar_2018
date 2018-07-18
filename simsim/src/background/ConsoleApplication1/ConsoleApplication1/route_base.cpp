#include "route.h"
#include "util.h"

int _notFound(SOCKET cli_socket)
{
	const char header[] = "Content-Length: 13\r\n\r\n";
	send(cli_socket, http_404, strlen(http_404), 0);
	send(cli_socket, header, strlen(header), 0);
	send(cli_socket, "404 Not Found", 13, 0);
	return 0;
}

char index_html[] = R"(
<html>
<head>
</head>
<body>
<script>

const encode = function(obj) {
	return Object.entries(obj).map(([k, v]) => `${k}=${encodeURIComponent(v)}`).join('&');
}

if(parent !== self) {
	parent.postMessage('Welcome! /remember hello world -> You: hello / Me: world', '*');
	parent.postMessage('First, you need to login. You: /login id pw or /join id pw .', '*');
}

let userID;

window.onmessage = function(event) {
	let echo = true, data;
	if(typeof event.data !== 'string') return;

	// All commands will use ajax request, so...
	const xhr = new XMLHttpRequest();

	if(userID !== undefined && event.data.startsWith('/remember ')) {
		const args = event.data.split('/remember ')[1].split(' ');
		const input = args[0], output = args.slice(1).join(' ');

		xhr.open('PUT', '/remember?' + encode({userid: userID, input, output}), false);
		xhr.send();

	} else if(event.data.startsWith('/login ')) {
		const args = event.data.split('/login ')[1].split(' ');
		const [id, pw] = args;
		xhr.open('GET', '/login?' + encode({id, pw}), false);
		xhr.send();

		echo = false;

		if(xhr.responseText !== 'wrong') {
			userID = xhr.responseText;
			data = 'Login success!';
		} else
			data = 'Wrong credentials!';

	} else if(event.data.startsWith('/join ')) {
		const args = event.data.split('/join ')[1].split(' ');
		const [id, pw] = args;
		xhr.open('PUT', '/register?' + encode({id, pw}), false);
		xhr.send();

	} else if(userID !== undefined) {
		const search = event.data.replace(/[^a-zA-Z]/g, '%');

		xhr.open('GET', '/search?' + encode({userid: userID, search}), false);
		xhr.send();

	} else {
		echo = false;
		data = 'You need to login!';
	}
	if(echo)
		data = xhr.responseText;
	if(parent !== self)
		parent.postMessage(data, '*');
	else
		console.log('>>', data);
}
</script>
</body>
</html>
)";

ROUTE("GET", "/", _index);
int _index(SOCKET cli_socket, char *, int)
{
	char additional_header[1000];
	send(cli_socket, http_200, strlen(http_200), 0);
	sprintf_s(additional_header, sizeof(additional_header), "Content-Type: text/html; charset=utf-8\r\nContent-Length: %d\r\n\r\n", strlen(index_html));
	send(cli_socket, additional_header, strlen(additional_header), 0);
	send(cli_socket, index_html, strlen(index_html), 0);
	return 0;
}

ROUTE("GET", "/favicon.ico", favicon);
int favicon(SOCKET s, char *, int) {
	char zerolen[] = "Content-Length: 0\r\n\r\n";
	send(s, http_200, strlen(http_200), 0);
	send(s, zerolen, strlen(zerolen), 0);
	return 0;
}
