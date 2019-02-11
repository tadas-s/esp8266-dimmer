#!/usr/bin/env python
import argparse
import random
import time

from pythonosc import osc_message_builder
from pythonosc import udp_client

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", default="127.0.0.1",
                        help="The ip of the OSC server")
    parser.add_argument("--port", type=int, default=8000,
                        help="The port the OSC server is listening on")
    args = parser.parse_args()

    client = udp_client.SimpleUDPClient(args.ip, args.port, allow_broadcast=True)

    for r in range(100):
        client.send_message("/fade/r", r * (1.0 / 100))
        time.sleep(0.04)
    for r in reversed(range(100)):
        client.send_message("/fade/r", r * (1.0 / 100))
        time.sleep(0.04)

    for r in range(100):
        client.send_message("/fade/g", r * (1.0 / 100))
        time.sleep(0.04)
    for r in reversed(range(100)):
        client.send_message("/fade/g", r * (1.0 / 100))
        time.sleep(0.04)

    for r in range(100):
        client.send_message("/fade/b", r * (1.0 / 100))
        time.sleep(0.04)
    for r in reversed(range(100)):
        client.send_message("/fade/b", r * (1.0 / 100))
        time.sleep(0.04)

    for r in range(100):
        client.send_message("/fade/w", r * (1.0 / 100))
        time.sleep(0.04)
    for r in reversed(range(100)):
        client.send_message("/fade/w", r * (1.0 / 100))
        time.sleep(0.04)
