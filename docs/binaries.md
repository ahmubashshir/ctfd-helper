# Helper Binaries

This project provides three statically compiled helper binaries for use in CTFd container-based challenges. Each binary is designed to simplify common tasks in challenge deployment and orchestration.

## `/bin/init`

See [tasks.md](./tasks.md)

## `/bin/tcpxy`

Use this to expose your challenge binary as a network service.

```
/bin/tcpxy <challenge> [optional args]
```

---

## `/bin/flag`

Use this to fetch flag from CTFd instance and write it to `flag.txt`.

Required environment variables that you must set:
- SERVER: IP of CTFd API instance.
- PORT: Port of CTFd API instance (optional, default is 9512).
- FLAG: Flag prefix.

CTFd container plugin sets these environment variables, which are also required:
- CHALLENGE_ID
- TEAM_ID
- USER_ID

---

For more details, see the main [README.md](../README.md) and [./tasks.md](tasks.md).

For a minimal, deployable example challenge using these helpers, see [./example/](example/).
