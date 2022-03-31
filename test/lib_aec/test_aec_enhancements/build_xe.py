# Copyright 2022 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
import configparser
import subprocess
import glob

parser = configparser.ConfigParser()
parser.read("parameters.cfg")

build_config = f"{str(parser.get('XCBuild', 'threads'))} {str(parser.get('Config', 'y_channel_count'))} {str(parser.get('Config', 'x_channel_count'))} {str(parser.get('Config', 'main_filter_phases'))} {str(parser.get('Config', 'shadow_filter_phases'))}"

cmd = f"waf configure clean build --aec-config".split(' ')
cmd.append(build_config)
subprocess.run(cmd, check=True)
