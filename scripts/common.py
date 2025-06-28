import os
from pathlib import Path

HOME_DIR = str(Path.home())
EXP_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJ_DIR = os.path.dirname(EXP_SCRIPT_DIR)
BUILD_DIR = os.path.join(PROJ_DIR, "build")
CONFIG_DIR = os.path.join(PROJ_DIR, "conf")
EVAL_SETTING_FILE = os.path.join(EXP_SCRIPT_DIR, "settings.ini")
HADOOP_DIR = str(Path.home() / "hadoop-3.3.4")