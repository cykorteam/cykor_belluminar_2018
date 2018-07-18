from hashlib import md5
from functools import wraps
from subprocess import Popen, PIPE
from threading import Timer, Thread

import os
import random
import string
import tempfile
import shlex

from flask import Flask, render_template, session, request, jsonify, render_template_string


ROOT = os.path.dirname(__file__)
CHROME_PATH = 'C:/Program Files (x86)/Google/Chrome/Application/chrome.exe'
BACKEND_PATH = os.path.join(ROOT, 'binary.exe')
TIMEOUT = 60 * 3
PORT = 5000

app = Flask(__name__)
app.secret_key = os.urandom(32).encode('hex')

messages = {}
messages_client = {}
ports = {}
procs_chrome = {}
procs_backend = {}


def run(cmd, timeout_sec, start_callback=None, after_callback=None):
    proc = Popen(shlex.split(cmd), stdout=PIPE, stderr=PIPE)
    timer = Timer(timeout_sec, proc.kill)
    if start_callback:
        start_callback(proc)
    try:
        timer.start()
        proc.communicate()
    finally:
        timer.cancel()
        if after_callback:
            after_callback()


def local_only(func):
    @wraps(func)
    def handler(*args, **kwargs):
        host = request.headers['host'].lower().strip().split(':')[0]
        if host != 'localhost':
            return render_template_string(
                '<link href="/static/semantic.min.css" rel="stylesheet"><h1>Local only!</h1>')
        else:
            return func(*args, **kwargs)
    return handler


def register_chrome(id_, proc):
    procs_chrome[id_] = proc


def terminate_chrome(id_):
    procs_chrome[id_].kill()


def register_backend(id_, proc):
    procs_backend[id_] = proc


def terminate_backend(id_):
    procs_backend[id_].kill()


def clear_chat(id_, path):
    print 'Client ID', id_, 'ended'
    del messages[id_]
    del messages_client[id_]
    os.system('rmdir /s /q "%s"' % path)


def execute_program():
    session['id'] = id_ = str(random.getrandbits(128))
    session['path'] = path = tempfile.mkdtemp()
    messages[session['id']] = []
    messages_client[session['id']] = []
    chrome_argv = '"%s" --user-data-dir="%s" http://localhost:%d/admin?id=%s --media-cache-size=1'\
        ' --disk-cache-size=1 --headless --disable-gpu --remote-debugging-port=0' % (
            CHROME_PATH, path, PORT, id_)
    backend_argv = '"%s"' % BACKEND_PATH

    def on_start(proc):
        register_backend(id_, proc)
        port = ''
        for _ in range(100):
            port += proc.stdout.read(1)
            if '\n' in port:
                break
        assert 'PORT : ' in port
        port = int(port.split('PORT : ')[1].strip())
        ports[id_] = port
        print 'Backend started with port', port
        # run chrome
        Thread(
            target=lambda: run(chrome_argv,
                               timeout_sec=TIMEOUT,
                               start_callback=lambda proc_chrome: register_chrome(id_, proc_chrome),
                               after_callback=lambda: [clear_chat(id_, path), terminate_backend(id_)]
                               )
        ).start()

    # run backend
    Thread(
        target=lambda: run(backend_argv,
                           timeout_sec=TIMEOUT,
                           start_callback=on_start,
                           after_callback=lambda: terminate_chrome(id_)
                           )
    ).start()


@app.route('/')
def index():
    return render_template('index.html', timeout=TIMEOUT)


@app.route('/pow')
def proof_of_work():
    session['pow'] = ''.join(random.choice(string.lowercase)
                             for _ in range(16))
    return session['pow']


@app.route('/', methods=['POST'])
def open_client():
    if md5(session['pow'] + request.form['data']).hexdigest()[:5] == '00000':
        execute_program()
        return 'success'
    else:
        return 'pow error'


@app.route('/status')
def status():
    return 'ok' if session.get('id') and session['id'] in messages else 'closed'


@app.route('/admin')
@local_only
def admin():
    return render_template('admin.html', id=request.args['id'], port=ports[request.args['id']])


@app.route('/pending')
@local_only
def pending():
    msg = messages.get(request.args['id'])
    if msg is not None:
        response = jsonify(msg)
        msg[:] = []
        return response
    else:
        return jsonify('closed')


@app.route('/pending_client')
def pending_client():
    msg = messages_client.get(session['id'])
    if msg is not None:
        response = jsonify(msg)
        msg[:] = []
        return response
    else:
        return jsonify('closed')


@app.route('/chat', methods=['POST'])
def send_chat():
    messages.get(session['id'], []).append(request.form['text'])
    messages_client.get(session['id'], []).append(request.form['text'])
    return ''


@app.route('/chat_client', methods=['POST'])
def send_chat_client():
    messages_client.get(request.form['id'], []).append('>> ' + request.form['text'])
    return ''


app.jinja_env.auto_reload = True
app.config['TEMPLATES_AUTO_RELOAD'] = True

if __name__ == '__main__':
    app.run(port=PORT, host='0.0.0.0', debug=False, threaded=True)
