# Need to use the just-built dev image as a base
ARG dev_image
FROM ${dev_image}
ARG lvat_package

# Install built toolchain in container
RUN --mount=type=bind,source=.,destination=/src \
    tar -C /usr/local -Jx -f /src/build/${lvat_package}.tar.xz --strip-components=1 \
        ${lvat_package}/bin \
        ${lvat_package}/include \
        ${lvat_package}/lib
