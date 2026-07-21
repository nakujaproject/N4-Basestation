"""
N4 Base Station - Generate All Diagrams
Run this script from the diagrams/ directory to regenerate all PNGs.
"""
import subprocess
import sys
import os

SCRIPTS = [
    "system_architecture.py",
    "data_flow.py",
    "command_flow.py",
]

script_dir = os.path.dirname(os.path.abspath(__file__))

for script in SCRIPTS:
    path = os.path.join(script_dir, script)
    print(f"\n>>> Running {script} ...")
    result = subprocess.run([sys.executable, path], capture_output=True, text=True)
    if result.stdout:
        print(result.stdout.strip())
    if result.returncode != 0:
        print(f"ERROR in {script}:")
        print(result.stderr)
    else:
        print(f"    OK")

print("\nAll diagrams generated successfully.")
