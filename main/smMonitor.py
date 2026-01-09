#!/usr/bin/env python3
"""
Smart ESP32 Monitor with trigger-based CSV logging using PTY
Supports auto-port detection
Usage: python smart_monitor.py

by Claude.ai,  prompt: "in esp32-idf version 5.4.x, is there a way to automatically capture numerical data in "idf.py monitor" into, say, a .csv file?"
followed by: "I like the | tee solution but do I still get an interactive console?    What I'd like ideally is a version of monitor where ESP32 could output a special code which would start the capture-to-file function and then end that with another code."

plus several tweaks - Claude was pretty clever on e.g. def strip_ansi(text)

"""

import os
import sys
import pty
import select
import subprocess
import re
from datetime import datetime

# Define your trigger codes
START_CAPTURE = ">>>START_LOG<<<"
END_CAPTURE = ">>>END_LOG<<<"

DEBUG = False

if DEBUG:
    # Open debug log file
    debug_log = open("monitor_debug.log", "w")

def debug(msg):
    if DEBUG:
        """Write debug messages to file with timestamp"""
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        debug_log.write(f"[{timestamp}] {msg}\n")
        debug_log.flush()
        return
    else:
        return

def strip_ansi(text):
    """Remove ANSI escape codes"""
    ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
    return ansi_escape.sub('', text)

def main():
    csv_file = None
    capture_active = False
    line_buffer = ""  # Buffer for building complete lines

    # Create a pseudo-terminal
    master, slave = pty.openpty()

    # Start idf.py monitor with auto-port detection
    cmd = ['idf.py', 'monitor']

    debug("Starting monitor with auto-port detection...")

    # Start the process with the slave end of the PTY
    process = subprocess.Popen(
        cmd,
        stdin=slave,
        stdout=slave,
        stderr=slave,
        close_fds=True
    )

    # Close slave in parent process (child has its own copy)
    os.close(slave)

    try:
        while True:
            # Check if data is available from the PTY or stdin
            ready, _, _ = select.select([master, sys.stdin], [], [], 0.1)

            # Handle data from the monitor (ESP32 output)
            if master in ready:
                try:
                    data = os.read(master, 1024)
                    if not data:
                        break

                    # Decode and display
                    text = data.decode('utf-8', errors='replace')
                    sys.stdout.write(text)
                    sys.stdout.flush()

                    # Add to line buffer
                    line_buffer += text

                    # Process only on actual newlines (\n)
                    while '\n' in line_buffer:
                        # Split on first \n only
                        line, line_buffer = line_buffer.split('\n', 1)

                        # Strip ANSI codes and carriage returns for trigger detection
                        clean_line = strip_ansi(line).replace('\r', '')

                        debug(f'Processing: {repr(clean_line[:80])}')

                        # Check for start trigger
                        if START_CAPTURE in clean_line:
                            debug(f'Found Start: {clean_line}')
                            if not capture_active:
                                timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
                                filename = f"capture_{timestamp}.csv"
                                csv_file = open(filename, 'w')
                                capture_active = True
                                # sys.stdout.write(f"\n[📊 CAPTURE STARTED: {filename}]\n")
                                # sys.stdout.flush()

                        # Check for end trigger
                        elif END_CAPTURE in clean_line:
                            debug(f'End: {clean_line}')
                            if capture_active and csv_file:
                                csv_file.close()
                                # sys.stdout.write(f"\n[✓ CAPTURE ENDED: {csv_file.name}]\n")
                                # sys.stdout.flush()
                                csv_file = None
                                capture_active = False

                        # Log data if capture is active
                        elif capture_active and csv_file:
                            # Only write non-empty lines
                            if clean_line.strip():  # Check if line has actual content
                                debug(f'got data: {repr(clean_line[:50])}')
                                # Write the clean line (without ANSI/control chars) plus newline
                                csv_file.write(clean_line + '\n')
                                csv_file.flush()
                            else:
                                debug('skipping empty line')

                except OSError:
                    break

            # Handle keyboard input (send to ESP32)
            if sys.stdin in ready:
                try:
                    user_input = os.read(sys.stdin.fileno(), 1024)
                    if user_input:
                        os.write(master, user_input)
                except OSError:
                    break

            # Check if process has exited
            if process.poll() is not None:
                break

    except KeyboardInterrupt:
        print("\n[Monitor interrupted by user]")

    finally:
        if csv_file:
            csv_file.close()
            debug(f"[File closed: {csv_file.name}]")

        os.close(master)
        process.terminate()
        process.wait()
        debug_log.close()

if __name__ == "__main__":
    main()
