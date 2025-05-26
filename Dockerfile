FROM buildpack-deps:bookworm

RUN sed -i 's|http://deb.debian.org|https://deb.debian.org|g' \
        /etc/apt/sources.list /etc/apt/sources.list.d/*.list || true && \
    apt-get update -qq && \
    apt-get install -y --no-install-recommends cmake gdb tree && \
    rm -rf /var/lib/apt/lists/*

RUN apt-get update && \
    apt-get install -y python3 python3-pip && \
    pip3 install --no-cache-dir pipenv --break-system-packages && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /work
CMD ["bash"]
