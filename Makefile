setup:
	gcc -std=gnu99 -o enc_server enc_server.c
	gcc -std=gnu99 -o enc_client enc_client.c
	gcc -std=gnu99 -o dec_server dec_server.c
	gcc -std=gnu99 -o dec_client dec_client.c
	gcc -std=gnu99 -lm -o keygen keygen.c

clean:
	rm enc_server 
	rm enc_client 
	rm dec_server 
	rm dec_client 
	rm keygen
