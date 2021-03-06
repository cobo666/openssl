#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#ifdef WIN32
	#include <windows.h>
	#include <winsock.h>

	#pragma comment(lib, "wsock32.lib")

	typedef int socklen_t;
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <dlfcn.h>
	#include <sys/select.h>
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#define closesocket close

	typedef	int SOCKET;
	#define SOCKET_ERROR	-1
	#define INVALID_SOCKET	-1


	// RSA
	#include <openssl/pem.h>
	#include <openssl/ssl.h>
	#include <openssl/rsa.h>
	#include <openssl/evp.h>
	#include <openssl/bio.h>
	#include <openssl/err.h>

	// AES
	#include <openssl/conf.h>

//MD5
	#include <openssl/md5.h>
#endif



#define MAX(x,y) (x) > (y) ? (x) : (y)
#define MIN(x,y) (x) < (y) ? (x) : (y)

void md5sum(char *data, int size, char *hash)
{
	MD5_CTX ctx;
	unsigned char digest[16] = { 0 };

	memset(&ctx, 0, sizeof(MD5_CTX));
	MD5_Init(&ctx);
	MD5_Update(&ctx, data, size);
	MD5_Final(digest, &ctx);

	for (int i = 0; i < 16; ++i)
	{
		sprintf(&hash[i * 2], "%02x", (unsigned int)digest[i]);
	}
}


char *get_file(char *filename, unsigned int *size)
{
	FILE	*file;
	char	*buffer;
	int	file_size, bytes_read;
	
	file = fopen(filename, "rb");
	if (file == NULL)
		return 0;
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	buffer = new char [file_size + 1];
	bytes_read = (int)fread(buffer, sizeof(char), file_size, file);
	if (bytes_read != file_size)
	{
		delete [] buffer;
		fclose(file);
		return 0;
	}
	fclose(file);
	buffer[file_size] = '\0';

	if (size != NULL)
	{
		*size = file_size;
	}

	return buffer;
}

