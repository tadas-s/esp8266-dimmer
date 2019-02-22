#!/usr/bin/env python
import argparse
import random
import time

from pythonosc import osc_message_builder
from pythonosc import udp_client

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", help="The ip of device being configured")
    parser.add_argument("--port", type=int, default=8000,
                        help="The port the OSC server is listening on")
    parser.add_argument("--id", type=int, default=0,
                        help="Device id")
    args = parser.parse_args()

    client = udp_client.SimpleUDPClient(args.ip, args.port)
    client.send_message("/settings/id", [args.id])