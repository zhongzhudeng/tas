import sys
sys.path.append("../../../")

import os
import numpy as np
import pandas as pd
import experiments.plot_utils as putils

def get_stack(fname):
  if "local" in fname:
    return "local"
  elif "global" in fname:
    return "global"

def check_cores(data, cores):
  if cores not in data:
    data[cores] = {}

def check_stack(data, cores, stack):
  if stack not in data[cores]:
    data[cores][stack] = {}

def check_run(data, cores, stack, run):
  if run not in data[cores][stack]:
    data[cores][stack][run] = {}

def check_nid(data, cores, stack, run, nid):
  if nid not in data[cores][stack][run]:
    data[cores][stack][run][nid] = {}

def check_cid(data, cores, stack, run, nid, cid):
  if cid not in data[cores][stack][run][nid]:
    data[cores][stack][run][nid][cid] = ""

# NOTE: Aggressor has different message size (1024 vs 64 bytes)
def get_avg_tp(fname):
  n_messages = 0
  n = 0

  f = open(fname)
  lines = f.readlines()

  first_line = lines[0]
  last_line = lines[len(lines) - 1]

  n_messages = int(putils.get_n_messages(last_line)) - \
      int(putils.get_n_messages(first_line))
  n = len(lines)

  return n_messages / n

def parse_metadata():
  dir_path = "./out/"
  data = {}

  putils.remove_cset_dir(dir_path)
  for f in os.listdir(dir_path):
    fname = os.fsdecode(f)
    if "tas_c" == fname or "hist" in fname:
      continue

    run = putils.get_expname_run(fname)
    cores = str(float(putils.get_expname_cores(fname)))
    cid = putils.get_client_id(fname)
    nid = putils.get_node_id(fname)
    stack = get_stack(fname)

    check_cores(data, cores)
    check_stack(data, cores, stack)
    check_run(data, cores, stack, run)
    check_nid(data, cores, stack, run, nid)
    check_cid(data, cores, stack, run, nid, cid)

    data[cores][stack][run][nid][cid] = fname

  return data

def parse_data(parsed_md):
  data = {}
  out_dir = "./out/"
  for cores in parsed_md:
    data_point = {}
    for stack in parsed_md[cores]:
      tp_x = np.array([])
      for run in parsed_md[cores][stack]:
        fname = out_dir + parsed_md[cores][stack][run]['0']['0']
        tp = get_avg_tp(fname)
        tp_x = np.append(tp_x, tp)
      data_point[stack] = {
        "tp": tp_x.mean(),
        "tp-std": tp_x.std(),
      }
    data[cores] = data_point
  
  return data

def save_dat_file(data):
  header = "cores local-avg local-std global-avg global-std\n"
  cores_list = list(data.keys())
  cores_list = list(map(str, sorted(map(float, cores_list))))

  fname = "./tp.dat"
  f_lat = open(fname, "w+")
  f_lat.write(header)

  for cores in cores_list:
    f_lat.write("{} {} {} {} {}\n".format(
      float(cores),
      data[cores]['local']["tp"],
      data[cores]['local']["tp-std"],
      data[cores]['global']["tp"],
      data[cores]['global']["tp-std"],
      ))
    
def main():
  parsed_md = parse_metadata()
  latencies = parse_data(parsed_md)
  save_dat_file(latencies)

if __name__ == '__main__':
  main()