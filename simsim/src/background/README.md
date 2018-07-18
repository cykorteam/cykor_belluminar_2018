# ActiveX(?)

### Summary

This is stupid backgrounded program.
Very stupid.

#### Concept

WebServer

#### Function

##### login

parameter : id, pw, sha256(id+pw)?

Check If requested user exists.
If not, this function response requested id of client.
This function has ```leak``` vulnerability.

##### register

parameter : id, pw

Each key(id, pw)'s Max Length is 64 bytes
Check if keys' value is valid (Only allow ascii-num)
Insert id, pw to table ```Users```

##### save

parameter : msg

msg' Max Length is 256 bytes.
Check if msg' value has char ```0x27(')```

##### load

Not implemented yet.

#### Vulnerability

race condition + type confusion

Whenever connection is establied, thread is created and execute ```connection``` function.
Requested parameters are saved at ```struc_params``` declared as a global variable (structure array). The ```struc_param``` structure looks like this.
```
struct struc_param {
  char * key;
  char * value;
}

struct struc_param struc_params[MAX_PARAM];
```

When 2 threads are created, (First : register, Second : save) the race condition is happened.

When ```register``` function's parameters are overwritten by ```save``` function's ```msg``` in ```id_pw_valid``` function, It can bypass valid char check (Because It only check 64 bytes, and ```msg``` is 256 bytes). 
Then, We can exploit sqlite3. 

