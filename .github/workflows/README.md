# Debugging actions

If you cannot local reproduce a failure, perhaps because it only occurs on a different system, you can use [`action-tmate`](https://github.com/mxschmitt/action-tmate) to connect to the GitHub actions server.

1. Add the following step to the end of the failing job:
    ```yaml
          - uses: mxschmitt/action-tmate@v3
            if: ${{ failure() }}
            with:
              limit-access-to-actor: true
    ```
1. Optionally, comment out the timeout to avoid being kicked out prematurely
1. Push changes
1. Open the failing action, you will see repeated logs that look like:
    ```shell
    SSH: ssh p74zJTjC5zWbyTAgwsqTCB4q5@nyc1.tmate.io
    or: ssh -i <path-to-private-SSH-key> p74zJTjC5zWbyTAgwsqTCB4q5@nyc1.tmate.io
    ```
1. Use given command to connect to the server, press `q` to exit welcome prompt and enter shell
1. Test will finish when all clients disconnect

## Known issues on Windows

- `xmake` has issues building anything
- `vim` completely hangs due to unconnected stdin and stdout, use `nano` instead

# Running RISC-V binaries on QEMU

One of the ways to locally replicate effects of `uraimo/run-on-arch-action@v3` is to install Ubuntu on QEMU.
This can be done by following the [official guide](https://canonical-ubuntu-hardware-support.readthedocs-hosted.com/boards/how-to/qemu-riscv/) with small recommended modifications:

1. `qemu-efi-riscv64` may not be available on other package managers, but you can click through the link then navigate through website to download `*.deb` package, then unpack it with `ar x *.deb`, then finaly extract `*.fd` files from `data.tar.*`.
1. Backup `RISCV_VIRT_VARS.fd` as you can sometimes run into QEMU boot issues that are solveable by restoring this file to defaults.
1. In `qemu-system-riscv64` invocation from "Running via EDK II", append `-netdev user,id=net0` with `,hostfwd=tcp::50022-:22` to enable SSH connection via `ubuntu@localhost -p50022` after enabling sshd via QEMU terminal.
1. Default login and password are `ubuntu/ubuntu`, you will be prompted to change the password immediately, `ubuntu1` will be "hard" enough. Empty password will cause sshd issues.

