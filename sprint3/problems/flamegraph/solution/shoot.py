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

SHOOT_COUNT = 1000
COOLDOWN = 0.05


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
perf = subprocess.Popen(
            ['sudo', 'perf', 'script', '-i', 'perf.data'],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        # иру 2: stackcollapse
collapse = subprocess.Popen(
            ['./FlameGraph/stackcollapse-perf.pl'],
            stdin=perf.stdout,
            #stdout=subprocess.PIPE,
            stdout=sys.stdout,
            stderr=subprocess.PIPE
        )
perf.stdout.close()
graph = subprocess.run(GRAPH_COMMAND, stderr=subprocess.PIPE, shell=True)
with open('graph.svg', 'r', encoding='utf-8') as file:
    print(file.read())
print('Job done')
