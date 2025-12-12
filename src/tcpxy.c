/*
 * #triplet x86_64 linux musl
 * @-static
 * @debug
 * @-pipe
 * @-pie
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 100

int server_fd = -1;
void handle_sigchld(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

void handle_sigint(int sig)
{
	if (server_fd != -1) {
		close(server_fd);
	}
	printf("Server shutting down...\n");
	exit(0);
}

void start_handler(int fd, char *const args[])
{
	// Fork a new process to handle the connection

	pid_t pid = fork();
	if (pid < 0) {
		perror("fork failed");
		close(fd);
		return;
	} else if (pid == 0) {
		// Redirect input/output to the client socket
		if (dup2(fd, STDIN_FILENO) < 0 || dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
			perror("dup2 failed");
			close(fd);
			exit(1);
		}

		if (execv(args[0], args) < 0) {
			perror("execvp failed");
			close(fd);
			exit(1);
		}
	}
}

void print_client_addr(struct sockaddr_in *client_addr)
{
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof(client_ip));
	printf("Client connected -> %s:%d\n", client_ip, ntohs(client_addr->sin_port));
}

// Function to handle the TCP listener and execute the given command
void start_server(int port, char *const cmdline[])
{
	int client_fd;
	struct sockaddr_in server_addr, client_addr;
	socklen_t client_len = sizeof(client_addr);

	// Create the socket
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket creation failed");
		exit(1);
	}

	// Set socket options (reuse address)
	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt failed");
		close(server_fd);
		exit(1);
	}

	// Set the socket to CLOEXEC so it will close automatically on exec
	if (fcntl(server_fd, F_SETFD, FD_CLOEXEC) < 0) {
		perror("fcntl failed");
		close(server_fd);
		exit(1);
	}

	// Define the server address with INADDR_ANY to accept connections on all interfaces
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all available interfaces
	server_addr.sin_port = htons(port);

	// Bind the socket to the address and port
	if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		perror("bind failed");
		close(server_fd);
		exit(1);
	}

	// Start listening for connections
	if (listen(server_fd, 5) < 0) {
		perror("listen failed");
		close(server_fd);
		exit(1);
	}

	printf("Listening on port %d...\n", port);
	printf("Handler: {");
	{
		size_t i = 0;
		while(cmdline[i] != NULL) {
			printf("\"%s\"%s", cmdline[i], (cmdline[i + 1]==NULL)?"}\n":", ");
			i++;
		}
	}



	while (1) {
		// Accept incoming connection
		client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
		if (client_fd < 0) {
			perror("accept failed");
			continue;  // Continue to listen for new connections
		}

		print_client_addr(&client_addr);
		start_handler(client_fd, cmdline);
		close(client_fd);
	}

	close(server_fd);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <path> [args ...]\n", argv[0]);
		exit(1);
	}

	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	signal(SIGCHLD, handle_sigchld);

	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
	setvbuf(stdin, NULL, _IONBF, 0);

	int port = 20240;
	char *cmdline[argc - 1];
	for(size_t i = 0; i < (argc - 1); i++) {
		cmdline[i] = argv[i + 1];
		cmdline[i + 1] = NULL;
	}

	start_server(port, cmdline);

	return 0;
}
