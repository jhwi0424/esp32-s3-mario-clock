"""Provision Clockwise over Improv Serial without compiling Wi-Fi secrets in firmware."""
import argparse
import json
import pathlib
import time

import serial

MAGIC = b"IMPROV"


def rpc(command, strings=()):
    args = b"".join(bytes([len(value.encode("utf-8"))]) + value.encode("utf-8") for value in strings)
    payload = bytes([command, len(args)]) + args
    frame = MAGIC + bytes([1, 3, len(payload)]) + payload
    return frame + bytes([sum(frame) & 0xFF])


def receive_frames(buffer):
    frames = []
    while True:
        start = buffer.find(MAGIC)
        if start < 0:
            if len(buffer) > 6:
                del buffer[:-6]
            break
        if start:
            del buffer[:start]
        if len(buffer) < 9:
            break
        size = 10 + buffer[8]
        if len(buffer) < size:
            break
        frame = bytes(buffer[:size])
        if frame[6] == 1 and (sum(frame[:-1]) & 0xFF) == frame[-1]:
            frames.append((frame[7], frame[9:-1]))
            del buffer[:size]
        else:
            del buffer[0]
    return frames


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--credentials-file", type=pathlib.Path, required=True)
    parser.add_argument("--timeout", type=int, default=90)
    parser.add_argument("--log-file", type=pathlib.Path, default=pathlib.Path("artifacts/provision.log"))
    args = parser.parse_args()
    credentials = json.loads(args.credentials_file.read_text(encoding="utf-8-sig"))
    ssid, password = credentials["ssid"], credentials["password"]
    if not 1 <= len(ssid.encode("utf-8")) <= 32 or len(password.encode("utf-8")) > 63:
        raise SystemExit("Invalid Wi-Fi SSID or password length")
    args.log_file.parent.mkdir(parents=True, exist_ok=True)
    deadline = time.monotonic() + args.timeout
    buffer = bytearray()
    provisioned = False
    booted = False
    text_tail = ""
    port = serial.Serial(port=None, baudrate=115200, timeout=0.2)
    port.dtr = False
    port.rts = False
    port.port = args.port
    port.open()
    with args.log_file.open("w", encoding="utf-8") as log, port:
        # Do not assert RTS/DTR: on native USB they can reset the ESP32-S3.
        time.sleep(0.5)
        # The upstream parser leaves its position at 1 after an RPC; a leading
        # newline brings it back to the frame boundary before a later command.
        port.write(b"\n" + rpc(1, (ssid, password)))
        print(f"Wi-Fi configuration sent on {args.port}; awaiting device confirmation.", flush=True)
        while time.monotonic() < deadline:
            chunk = port.read(port.in_waiting or 1)
            if not chunk:
                continue
            text = chunk.decode("utf-8", errors="replace").replace(password, "[REDACTED]")
            text_tail = (text_tail + text)[-4096:]
            log.write(text)
            log.flush()
            buffer.extend(chunk)
            if "[Mario] Clockface started." in text_tail:
                booted = True
            for frame_type, payload in receive_frames(buffer):
                if frame_type == 1 and payload:
                    print(f"Improv state: {payload[0]}", flush=True)
                    if payload[0] == 4:
                        provisioned = True
                elif frame_type == 2 and payload and payload[0] != 0:
                    raise SystemExit(f"Improv provisioning error: {payload[0]}; see local log")
                elif frame_type == 4 and payload:
                    print("Improv RPC response received.", flush=True)
                    if len(payload) >= 3 and payload[0] in (1, 2):
                        url = payload[3:3 + payload[2]].decode("utf-8", errors="replace")
                        if url.startswith("http://"):
                            print(f"Clock settings: {url}", flush=True)
            if provisioned and booted:
                break
        print(f"Provisioned={provisioned}; clockface startup observed={booted}; log={args.log_file}", flush=True)
    if not provisioned:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
