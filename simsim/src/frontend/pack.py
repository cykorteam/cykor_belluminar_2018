#!/usr/bin/env python
import os
import zipfile

def zipdir(path):
    # zipf is zipfile handle
    zipf.write(path)

targets = [
'templates/admin.html',
'templates/index.html',
'static/pow_worker.js',
'static/jquery.min.js',
'static/semantic.min.css',
'binary.exe',
'hello.py',
'sqlite3.dll'
]
if __name__ == '__main__':
    zipf = zipfile.ZipFile('static/binary.zip', 'w', zipfile.ZIP_DEFLATED)
    for x in targets:
    	zipdir(x)
    zipf.close()