FROM debian:buster

RUN apt-get update && apt-get install -y --no-install-recommends \
        qemu-system-x86 socat \
 && rm -rf /var/lib/apt/lists/

RUN useradd --create-home --shell /bin/bash keap
WORKDIR /home/keap

COPY deploy/run.sh share/bzImage share/rootfs.cpio.gz /home/keap/

RUN chmod 555 /home/keap && \
    chown -R root:root /home/keap/ && \
    chmod 000 /home/keap/* && \
    chmod 555 /home/keap/run.sh && \
    chmod 444 /home/keap/bzImage && \
    chmod 444 /home/keap/rootfs.cpio.gz

EXPOSE 1337

USER keap
RUN ! find / -writable -or -user $(id -un) -or -group $(id -Gn|sed -e 's/ / -or -group /g') 2> /dev/null | grep -Ev -m 1 '^(/dev/|/run/|/proc/|/sys/|/tmp|/var/tmp|/var/lock)'

USER keap

CMD socat -T 300 \
    TCP-LISTEN:1337,nodelay,reuseaddr,fork \
    EXEC:"stdbuf -i0 -o0 -e0 /home/keap/run.sh",stderr,user=keap,group=keap

