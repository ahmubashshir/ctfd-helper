// #triplet x86_64 linux musl
// @-static
// #strip

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

// Function to extract flag from the response
int get_flag(
    const char *challenge_id,
    const char *team_id,
    const char *user_id,
    const char *flag_endpoint_host,
    const char* flagfmt,
    char* buf,
    const char* server,
    unsigned port
)
{
	if (challenge_id == NULL || team_id == NULL || user_id == NULL) {
		fprintf(stderr, "CHALLENGE_ID, TEAM_ID, or USER_ID is empty.\n");
		return 1;
	}

	int sockfd;
	struct sockaddr_in server_addr;
	char request[512];
	char response[1024];  // Buffer to store incoming response

	// Create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Socket creation failed");
		return 1;
	}

	// Set server address
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, server, &server_addr.sin_addr) <= 0) {
		perror("inet_pton failed");
		return 1;
	}

	// Connect to server
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connection failed");
		close(sockfd);
		return 1;
	}

	// Prepare HTTP request
	snprintf(request, sizeof(request),
	         "GET /flag?chal_id=%s&team_id=%s&user_id=%s HTTP/1.1\r\nHost: %s\r\n\r\n",
	         challenge_id, team_id, user_id, (flag_endpoint_host == NULL ? "localhost" : flag_endpoint_host));

	// Send HTTP request
	if (send(sockfd, request, strlen(request), 0) < 0) {
		perror("Send failed");
		close(sockfd);
		return 1;
	}

	// Read the response line by line
	ssize_t received;
	int flag_found = 0;
	while ((received = recv(sockfd, response, sizeof(response) - 1, 0)) > 0) {
		response[received] = '\0';  // Null-terminate the response

		// Process the response line by line
		char *line = strtok(response, "\r\n");  // Split by lines
		while (line != NULL) {
			if (strstr(line, flagfmt)) {
				// If the line contains the flag format, print the flag
				printf("Fetched flag: %s\n", line);
				strncpy(buf, line, 512);
				flag_found = 1;
				break;
			}
			line = strtok(NULL, "\r\n");  // Get next line
		}

		if (flag_found) {
			break;
		}
	}

	if (!flag_found) {
		fprintf(stderr, "Failed to fetch flag.\n");
		close(sockfd);
		return 1;
	}

	close(sockfd);
	return 0;
}

int main()
{
	const char *challenge_id = getenv("CHALLENGE_ID");
	const char *team_id = getenv("TEAM_ID");
	const char *user_id = getenv("USER_ID");
	const char *flag_endpoint_host = getenv("FLAG_ENDPOINT_HOST");
	const char *server  = getenv("SERVER");
	const char *flagfmt = getenv("FLAG");
	const char *sport    = getenv("PORT");
	unsigned long port   = strtoul(sport == NULL?"9512":sport, NULL, 10);

	if (server == NULL || flagfmt == NULL) {
		printf("Unconfigured... set SERVER and FLAGFMT in environment...\n");
		return 1;
	}
	if(sport == NULL) printf("PORT not set, using default: %lu\n", port);


	char flag[512] = "";

	if (get_flag(challenge_id, team_id, user_id, flag_endpoint_host, flagfmt, flag, server, port) != 0) {
		return 1;
	}

	// Simulate the flag removal process
	if (remove("flag.txt") == 0) {
		printf("Successfully removed flag\n");
	} else if (access("flag.txt", F_OK) == 0) {
		perror("Failed to remove flag.txt");
	}

	FILE *flag_file = fopen("flag.txt", "w");
	if (flag_file) {
		fprintf(flag_file, "%s\n", flag);
		fclose(flag_file);
	} else {
		perror("Failed to open flag.txt");
	}

	return 0;
}
