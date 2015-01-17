#include "http.h"
#include "gettext.h"
#include "mem.h"

int http_progress = 0;

/**
 * Emptyblock is a statically defined variable for functions to return if they are unable
 * to complete a request
 */
const struct block emptyblock = {0, 0, NULL};

//The maximum amount of bytes to send per net_write() call
#define NET_BUFFER_SIZE 1024

// Write our message to the server
static s32 send_message(s32 server, char *msg) {
	s32 bytes_transferred = 0;
    s32 remaining = strlen(msg);
    while (remaining) {
        if ((bytes_transferred = net_write(server, msg, remaining > NET_BUFFER_SIZE ? NET_BUFFER_SIZE : remaining)) > 0) {
            remaining -= bytes_transferred;
			usleep (20 * 1000);
        } else if (bytes_transferred < 0) {
            return bytes_transferred;
        } else {
            return -ENODATA;
        }
    }
	return 0;
}

/**
 * Connect to a remote server via TCP on a specified port
 *
 * @param u32 ip address of the server to connect to
 * @param u32 the port to connect to on the server
 * @return s32 The connection to the server (negative number if connection could not be established)
 */
static s32 server_connect(u32 ipaddress, u32 socket_port) {
	//Initialize socket
	s32 connection = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (connection < 0) return connection;

	struct sockaddr_in connect_addr;
	memset(&connect_addr, 0, sizeof(connect_addr));
	connect_addr.sin_family = AF_INET;
	connect_addr.sin_port = socket_port;
	connect_addr.sin_addr.s_addr= ipaddress;
	
	//Attemt to open the socket
	if (net_connect(connection, (struct sockaddr*)&connect_addr, sizeof(connect_addr)) == -1) {
		net_close(connection);
		return -1;
	}
	return connection;
}

//The amount of memory in bytes reserved initially to store the HTTP response in
//Be careful in increasing this number, reading from a socket on the Wii 
//will fail if you request more than 20k or so
#define HTTP_BUFFER_SIZE 1024 * 5

//The amount of memory the buffer should expanded with if the buffer is full
#define HTTP_BUFFER_GROWTH 1024 * 5

/**
 * This function reads all the data from a connection into a buffer which it returns.
 * It will return an empty buffer if something doesn't go as planned
 *
 * @param s32 connection The connection identifier to suck the response out of
 * @return block A 'block' struct (see http.h) in which the buffer is located
 */
struct block read_message(s32 connection, const char *fname)
{
	//Create a block of memory to put in the response
	struct block buffer;
	FILE *f = NULL;
	bool found = false;
	unsigned char *buffer_found;

	buffer_found = mem_calloc(HTTP_BUFFER_SIZE);
	buffer.data = mem_calloc(HTTP_BUFFER_SIZE);
	buffer.filesize = 0;
	buffer.size = HTTP_BUFFER_SIZE;
	int progress_count = 0;

	if(buffer.data == NULL) {
		SAFE_FREE(buffer_found);
		return emptyblock;
	}
	
	if (fname != NULL)
	{
		if ((f = fopen(fname,"wb")) == NULL) {
			SAFE_FREE(buffer_found);
			SAFE_FREE(buffer.data);
			return emptyblock;
			}
	}

	//The offset variable always points to the first byte of memory that is free in the buffer
	u32 offset = 0;
	
	while(1)
	{
		//Fill the buffer with a new batch of bytes from the connection,
		//starting from where we left of in the buffer till the end of the buffer
		s32 bytes_read;
		
		if (!found)
			bytes_read = net_read(connection, buffer.data + offset, buffer.size - offset);
		else
			bytes_read = net_read(connection, buffer_found, HTTP_BUFFER_SIZE);
		
		//Anything below 0 is an error in the connection
		if(bytes_read < 0)
		{
			printf(gt("Connection error from net_read()  Errorcode: %i"), bytes_read);
			printf("\n");
			SAFE_FREE(buffer_found);
			SAFE_FREE(buffer.data);
			return emptyblock;
		}
		
		//No more bytes were read into the buffer,
		//we assume this means the HTTP response is done
		if(bytes_read == 0)
		{
			break;
		}
		
		if (fname != NULL) {
			if (!found) {
				int i;
				for (i=0; i < bytes_read; i++) {
					int off = offset + i - 3;
					if (off >= 0) {
						if (memcmp(buffer.data+off, "\r\n\r\n", 4) == 0) {
							found = true;
							offset = off + 4;
							break;
						}
					}
				}
				if (found) {
					fwrite(buffer.data + offset, 1, bytes_read - offset, f);
					buffer.filesize += bytes_read-i;
				} else {
					offset += bytes_read;
				}
			} else {
				fwrite(buffer_found, 1, bytes_read, f);
				buffer.filesize += bytes_read;
			}
		} else {
			offset += bytes_read;
		}

		//Check if we have enough buffer left over,
		//if not expand it with an additional HTTP_BUFFER_GROWTH worth of bytes
		if(offset >= buffer.size)
		{
			buffer.size += HTTP_BUFFER_GROWTH;
			buffer.data = mem_realloc(buffer.data, buffer.size);
			
			if(buffer.data == NULL)
			{
				SAFE_FREE(buffer_found);
				return emptyblock;
			}
		}

		// display progress
		if (http_progress) {
			if (fname == NULL) {
				while (offset / http_progress >= progress_count) {
					printf(".");
					progress_count++;
				}
			} else {
				while (buffer.filesize / http_progress >= progress_count) {
					printf(".");
					progress_count++;
				}
			}
		}
	}

