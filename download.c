#include "download.h"

int parse_url(const char *url, url_info *info)
{
   if (url == NULL)
   {
      return -1;
   }

   memset(info, 0, sizeof(url_info));

   const char *p = strstr(url, "://");
   if (p == NULL)
   {
      return -1;
   }

   size_t proto_len = p - url;
   if (proto_len >= sizeof(info->protocol))
   {
      return -1;
   }

   strncpy(info->protocol, url, proto_len);
   info->protocol[proto_len] = '\0';

   const char *start = p + 3;

   const char *at = strchr(start, '@');
   if (at)
   {
      const char *colon = strchr(start, ':');
      if (!colon || colon > at)
      {
         return -1;
      }

      size_t user_len = colon - start;
      size_t pass_len = at - colon - 1;

      strncpy(info->user, start, user_len);
      info->user[user_len] = '\0';
      strncpy(info->pass, colon + 1, pass_len);
      info->pass[pass_len] = '\0';

      start = at + 1;
   }
   else
   {
      strcpy(info->user, "anonymous");
      strcpy(info->pass, "anonymous");
   }

   const char *slash = strchr(start, '/');
   if (!slash)
   {
      strcpy(info->host, start);
      strcpy(info->path, "/");
      info->filename[0] = '\0';
   }
   else
   {
      size_t host_len = slash - start;
      strncpy(info->host, start, host_len);
      info->host[host_len] = '\0';

      strcpy(info->path, slash);

      const char *last_slash = strrchr(info->path, '/');
      if (last_slash && *(last_slash + 1) != '\0')
      {
         strcpy(info->filename, last_slash + 1);
      }
   }

   struct hostent *h = gethostbyname(info->host);
   if (h == NULL)
   {
      return -1;
   }

   const char *addr = inet_ntoa(*((struct in_addr *)h->h_addr));
   strcpy(info->ip, addr);

   return 0;
}

int get_port(const url_info *info)
{
   if (strcmp(info->protocol, "ftp") == 0)
   {
      return FTP_SERVER_PORT;
   }
   else if (strcmp(info->protocol, "http") == 0)
   {
      return HTTP_PORT;
   }
   else
   {
      return -1;
   }
}

int connect_socket(const char *ip, int port)
{
   int sockfd;
   struct sockaddr_in server_addr;

   bzero((char *)&server_addr, sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = inet_addr(ip);
   server_addr.sin_port = htons(port);

   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
   {
      fprintf(stderr, "socket() error\n");
      return -1;
   }

   if (connect(sockfd,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr)) < 0)
   {
      fprintf(stderr, "connect() error\n");
      return -1;
   }

   return sockfd;
}

int authenticate_ftp(const int socket, const char *user, const char *pass)
{
   char user_cmd[256];
   char pass_cmd[256];
   char response[MAX_RESPONSE_SIZE];

   sprintf(user_cmd, "USER %s\r\n", user);
   sprintf(pass_cmd, "PASS %s\r\n", pass);

   if (write(socket, user_cmd, strlen(user_cmd)) < 0)
   {
      fprintf(stderr, "Error: Failed to send USER command\n");
      return -1;
   }

   int code = read_reply(socket, response);
   printf("USER Response (%d):\n%s\n", code, response);

   if (code != 331)
   {
      fprintf(stderr, "Error: USER command failed with code %d\n", code);
      return -1;
   }

   if (write(socket, pass_cmd, strlen(pass_cmd)) < 0)
   {
      fprintf(stderr, "Error: Failed to send PASS command\n");
      return -1;
   }

   code = read_reply(socket, response);
   printf("PASS Response (%d):\n%s\n", code, response);

   return code;
}

