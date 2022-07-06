# Intro-to-Network-Programming
NYCU Intro to Network Programming 2022 Fall

### Installing docker on Mac m$1$

https://docs.docker.com/desktop/mac/apple-silicon/

After installation, typing `docker` in terminal is still not available since the command wasn't added to `$PATH`, thus run the following two commands on terminal.

https://stackoverflow.com/questions/64009138/docker-command-not-found-when-running-on-mac

```bash
# Add Visual Studio Code (code)
export PATH="$PATH:/Applications/Visual Studio Code.app/Contents/Resources/app/bin"
# Add Docker Desktop for Mac (docker)
export PATH="$PATH:/Applications/Docker.app/Contents/Resources/bin/"
```
`docker compose` : show all the operations can be done by docker compose.

`docker compose up -d` : 部署容器
