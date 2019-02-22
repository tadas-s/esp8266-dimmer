#!/usr/bin/env python
import argparse
import random
import time

from pythonosc import osc_message_builder
from pythonosc import udp_client

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", default="192.168.0.255",
                        help="The broadcast address")
    parser.add_argument("--port", type=int, default=8000,
                        help="The port the OSC server is listening on")
    parser.add_argument("--id", type=int, default=0,
                        help="Device ID")
    args = parser.parse_args()

    client = udp_client.SimpleUDPClient(args.ip, args.port, allow_broadcast=True)

    for r in [0.0, 0.33, 0.66, 1.0]:
        for g in [0.0, 0.33, 0.66, 1.0]:
            for b in [0.0, 0.33, 0.66, 1.0]:
                for w in [0.0, 0.33, 0.66, 1.0]:
                    client.send_message(f"/{args.id}/rgbw", [r, g, b, w])
                    time.sleep(0.1)

