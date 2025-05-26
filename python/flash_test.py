import subprocess
from enum import Enum
import os

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
BIN_PATH = os.path.join(ROOT_DIR, "build/c_project")

class CMD(Enum):
    INIT = 0
    WRITE = 1
    UPDATE_DATA = 2
    INIT_TEST_DATA = 3
    
tests = [[CMD.INIT_TEST_DATA, CMD.INIT, CMD.WRITE]]

if __name__ == "__main__":

    def parse_cfg_symbols(filename):
        cfg_dict = {}
        with open(f'../{filename}', 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('#define CFG_'):
                    parts = line.split(None, 3)
                    if len(parts) == 3:
                        key, value = parts[1:]
                        cfg_dict[key] = value
        return cfg_dict

    cfg_symbols = parse_cfg_symbols('flash_conf.h')

    for test_idx, test in enumerate(tests):

        try:

            print('running..')

            args = [str(len(test))] + [str(op.value) for op in test]
            #print(args)

            result = subprocess.run(
                [BIN_PATH] + args,
                cwd=ROOT_DIR,
                capture_output=True,
                text=True,
                check=True
            )

            print(result.stdout)

        except subprocess.CalledProcessError as e:
            print(f'ERROR: Exception occured during test {test_idx}')
            print(f'ERROR: Exception details: {e}')
