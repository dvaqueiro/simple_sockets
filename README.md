# TCP client/server researching in C

Test some simple implementations of TCP client/server in C capable handle multiples clients.

## Server types:
- **simple_server.c**: This servers accepts only one client at a time. This is the base server to test creating, binding, ... sockets in C.

- **select_server.c**: This server uses the `select` function to handle multiples clients. This is the base server to test the `select` function in C.

## Build and run
To build the server, run the following commands:
```bash
$ make build server
```

To run the server, run the following commands:
```bash
$ ./bin/server
```

To build the client, run the following commands:
```bash
$ make build client
```

To run the client, run the following commands:
```bash
$ ./bin/client
```