int write_file(char *filename, const char *bytes, int size)
{
    FILE *fp = fopen(filename, "wb");
    int ret;
    
    if (fp == NULL)
    {
        perror("Unable to open file for writing");
        return -1;
    }
    
    ret = fwrite(bytes, sizeof(char), size, fp);
    
    if (ret != size)
    {
        printf("fwrite didnt write all data\n");
	fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

int aes_file_download(char *ip_str, unsigned short int port, char *response, int size, unsigned int *download_size, char *file_name, unsigned int &final_size, char *hash, unsigned char *encrypted_key)
{
	struct sockaddr_in	servaddr;
	SOCKET sock;
	int ret;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	memset(&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr(ip_str);
	servaddr.sin_port = htons(port);

	// 3 way handshake
	printf("Attempting to connect to %s\n", ip_str);
	ret = connect(sock, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in));
	if (ret == SOCKET_ERROR)
	{
#ifdef _WIN32
		ret = WSAGetLastError();

		switch (ret)
		{
		case WSAETIMEDOUT:
			printf("Fatal Error: Connection timed out.\n");
			break;
		case WSAECONNREFUSED:
			printf("Fatal Error: Connection refused\n");
			break;
		case WSAEHOSTUNREACH:
			printf("Fatal Error: Router sent ICMP packet (destination unreachable)\n");
			break;
		default:
			printf("Fatal Error: %d\n", ret);
			break;
		}
#else
		ret = errno;

	        switch(ret)
	        {
		case ENETUNREACH:
			printf("Fatal Error: The network is unreachable from this host at this time.\n(Bad IP address)\n");
			break;
	        case ETIMEDOUT:
	                printf("Fatal Error: Connecting timed out.\n");
	                break;
	        case ECONNREFUSED:
	                printf("Fatal Error: Connection refused\n");
	                break;
	        case EHOSTUNREACH:
	                printf("Fatal Error: router sent ICMP packet (destination unreachable)\n");
	                break;
	        default:
	                printf("Fatal Error: %d\n", ret);
	                break;
	        }
#endif
		return -1;
	}
	printf("TCP handshake completed\n");

	memset(response, 0, size);

	int expected_size = 0;
	recv(sock, (char *)&expected_size, 4, 0);
	recv(sock, (char *)file_name, 128, 0);
	recv(sock, (char *)&final_size, 4, 0);
	recv(sock, (char *)hash, 32, 0);
	recv(sock, (char *)encrypted_key, 256, 0);

	while (*download_size < expected_size)
	{
		*download_size += recv(sock, &response[*download_size], expected_size - *download_size, 0);
	}
	closesocket(sock);
	return 0;
}

void StripChars(const char *in, char *out, char *stripc)
{
    while (*in)
    {
    	bool flag = false;

	int length = strlen(stripc);
    	for(int i = 0; i < length; i++)
	{
		if (*in == stripc[i])
		{
			flag = true;
			break;
		}
	}

	if (flag)
	{
		in++;
		continue;
	}
        *out++ = *in++;
    }
    *out = 0;
}


RSA *createRSA(unsigned char *key, bool pub)
{
	RSA *rsa = NULL;
	BIO *keybio = BIO_new_mem_buf(key, -1); // a bio is just a memory buffer, -1 means do strlen of char *key

	if (keybio == NULL)
	{
		printf( "Failed to create key BIO");
		return 0;
	}

	if (pub)
	{
		rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
	}
	else
	{
		rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
	}
	BIO_free_all(keybio);

	if (rsa == NULL)
	{
		printf( "Failed to create RSA");
	}
 
	return rsa;
}

void handleErrors(void)
{
	ERR_print_errors_fp(stderr);
	abort();
}


int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
			unsigned char *iv, unsigned char *plaintext)
{
	EVP_CIPHER_CTX *ctx;

	int len;

	int plaintext_len;

	/* Create and initialise the context */
	if(!(ctx = EVP_CIPHER_CTX_new()))
		handleErrors();

	/*
	 * Initialise the decryption operation. IMPORTANT - ensure you use a key
	 * and IV size appropriate for your cipher
	 * In this example we are using 256 bit AES (i.e. a 256 bit key). The
	 * IV size for *most* modes is the same as the block size. For AES this
	 * is 128 bits
	 */
	if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
		handleErrors();

	/*
	 * Provide the message to be decrypted, and obtain the plaintext output.
	 * EVP_DecryptUpdate can be called multiple times if necessary.
	 */
	if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
		handleErrors();
	plaintext_len = len;

	/*
	 * Finalise the decryption. Further plaintext bytes may be written at
	 * this stage.
	 */
	if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
		handleErrors();
	plaintext_len += len;

	/* Clean up */
	EVP_CIPHER_CTX_free(ctx);

	return plaintext_len;
}

int aes_decrypt(unsigned char *key, unsigned char *iv, char *filename, unsigned int &decryptedtext_len, char *remote_hash)
{
	unsigned int size = 0;
	unsigned char *ciphertext = (unsigned char *)get_file(filename, &size);
	if (ciphertext == NULL)
	{
		printf("Unable to open %s\r\n", filename);
		return -1;
	}

	/* Buffer for the decrypted text */
	unsigned char *decryptedtext = (unsigned char *)malloc(size);
	if (decryptedtext == NULL)
	{
		perror("malloc failed");
		return -1;
	}

	unsigned int ciphertext_len = size;

	decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, iv, decryptedtext);
	decryptedtext[decryptedtext_len] = '\0';

	char new_filename[256] = {0};



	unsigned char local_hash[33] = {0};

	md5sum((char *)decryptedtext, decryptedtext_len, (char *)&local_hash[0]);


	delete [] ciphertext;
	if (strcmp(remote_hash, (char *)&local_hash[0]) == 0)
	{
		printf("MD5 Hash Pass: %s\r\n", remote_hash);
	}
	else
	{
		printf("MD5 Hash Fail: local %s != remote %s\r\n", local_hash, remote_hash);
		free((void *)decryptedtext);
		return -1;
	}


	sprintf(new_filename, "%s_decrypted", filename);
	write_file(new_filename, (char *)decryptedtext, decryptedtext_len);
	free((void *)decryptedtext);
	return 0;
}


