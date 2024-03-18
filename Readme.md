# eat-ram

Eat RAM to trigger low memory/OOM.

## Building

### Standard system build

To build using the standard system compiler:

    make eat-ram

### Multi-platform build using Cosmopolitan Libc

Install Cosmopolitan Libc:

    mkdir -p ~/cosmocc
    cd ~/cosmocc
    wget https://cosmo.zip/pub/cosmocc/cosmocc.zip
    unzip cosmocc.zip

Build:

    make eat-ram.com


