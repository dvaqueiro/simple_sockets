build_server:
	gcc -o bin/server -Wall -pedantic src/server_select.c

build_client:
	gcc -o bin/client -Wall -pedantic src/client.c

clean:
	rm -f bin/*