int main(int argc, char *argv[])
{
	unsigned short port = 65535;
	unsigned int size = 0;
	unsigned char encrypted_key[256] = {0};
	char remote_hash[33] = {0};

	if (argc < 5)
	{
		printf("Usage: download ip port max_size prikey\r\n");
		printf("Example: ./aes_download 127.0.0.1 65535 536870912 id_rsa\r\n");
		return 0;
	}

	port = atoi(argv[2]);
	size = atoi(argv[3]);

	unsigned int prikey_size = 0;
	unsigned char *prikey = (unsigned char *)get_file(argv[4], &prikey_size);

	printf("Allocating %d bytes\r\n", size);
	char *response = (char *)malloc(size);
	if (response == NULL)
	{
		perror("malloc failed");
	}

	unsigned int download_size = 0;
	unsigned int final_size = 0;
	char file_name[128] = {0};

	printf("Attempting to download file from ip %s port %d\r\n", argv[1], (int)port);
	int ret = aes_file_download(argv[1], port, response, size, &download_size, file_name, final_size, remote_hash, encrypted_key);
	if (ret != 0 || download_size == 0)
	{
		printf("Download failed\r\n");
		return 0;
	}

	printf("Download complete\r\n");
	printf("Got %d bytes file name %s remote md5sum %s\r\n", download_size, file_name, remote_hash);


	char new_filename[256] = {0};
	char strip_filename[256] = {0};

	StripChars(file_name, strip_filename, (char *)".\\/;:*?\"<>|");

	sprintf(new_filename, "downloaded_%s", strip_filename);
	printf("Saving as file name %s\r\n", new_filename);
	write_file(new_filename, response, download_size);
	printf("Attempting to decrypt AES key with RSA\r\n");

	unsigned char decrypted[4098] = {0};

	RSA *rsa = createRSA(prikey, false);
	int decrypted_length = RSA_private_decrypt(256, encrypted_key, decrypted, rsa, RSA_PKCS1_PADDING);
	if (decrypted_length == -1)
	{
		char err[130];

		ERR_load_crypto_strings();
		ERR_error_string(ERR_get_error(), err);
		printf("ERROR: %s\n", err);
		free((void *)response);
		return -1;
	}
	RSA_free(rsa);

//	printf("Decrypted Text: %s\n", decrypted);
//	printf("Decrypted Length: %d\n", decrypted_length);

	if (decrypted_length != 16 + 32 + 1)
	{
		printf("Unexpected AES key string size\r\n");
		free((void *)response);
		return -1;
	}

	if (decrypted[16] != ' ')
	{
		printf("Unexpected AES key format: %s\r\n", decrypted);
		free((void *)response);
		return -1;
	}



	unsigned char key[128] = {0};
	unsigned char iv[128] = {0};

	ret = sscanf((char *)&decrypted[0], "%s %s", (char *)&iv[0], (char *)&key[0]);

	if (ret != 2)
	{
		printf("Unexpected AES key format: %s matched %d\r\n", decrypted, ret);
		free((void *)response);
		return -1;
	}

	printf("Decrypted AES Key successfully\r\n");

	printf("Attempting to decrypt file with remote hash %s\r\n", remote_hash);
	aes_decrypt(key, iv, new_filename, size, remote_hash);



	if (size == final_size)
	{
		printf("decrypted successfully\r\n");
		printf("Saved as %s_decrypted\r\n", new_filename);
	}
	free((void *)response);
	delete [] prikey;

	return 0;
}

