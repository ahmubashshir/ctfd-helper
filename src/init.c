#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
char** setup_temp_env()
{
	// First, count the number of environment variables
	size_t env_count = 0;
	while (environ[env_count]) {
		env_count++;
	}

	// Allocate memory for the env array (plus 1 for NULL terminator)
	char **env = malloc(sizeof(char *) * (env_count + 1));
	if (!env) {
		perror("malloc failed for env array");
		return NULL;
	}

	// Copy each environment variable using strdup
	for (size_t i = 0; i < env_count; i++) {
		env[i] = strdup(environ[i]);
		if (!env[i]) {
			perror("strdup failed for environment variable");
			// Free already allocated memory if strdup fails
			for (size_t j = 0; j < i; j++) {
				free(env[j]);
			}
			free(env);
			return NULL;
		}
	}

	// Null-terminate the new environment array
	env[env_count] = NULL;
	return env;
}

void set_temp_env(char ***temp_env, const char *var, const char *value)
{
	// Ensure temp_env is initialized
	if (*temp_env == NULL) {
		*temp_env = setup_temp_env();
	}

	// Find the size of the current temp_env array
	size_t i = 0;
	while ((*temp_env)[i]) {
		i++;
	}

	// Allocate space for a new environment variable
	*temp_env = realloc(*temp_env, sizeof(char *) * (i + 2));

	// Allocate space for the new variable (e.g., VAR=value)
	size_t len = strlen(var) + strlen(value) + 2;  // +2 for '=' and '\0'
	(*temp_env)[i] = malloc(len);
	snprintf((*temp_env)[i], len, "%s=%s", var, value);

	// Null-terminate the environment array
	(*temp_env)[i + 1] = NULL;
}

void unset_temp_env(char ***temp_env, const char *var)
{
	if (*temp_env == NULL) return;  // No temp_env to unset from

	size_t i = 0;
	size_t len = strlen(var);
	while ((*temp_env)[i]) {
		// Check if the current variable matches the one to unset
		if (strncmp((*temp_env)[i], var, len) == 0 && (*temp_env)[i][len] == '=') {
			// Free the memory for the variable
			free((*temp_env)[i]);

			// Shift the remaining elements down
			while ((*temp_env)[i + 1]) {
				(*temp_env)[i] = (*temp_env)[i + 1];
				i++;
			}

			// Null-terminate the array
			(*temp_env)[i] = NULL;
			break;
		}
		i++;
	}
}

int run(char* const* args, char* const* envs)
{

	size_t i = 0;
	printf("init$ ");
	if(envs != NULL) printf("tmpenv ");
	do {
		printf("%s%c", args[i], (args[i + 1] == NULL)?'\n':' ');
	} while(args[++ i] != NULL);

	pid_t pid = fork();  // Create a new child process
	if (pid < 0) {
		perror("fork failed");
		return 1;
	}

	if (pid == 0) {
		if (execve(args[0], args, (envs == NULL)?environ:envs) == -1) exit(1);
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

		if (*s == '@' && s[1] == '-') { // handle @-VAR  (unset)
			char *var = s + 2;
			if (*var == 0) {
				fprintf(stderr, "invalid unset syntax: %s\n", s);
				continue;
			}
			unsetenv(var);
			printf("init$ delenv %s\n", var);
			continue;
		} else if (*s == '@') { // handle @VAR=value...
			char *eq = strchr(s, '=');
			char *cl = strchr(s, ':');
			char *value = NULL;
			if (!(eq || cl)) {
				fprintf(stderr, "invalid env syntax: %s\n", s);
				continue;
			} else if (eq) {
				*eq = 0;
				value = eq + 1; // everything after =
			} else if (cl) {
				*cl = 0;
				FILE* fp = fopen(cl + 1, "r");
				if (!fp) {
					perror("failed to open file for @VAR:path");
					continue;
				}

				fseek(fp, 0, SEEK_END);
				size_t len = ftell(fp);
				fseek(fp, 0, SEEK_SET);

				value = malloc(len + 1);
				if(value == NULL) {
					perror("failed to allocate memory env file");
					fclose(fp);
					continue;
				}
				fread(value, 1, len, fp);
				fclose(fp);
				remove(cl + 1);
				value[len] = 0;
			}

			char *var = s + 1;
			setenv(var, value, 1);
			if(cl) {
				printf("init$ readenv %s < %s\n", var, cl + 1);
				free(value);
			} else {
				printf("init$ setenv %s = \"%s\"\n", var, value);
			}
			continue;
		}

		// Create a temporary environment for the current command
		char **temp_env = NULL;


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
			if (token[0] == '$' && temp_env == NULL)
				temp_env = setup_temp_env();
			if(token[0] == '$' && token[1] == '-') {
				if (temp_env != NULL)
					unset_temp_env(&temp_env, token + 2);
				free(token);
			} else if(token[0] == '$') {
				if (temp_env != NULL) {
					char *eq = strchr(token, '=');
					if (eq) {
						*eq = 0;
						char *var = token + 1;     // skip '@'
						char *value = eq + 1;  // everything after '='
						set_temp_env(&temp_env, var, value);
					} else {
						fprintf(stderr, "invalid env syntax: %s\n", s);
					}
				}
				free(token);
			} else {
				args = realloc(args, sizeof(char *) * (argc + 2));
				args[argc++] = token;
				args[argc] = NULL;
			}
		}

		if (argc > 0)
			ret = run(args, temp_env);

		for (size_t i = 0; i < argc; i++)
			free(args[i]);
		free(args);
		if(temp_env != NULL) {
			for(size_t i = 0; temp_env[i] != NULL; i++)
				free(temp_env[i]);
			free(temp_env);
		}

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
