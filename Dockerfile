FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=America/La_Paz \
    OMPI_ALLOW_RUN_AS_ROOT=1 \
    OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

RUN apt-get update && apt-get install -y --no-install-recommends \
        openssh-server \
        openssh-client \
        openmpi-bin \
        libopenmpi-dev \
        build-essential \
        make \
        procps \
        ca-certificates \
        iputils-ping \
        net-tools \
        vim-tiny \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /run/sshd /root/.ssh /app/bin /app/data /app/src /app/scripts /app/config \
    && chmod 700 /root/.ssh

RUN sed -ri 's/^#?PermitRootLogin[[:space:]]+.*/PermitRootLogin yes/' /etc/ssh/sshd_config \
    && sed -ri 's/^#?PubkeyAuthentication[[:space:]]+.*/PubkeyAuthentication yes/' /etc/ssh/sshd_config \
    && sed -ri 's/^#?PasswordAuthentication[[:space:]]+.*/PasswordAuthentication no/' /etc/ssh/sshd_config \
    && (grep -q '^UseDNS' /etc/ssh/sshd_config || echo 'UseDNS no' >> /etc/ssh/sshd_config)

COPY Makefile /app/Makefile

COPY scripts/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

WORKDIR /app
EXPOSE 22

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["bash", "-lc", "tail -f /dev/null"]
