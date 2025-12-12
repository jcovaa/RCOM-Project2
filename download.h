/**
 * @file download.h
 * @brief Header file for download.c
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>

#define FTP_SERVER_PORT 21

#define MAX_RESPONSE_SIZE 1024

/**
 * @struct url_info
 * @brief Structure that contains parsed components of a URL
 */
typedef struct
{
   char protocol[6];
   char user[64];
   char pass[64];
   char host[256];
   char path[512];
   char filename[256];
   char ip[16];
} url_info;

/**
 * @brief Parses a URL into a url_info structure.
 *
 * @param url The URL string to parse.
 * @param info Pointer to a url_info structure to fill with parsed data.
 *
 * @return 0 on success, -1 on failure.
 */
int parse_url(const char *url, url_info *info);

/**
 * @brief Returns the port number based on the protocol in url_info.
 *
 * @param info Pointer to a url_info structure.
 * @return Port number on success, -1 on failure.
 */
int get_port(const url_info *info);

/**
 * @brief Connects to a socket given an IP address and port.
 *
 * @param ip IP address as a string.
 * @param port Port number to connect to.
 * @return Socket file descriptor on success, -1 on failure.
 */
int connect_socket(const char *ip, int port);

/**
 * @brief Authenticates to an FTP server using provided username and password.
 *
 * @param socket Socket file descriptor for the control connection.
 * @param user Username for authentication.
 * @param pass Password for authentication.
 * @return FTP server response code on success, -1 on failure.
 */
int authenticate_ftp(const int socket, const char *user, const char *pass);

/**
 * @brief Sets the transfer mode to binary (TYPE I).
 *
 * @param socket Socket file descriptor for the control connection.
 * @return FTP server response code on success, -1 on failure.
 */
int set_binary_mode(const int socket);

/**
 * @brief Sends the QUIT command to close the connection.
 *
 * @param socket Socket file descriptor for the control connection.
 * @return FTP server response code on success, -1 on failure.
 */
int close_connection(const int socket);

/**
 * @brief Enters passive mode on the FTP server.
 *
 * @param socket Socket file descriptor for the control connection.
 * @param ip Buffer to store the IP address returned by the server.
 * @param port Pointer to store the port number returned by the server.
 * @return FTP server response code on success, -1 on failure.
 */
int enter_passive_mode(const int socket, char *ip, int *port);

/**
 * @brief Sends a RETR command to the FTP server to retrieve a file.
 *
 * @param socketfd Socket file descriptor for the control connection.
 * @param filename Name of the file to retrieve.
 * @return FTP server response code on success, -1 on failure.
 */
int send_retr(const int socketfd, const char *filename);

/**
 * @brief Downloads a file from the FTP server over the data connection.
 *
 * @param socketfdA Socket file descriptor for the control connection.
 * @param socketfdB Socket file descriptor for the data connection.
 * @param filename Name of the file to save the downloaded data.
 * @return FTP server response code on success, -1 on failure.
 */
int download_file(const int socketfdA, const int socketfdB, char *filename);

/**
 * @brief Reads a reply from the FTP server.
 *
 * @param socket Socket file descriptor to read from.
 * @param buffer Buffer to store the server's response.
 * @return FTP server response code on success, -1 on failure.
 */
int read_reply(const int socket, char *buffer);
