#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run(char* const* args)
{

	size_t i = 0;
	printf("init$ ");
	do {
		printf("%s%c", args[i], (args[i + 1] == NULL)?'\n':' ');
	} while(args[++ i] != NULL);

	pid_t pid = fork();  // Create a new child process
	if (pid < 0) {
		perror("fork failed");
		return 1;
	}

	if (pid == 0) {
		if (execvp(args[0], args) == -1) exit(1);
	}

	int status;
	waitpid(pid, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		fprintf(stderr, "  ==> %s returned %d\n", args[0], WEXITSTATUS(status));
		return 1;
	}

	return 0;
}


static void handle_sigchld(int sig)
{
	while (waitpid(-1, NULL, 0) > 0) {}
}

static void handle_sigint(int sig)
{
	printf("Received signal %d, forwarding...\n", sig);

	kill(0, sig);
	while (waitpid(-1, NULL, 0) > 0) {}
	exit(0);
}

int setup_tasks()
{
	const char *filename = "/tasks";
	FILE *fp = fopen(filename, "r");
	if (!fp) {
		perror("failed to open /tasks");
		return 0;
	}

	char *line = NULL;
	size_t n = 0;
	int ret = 0;

	while (getline(&line, &n, fp) != -1) {
		// strip newline
		line[strcspn(line, "\r\n")] = 0;

		// skip leading whitespace
		char *s = line;
		while (*s == ' ' || *s == '\t')
			s++;

		// ignore empty or comment lines
		if (*s == 0 || *s == '#')
			continue;

		// handle @-VAR  (unset)
		if (*s == '@' && s[1] == '-') {
			char *var = s + 2;
			if (*var == 0) {
				fprintf(stderr, "invalid unset syntax: %s\n", s);
				continue;
			}
			unsetenv(var);
			continue;
		}

		// handle @VAR=value...
		if (*s == '@') {
			char *eq = strchr(s, '=');
			if (!eq) {
				fprintf(stderr, "invalid env syntax: %s\n", s);
				continue;
			}

			*eq = 0;
			char *var = s + 1;     // skip '@'
			char *value = eq + 1;  // everything after '='

			setenv(var, value, 1);
			continue;
		}

		// otherwise treat it as a command line
		char **args = NULL;
		size_t argc = 0;
		char *p = s;

		while (*p) {
			while (*p == ' ')
				p++;
			if (*p == 0)
				break;

			char *start;
			if (*p == '"') {
				p++;
				start = p;
				while (*p && *p != '"')
					p++;
			} else {
				start = p;
				while (*p && *p != ' ')
					p++;
			}

			size_t len = p - start;
			char *token = malloc(len + 1);
			memcpy(token, start, len);
			token[len] = 0;

			if (*p == '"')
				p++;
			if (*p == ' ')
				p++;

			args = realloc(args, sizeof(char *) * (argc + 2));
			args[argc++] = token;
			args[argc] = NULL;
		}

		if (argc > 0)
			ret = run(args);

		for (size_t i = 0; i < argc; i++)
			free(args[i]);
		free(args);

		if(ret != 0) break;
	}

	free(line);
	fclose(fp);
	return ret;
}

int main()
{
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	signal(SIGCHLD, handle_sigchld);

	setvbuf(stdout, NULL, _IONBF, 0);  // Disable buffering for stdout
	setvbuf(stderr, NULL, _IONBF, 0);  // Disable buffering for stderr
	setvbuf(stdin, NULL, _IONBF, 0);   // Disable buffering for stdin


	if(setup_tasks()) printf("OK.\n");
	else printf("ERROR.\n");
	return 0;
}
