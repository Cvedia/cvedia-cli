# Installation:

To create a Docker container based on the latest cvedia-cli source run the command:
```bash
docker build -t cvedia-cli ./
```

# Usage:

To run a dockerized cvedia command, replace 'cvedia' from the cvedia.com explorer with the following:
```bash
docker run --rm --net=host --volume /data:/data cvedia-cli [params]
```
Example:
```bash
docker run --rm --net=host --volume /data:/data cvedia-cli  -j CVT2CUg5M7RwcOXsz8UcQZipQbMPMQDz -b 500 -o CSV
```

For convenience you could add an alias as the following:
```bash
alias cvedia='docker run --rm --net=host --volume /data:/data cvedia-cli'
```
Then you can use the command from cvedia.com explorer directly by copy-paste.
NOTE: This alias will conflict cvedia compiled and installed outside docker.

Example:
```bash
alias cvedia='docker run --rm --net=host --volume /data:/data cvedia-cli'
cvedia -j CVT2CUg5M7RwcOXsz8UcQZipQbMPMQDz -b 500 -o CSV
```
