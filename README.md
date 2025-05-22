# simple-ftp

## Compile

```bash
# Compile the project
xmake
```

## Run

```bash
# Run the server
env FTP_USERNAME="anonymous" FTP_PASSWORD="anonymous" xmake run simple-ftp-server --port 8080
```

```bash
# Run the client
xmake run simple-ftp-client --port 8080
```