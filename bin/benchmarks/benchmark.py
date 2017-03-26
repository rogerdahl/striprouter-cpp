#!/usr/bin/env python

import os
import subprocess
import time

import cpuinfo # pip install py-cpuinfo


N_REPEAT = 10
BENCHMARKS_FILE_PATH = './benchmarks.txt'
REL_BIN_PATH = '../striprouter'


def main():
    cpu_info_dict = cpuinfo.get_cpu_info()

    start_time = time.time()

    for i in range(N_REPEAT):
        print '{} / {}'.format(i + 1, N_REPEAT)
        run()

    end_time = time.time()
    elapsed_sec = end_time - start_time
    avg_sec = elapsed_sec / N_REPEAT

    result_str = 'avg_sec={:.2f} repeats={} cpu="{}" actual_hz="{}"'.format(
        avg_sec, N_REPEAT, cpu_info_dict['brand'], cpu_info_dict['hz_actual_raw'][0]
    )

    print result_str

    with open(BENCHMARKS_FILE_PATH, 'aw') as f:
        f.write(result_str + '\n')

    print 'Result added to benchmark file. path="{}"'.format(BENCHMARKS_FILE_PATH)


def run():
    rel_dir_path, bin_name = os.path.split(REL_BIN_PATH)
    rel_this_dir_path = os.path.dirname(__file__)
    abs_dir_path = os.path.abspath(os.path.join(rel_this_dir_path, rel_dir_path))

    subprocess.call([
        './' + bin_name,
        '--nogui', '--exitafter', '1000', '--checkpoint', '100',
        '--circuit', './circuits/benchmark.circuit',
    ], cwd=abs_dir_path)


if __name__ == '__main__':
    main()
