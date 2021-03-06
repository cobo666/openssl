CPP := g++
CC := gcc

all: adler32 rabinkarp rsync_upload rsync_download crc32 base64_enc base64_dec huffman_enc huffman_dec rsa_enc rsa_dec aes_enc aes_dec upload download aes_upload aes_download aes_upload_reverse aes_download_reverse getrandom


adler32: adler32.c
	$(CC) -o adler32 adler32.c

rabinkarp: rabinkarp.c
	$(CC) -o rabinkarp rabinkarp.c

rsync_upload: rsync_upload.c
	$(CC) -o rsync_upload rsync_upload.c md5sum.c -O3
rsync_download: rsync_download.c
	$(CC) -o rsync_download rsync_download.c md5sum.c -O3

crc32: crc32.c
	$(CC) -o crc32 crc32.c

base64_enc: base64_enc.c
	$(CC) -o base64_enc base64_enc.c base64.c
base64_dec: base64_dec.c
	$(CC) -o base64_dec base64_dec.c base64.c

huffman_enc: huffman_enc.c huffman.c
	$(CC) -o huffman_enc huffman_enc.c huffman.c
huffman_dec: huffman_dec.c huffman.c
	$(CC) -o huffman_dec huffman_dec.c huffman.c


rsa_enc: rsa_enc.cpp
	$(CPP) -o rsa_enc rsa_enc.cpp -lcrypto
rsa_dec: rsa_dec.cpp
	$(CPP) -o rsa_dec rsa_dec.cpp -lcrypto

aes_enc: aes_enc.cpp
	$(CPP) -o aes_enc aes_enc.cpp -lcrypto
aes_dec: aes_dec.cpp
	$(CPP) -o aes_dec aes_dec.cpp -lcrypto

upload: upload.cpp
	$(CPP) -o upload upload.cpp
download: download.cpp
	$(CPP) -o download download.cpp

aes_upload: aes_upload.cpp
	$(CPP) -o aes_upload aes_upload.cpp -lcrypto
aes_download: aes_download.cpp
	$(CPP) -o aes_download aes_download.cpp -lcrypto

aes_upload_reverse: aes_upload_reverse.cpp
	$(CPP) -o aes_upload_reverse aes_upload_reverse.cpp -lcrypto
aes_download_reverse: aes_download_reverse.cpp
	$(CPP) -o aes_download_reverse aes_download_reverse.cpp -lcrypto

getrandom: getrandom.cpp
	$(CPP) -o getrandom getrandom.cpp

clean:
	rm adler32 rabinkarp rsync_upload rsync_download crc32 base64_enc base64_dec huffman_enc huffman_dec rsa_enc rsa_dec aes_enc aes_dec upload download aes_upload aes_download aes_upload_reverse aes_download_reverse getrandom