	//At the end of above loop offset should be precisely the amount of bytes that were read from the connection
	buffer.size = offset;
	SAFE_FREE(buffer_found);
	if (fname != NULL)
		fclose(f);
		
	//Shrink the size of the buffer so the data fits exactly in it
	buffer.data = mem_realloc(buffer.data, buffer.size);
	if(buffer.data == NULL)
	{	
		return emptyblock;
	}
	
	return buffer;
}

/**
 * Downloads the contents of a URL to memory
 * This method is not threadsafe (because networking is not threadsafe on the Wii)
 */
struct block downloadfile(const char *url) {
	return downloadfile_fname(url, NULL);
}

struct block downloadfile_fname(const char *url, const char *fname)
{
	//Check if the url starts with "http://", if not it is not considered a valid url
	if(strncmp(url, "http://", strlen("http://")) != 0)
	{
		printf(gt("URL '%s' doesn't start with 'http://'"), url);
		printf("\n");
		return emptyblock;
	}
	
	//Locate the path part of the url by searching for '/' past "http://"
	char *path = strchr(url + strlen("http://"), '/');
	
	//At the very least the url has to end with '/', ending with just a domain is invalid
	if(path == NULL)
	{
		printf(gt("URL '%s' has no PATH part"), url);
		printf("\n");
		return emptyblock;
	}
	
	//Extract the domain part out of the url
	int domainlength = path - url - strlen("http://");
	
	if(domainlength == 0)
	{
		printf(gt("No domain part in URL '%s'"), url);
		printf("\n");
		return emptyblock;
	}
	
	char domain[domainlength + 1];
	strncpy(domain, url + strlen("http://"), domainlength);
	domain[domainlength] = '\0';
	
	//Parsing of the URL is done, start making an actual connection
	u32 ipaddress = getipbynamecached(domain);
	
	if(ipaddress == 0)
	{
		printf("\n");
		printf(gt("domain %s could not be resolved"), domain);
		return emptyblock;
	}


	s32 connection = server_connect(ipaddress, 80);
	
	if(connection < 0) {
		printf(gt("Error establishing connection"));
		return emptyblock;
	}
	
	//Form a nice request header to send to the webserver
	extern char CFG_VERSION[];
//referer removed for compatability with geckocodes.org if it causes problems make it conditional
//	char* headerformat = "GET %s HTTP/1.0\r\nHost: %s\r\nReferer: %s\r\nUser-Agent: CFG-Loader %s\r\n\r\n";
//	char header[strlen(headerformat) + strlen(domain) + strlen(path) + strlen(domain) + strlen(CFG_VERSION) + 16];
//	sprintf(header, headerformat, path, domain, domain, CFG_VERSION);
	char* headerformat = "GET %s HTTP/1.0\r\nHost: %s\r\nUser-Agent: CFG-Loader %s\r\n\r\n";
	char header[strlen(headerformat) + strlen(path) + strlen(domain) + strlen(CFG_VERSION) + 16];
	sprintf(header, headerformat, path, domain, CFG_VERSION);

	//Do the request and get the response
	send_message(connection, header);
	struct block response = read_message(connection, fname);
	net_close(connection);

	// Check response status. Should be something like HTTP/1.1 200 OK
	if (response.size > 10 && strncmp((char*)response.data, "HTTP/", 5)==0) {
		char htstat[100];
		int i;
		for (i=0; i<100 && i<response.size; i++) {
			if (response.data[i] == '\n' || response.data[i] == '\r') {
				strncpy(htstat, (char*)response.data, i);
				htstat[i] = 0;
				//printf("HTTP response status: %s\n", htstat);
				char *codep;
				codep = strchr(htstat, ' ');
				if (codep) {
					int code;
					if (sscanf(codep+1, "%d", &code) == 1) {
						//printf("HTTP response code: %d\n", code);
						//if (code != 200) {
						if (code >= 400) {
							printf("%s: %s", gt("ERROR"), htstat);
							if (!http_progress) printf("\n");
							SAFE_FREE(response.data);
							return emptyblock;
						}
					}
				}
				break;
			}
		}
	}

	if (fname != NULL) {
		return response;
	}
	// Search for the 4-character sequence \r\n\r\n in the response
	// which signals the start of the http payload (file)
	unsigned char *filestart = NULL;
	u32 filesize = 0;
	int i;
	if (response.size > 3)			//need at least the 4 chars we are trying to match
		for(i = 0; i < response.size-3; i++)
		{
			if (memcmp(response.data+i, "\r\n\r\n", 4) == 0) {
				filestart = response.data + i + 4;
				filesize = response.size - i - 4;
				break;
			}
		}
	
	if(filestart == NULL)
	{
		printf(gt("HTTP Response was without a file"));
		printf("\n");
		SAFE_FREE(response.data);
		return emptyblock;
	}
	
	// move file part of the response into the start of the block
	memmove(response.data, filestart, filesize);
	// free extra memory
	response.data = mem_realloc(response.data, filesize);
	response.size = filesize;
	
	return response;
}

struct block downloadfile_progress(const char *url, int size)
{
	return downloadfile_progress_fname(url, size, NULL);
}

struct block downloadfile_progress_fname(const char *url, int size, const char *fname)
{
	int x = http_progress;
	struct block file;
	if (size == 1) size = 100;
	size *= 1024;
	http_progress = size;
	file = downloadfile_fname(url, fname);
	http_progress = x;
	return file;
}


