all: clean
	gcc -g rsh_server.c -o rsh_server
	gcc -g rsh_client.c -o rsh_client
clean:
	rm -rf rsh_client rsh_server
killserver:
	killall rsh_server