int enter_passive_mode(const int socket, char *ip, int *port)
{
   char response[MAX_RESPONSE_SIZE];
   const char *pasv_cmd = "PASV\r\n";

   if (write(socket, pasv_cmd, strlen(pasv_cmd)) < 0)
   {
      fprintf(stderr, "Error: Failed to send PASV command\n");
      return -1;
   }
   int code = read_reply(socket, response);
   printf("PASV Response (%d):\n%s\n", code, response);

   if (code != 227)
   {
      fprintf(stderr, "Error: PASV command failed with code %d\n", code);
      return -1;
   }
   int h1, h2, h3, h4, p1, p2;
   char *p1_start = strchr(response, '(');
   char *p2_end = strchr(response, ')');
   if (!p1_start || !p2_end)
   {
      fprintf(stderr, "Error: PASV response parsing failed\n");
      return -1;
   }

   sscanf(p1_start + 1, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
   sprintf(ip, "%d.%d.%d.%d", h1, h2, h3, h4);
   *port = p1 * 256 + p2;

   return code;
}

int send_retr(const int socketfd, const char *filename)
{
   char retr_cmd[5 + strlen(filename) + 3];
   char response[MAX_RESPONSE_SIZE];

   sprintf(retr_cmd, "RETR %s\r\n", filename);
   if (write(socketfd, retr_cmd, strlen(retr_cmd)) < 0)
   {
      fprintf(stderr, "Error: Failed to send RETR command\n");
      return -1;
   }

   int code = read_reply(socketfd, response);
   printf("RETR Response (%d):\n%s\n", code, response);
   return code;
}

int download_file(const int socketfdA, const int socketfdB, char *filename)
{
   FILE *f = fopen(filename, "wb");
   if (f == NULL)
   {
      fprintf(stderr, "Error: Failed to open file %s for writing\n", filename);
      return -1;
   }

   char buffer[MAX_RESPONSE_SIZE];
   int bytes;
   while ((bytes = recv(socketfdB, buffer, sizeof(buffer), 0)) > 0)
   {
      fwrite(buffer, 1, bytes, f);
   }
   fclose(f);

   int code = read_reply(socketfdA, buffer);
   printf("Final Response (%d):\n%s\n", code, buffer);
   return code;
}

int read_reply(const int socket, char *buffer)
{
   memset(buffer, 0, MAX_RESPONSE_SIZE);
   int total_bytes = 0;
   int code = 0;
   int found_final = 0;

   while (!found_final)
   {
      int n = recv(socket, buffer + total_bytes, MAX_RESPONSE_SIZE - total_bytes - 1, 0);
      if (n <= 0)
      {
         buffer[0] = '\0';
         return -1;
      }

      total_bytes += n;
      buffer[total_bytes] = '\0';

      char *line = buffer;
      while (line < buffer + total_bytes)
      {
         if (isdigit(line[0]) && isdigit(line[1]) && isdigit(line[2]))
         {
            if (line[3] == ' ')
            {
               code = atoi(line);
               found_final = 1;
               break;
            }
         }
         char *next_line = strchr(line, '\n');
         if (next_line == NULL)
         {
            break;
         }
         line = next_line + 1;
      }
   }

   return code;
}

int main(int argc, char **argv)
{
   if (argc != 2)
   {
      fprintf(stderr, "Usage: %s <URL of file to download>\n", argv[0]);
      printf("URL syntax: ftp://[<user>:<password>@]<host>/<url-path>\n");
      exit(-1);
   }

   url_info info;

   memset(&info, 0, sizeof(url_info));

   if (parse_url(argv[1], &info) != 0)
   {
      fprintf(stderr, "Error: Failed to parse URL\n");
      exit(-1);
   }

   printf("=== URL Parsing Results ===\n");
   printf("Protocol: %s\n", info.protocol);
   printf("User:     %s\n", info.user);
   printf("Password: %s\n", info.pass);
   printf("Host:     %s\n", info.host);
   printf("IP:       %s\n", info.ip);
   printf("Path:     %s\n", info.path);
   printf("Filename: %s\n", info.filename);
   printf("===========================\n");

   char response[MAX_RESPONSE_SIZE];
   int sockfdA = connect_socket(info.ip, get_port(&info));
   if (sockfdA < 0)
   {
      fprintf(stderr, "Error: Failed to connect to %s:%d\n", info.ip, get_port(&info));
      exit(-1);
   }

   int reply_code = read_reply(sockfdA, response);
   if (reply_code == -1)
   {
      fprintf(stderr, "Error: Failed to read server reply\n");
      close(sockfdA);
      exit(-1);
   }
   printf("Server Reply (%d): \n%s", reply_code, response);

   if (authenticate_ftp(sockfdA, info.user, info.pass) != 230)
   {
      fprintf(stderr, "Error: Authentication failed\n");
      close(sockfdA);
      exit(-1);
   }

   int data_port;
   char data_ip[16];
   if (enter_passive_mode(sockfdA, data_ip, &data_port) != 227)
   {
      fprintf(stderr, "Error: Entering passive mode failed\n");
      close(sockfdA);
      exit(-1);
   }

   int sockfdB = connect_socket(data_ip, data_port);
   if (sockfdB < 0)
   {
      fprintf(stderr, "Error: Failed to connect to data port %s:%d\n", data_ip, data_port);
      close(sockfdA);
      exit(-1);
   }

   if (send_retr(sockfdA, info.path) != 150)
   {
      fprintf(stderr, "Error: RETR command failed\n");
      close(sockfdA);
      close(sockfdB);
      exit(-1);
   }

   if (download_file(sockfdA, sockfdB, info.filename) != 226)
   {
      fprintf(stderr, "Error: File download failed\n");
      close(sockfdA);
      close(sockfdB);
      exit(-1);
   }

   close(sockfdA);
   close(sockfdB);
   return 0;
}