"""Restart the clock over HTTP and capture the serial startup log."""
import argparse
import http.client
import pathlib
import time

import serial


def open_serial(name):
    port = serial.Serial(port=None, baudrate=115200, timeout=0.2)
    port.dtr = False
    port.rts = False
    port.port = name
    port.open()
    port.write(b"\n")
    return port


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--host", required=True)
    parser.add_argument("--seconds", type=int, default=45)
    parser.add_argument("--log-file", type=pathlib.Path, default=pathlib.Path("artifacts/reboot.log"))
    args = parser.parse_args()
    args.log_file.parent.mkdir(parents=True, exist_ok=True)
    port = open_serial(args.port)
    port.reset_input_buffer()
    connection = http.client.HTTPConnection(args.host, timeout=15)
    connection.request("POST", "/restart")
    response = connection.getresponse()
    print(f"Restart HTTP status: {response.status}", flush=True)
    response.read()
    connection.close()
    deadline = time.monotonic() + args.seconds
    output = bytearray()
    while time.monotonic() < deadline:
        try:
            if port is None:
                port = open_serial(args.port)
            chunk = port.read(port.in_waiting or 1)
            if chunk:
                output.extend(chunk)
                if b"[Mario] Clockface started." in output:
                    # Allow the line to finish.
                    output.extend(port.read(512))
                    break
        except (serial.SerialException, OSError):
            if port is not None:
                try:
                    port.close()
                except OSError:
                    pass
            port = None
            time.sleep(0.2)
    if port is not None:
        port.close()
    args.log_file.write_bytes(output)
    text = output.decode("utf-8", errors="replace")
    for line in text.splitlines():
        if any(marker in line for marker in ("[Mario-S3]", "[Mario]", "[Time]", "[HUB75]", "[WiFi]")):
            print(line, flush=True)
    if "[Mario] Clockface started." not in text:
        raise SystemExit("Clockface startup was not observed; inspect local reboot log")
    if "1970" in text:
        raise SystemExit("Clockface started, but time synchronization has not completed")


if __name__ == "__main__":
    main()
