import argparse
import subprocess
import time
import random
import shlex

import sys
import os

RANDOM_LIMIT = 1000
SEED = 123456789
PERF_COMMAND = 'sudo perf record -o perf.data -p '
GRAPH_COMMAND = 'sudo perf script -i perf.data | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > graph.svg'
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.PIPE)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


server = run(start_server())
perf = run(PERF_COMMAND + str(server.pid))
make_shots()
stop(server)
stop(perf, True)

try:
    perf = subprocess.Popen(
        ['sudo', 'perf', 'script', '-i', 'perf.data'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding='utf-8'
    )

    collapse = subprocess.Popen(
        ['./FlameGraph/stackcollapse-perf.pl'],
        stdin=perf.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding='utf-8'
    )
    perf.stdout.close()

    with open('graph.svg', 'wb') as f:
        flame = subprocess.Popen(
            ['./FlameGraph/flamegraph.pl'],
            stdin=collapse.stdout,
            stdout=f,
            stderr=subprocess.PIPE,
            encoding='utf-8'
        )
        collapse.stdout.close()

        _, flame_err = flame.communicate()
        if flame.returncode != 0:
            print(f"Flamegraph error: {flame_err}")
    print("Success")

except Exception as e:
    print(f"Error: {str(e)}")

graph = subprocess.run(GRAPH_COMMAND, stderr=subprocess.PIPE, shell=True)
with open('graph.svg', 'r', encoding='utf-8') as file:
    print(file.read())
print('Job done')
