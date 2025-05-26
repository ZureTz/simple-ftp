# simple-ftp

## Compile

```bash
# Compile the project
xmake
```

## Run

Server side:

Make a new 'config.json' file:
```bash
cp config.example.json config.json

```

And edit the shared directory path and username/password in `config.json` to your desired values.

Then run the server:
```bash
xmake run simple-ftp-server --port 8080
```

Or run binary directly:
```bash
build/linux/<your-arch>/simple-ftp-server --port 8080
```

Run the client:
```bash
xmake run --workdir=received simple-ftp-client --host <server-ip> --port 8080
```

Or run binary directly:
```bash
mkdir -p received && cd received && \
    build/linux/<your-arch>/simple-ftp-client --host <server-ip> --port 8080
```