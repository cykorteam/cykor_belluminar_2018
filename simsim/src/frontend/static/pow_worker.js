// MD5 for PoW
self.importScripts('https://blueimp.github.io/JavaScript-MD5/js/md5.js');

onmessage = function(event) {
	var prefix = event.data, i = 0;
	while(1) {
		if(md5(prefix + i, null).substr(0, 5) === '00000') {
			postMessage(i);
			break;
		}
		i++;
	}
}