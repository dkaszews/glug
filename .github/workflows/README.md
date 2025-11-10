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

# Known issues on Windows

- `xmake` has issues building anything
- `vim` completely hangs due to unconnected stdin and stdout, use `nano` instead

