Work in Progress, but reasonably stable
=======================================

MemKV
=====
Simple little in-memory key/value store.  It consists of two parts:
- `memkvd` - A small daemon which holds keys and values
- `memkv`  - A client for `memkvd`

Written for OpenBSD, probably needs some tweaking for other OSs.

Features
--------
- Store and retrieve small amounts of data (e.g. usernames and passwords) without touching disk
- Single static binaries each for client and daemon
- Reasonably secure, more or less[^1]
- No network comms
- No external dependencies[^1]

[^1]: on OpenBSD, at least.

Quickstart
----------
```sh
$ make       # Build it
$ ./memkvd   # Start the daemon
$ ./memkv -h # What can we do?
Usage: memkv [-h] [-S path] {-gsdl} [key [value]]

Gets, sets, deletes, or lists key/values pairs stored in memkvd.

Flags:
  -h      - This help
  -S path - Path to memkvd's socket (default: $HOME/.memkvd.sock)
  -g      - Get a key's value
  -s      - Set a key's value
  -d      - Delete a key/value pair
  -l      - List all keys

$ ./memkv -s myname r00t # Set a not-very-secret value
$ ./memkv -s mypass      # Set a value without putting it in argv
Value (will not prompt):
Added mypass

$ ./memkv -l # List what's stored
myname
mypass

$ ./memkv -g myname                                                 # Get a value
r00t
$ ./memkv -g mypass | ldap search -y- -D "$(./memkv -g myname)",... # Easy :)
```

Usage
-----

### Server (`memkvd`):

```
Usage: memkvd [-dhr] [-S path]

Gets, sets, deletes, or lists key/values pairs stored in memkvd.

Flags:
  -h      - This help
  -S path - Path to the unixsocket (default: $HOME/.memkvd.sock)
  -d      - Debug mode: stay in the foreground while running
  -r      - Remove the unix socket if it exists
```

### Client (`memkv`):
```
Usage: memkv [-h] [-S path] {-gsdl} [key [value]]

Gets, sets, deletes, or lists key/values pairs stored in memkvd.

Flags:
  -h      - This help
  -S path - Path to memkvd's socket (default: $HOME/.memkvd.sock)
  -g      - Get a key's value
  -s      - Set a key's value
  -d      - Delete a key/value pair
  -l      - List all keys
```

Building
--------
A simple `make` should be all that's needed.  This'll produce two binaries,
`memkv` and `memkvd`.  Probably easiest to copy them somewhere in your `$PATH`.

To remove the generated binaries and object files binaries use `make clean`.

Protocol
--------
The client/daemon protocol is fairly simple.  The client sends a request to the
daemon via a Unix socket and writes to stdout whatever the daemon sends back.

Requests consist of a character specifying the operation, and one or two
[strings](Strings) depending on the operation.  The operations are as follows:

Name   | Byte | String1 | String2 | Description
-------|------|---------|---------|-
Get    | `g`  | Key     | _none_  | Get a value for a key
Set    | `s`  | Key     | Value   | Set a value for a key
Delete | `d`  | Key     | _none_  | Delete a key/value pair
List   | `l`  | _none_  | _none_  | List stored keys

### Strings
Strings are sent as a 2-byte, host byte order length, followed by that many
bytes.  NUL bytes aren't welcome.  Please don't send any.

### Obligatory ASCII Diagrams

#### String:
```
+----------------+------------------------------
| 2-byte length  | String, not NUL-terminated...
+----------------+------------------------------
```

In the below, the `Key...` and `Value...` sections are strings, as above.

#### Get:
```
+--------+-------
|  'g'   | Key...
+--------+-------
```

#### Set:
```
+--------+--------+---------
|  's'   | Key... | Value...
+--------+--------+---------
```

#### Delete:
```
+--------+-------
|  'd'   | Key...
+--------+-------
```

#### List:
```
+--------+
|  'l'   |
+--------+
```

Why?
----
Originally the idea was to fiddle around with
[shared memory](https://man.openbsd.org/shm_open) as a way to keep secrets
off disk without a daemon running.  Turns out that writes to `/tmp`, and
the [`shmget(2)`](https://man.openbsd.org/shmget) side of things isn't any
better.

`¯\_(ツ)_/¯`
