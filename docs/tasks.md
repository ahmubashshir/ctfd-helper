# Task File Syntax

The `/tasks` file controls environment setup and command execution for the CTFd helper container. It is executed by `/bin/init` at container startup.

**For a minimal deployable example, see [`docs/example/`](./example/).**

## Syntax Rules

- Lines starting with `#` are comments.
- Blank lines are ignored.
- All commands must use absolute/relative paths.

### Environment Variable Management

- `@VAR=VALUE`  
  Set an environment variable `VAR` to `VALUE` (exported for subsequent commands).

- `@-VAR`  
  Unset the environment variable `VAR`.

- `@READ:/path/to/file`  
  Read the contents of the specified file and set it as the value for the variable named in the line (see below).
  The file is removed after reading.

### Command Execution
- Command lines (not starting with `@` or `#`) are executed as shell commands.
- You can use temporary environment variables for a single command:
  - `$VAR=VALUE` before a command sets `VAR` only for that command.
  - `$-VAR` before a command unsets `VAR` only for that command.

## Example

```
# Set up environment
@FLAG=CTF{example_flag}
@CHALLENGE_ID=123
@-OLD_ENV

# Read a secret from a file
@READ:/run/secret

# Run a command with temporary environment
$SERVER=localhost $PORT=9512 $FLAG=CTF{ /bin/flag

# Start the TCP proxy (tcpxy) to expose the challenge
/bin/tcpxy /bin/problem
```

## Notes

- All commands must use absolute paths (e.g., `/bin/problem`).
- Temporary environment variables (`$VAR=VALUE`) and unsets (`$-VAR`) only affect the command they precede.
- The `/tasks` file is intended to be simple and robust for CTF deployment automation.

## Minimal Example

See [`docs/simple-challenge/tasks`](./simple-challenge/tasks):

```
@FLAG:./flag.txt
/challenge
```

This loads the contents of `flag.txt` into the `FLAG` environment variable, then runs the `/challenge` binary.

For more details, see the main [README.md](../README.md), the [binaries documentation](./binaries.md), or the [example](./example).
