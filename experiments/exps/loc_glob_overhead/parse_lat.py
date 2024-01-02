import sys
sys.path.append("../../../")

import os
import numpy as np
import pandas as pd
import experiments.plot_utils as putils


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

def get_avg_tp(fname_c1, fname_c0):
  n_messages = 0
  n = 0

  f = open(fname_c1)
  lines = f.readlines()

  c1_first_ts = putils.get_first_ts(fname_c0)
  idx, _ = putils.get_min_idx(fname_c1, c1_first_ts)

  first_line = lines[idx]
  last_line = lines[len(lines) - 1]

  n_messages = int(putils.get_n_messages(last_line)) - \
      int(putils.get_n_messages(first_line))
  n = len(lines) - idx

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
    stack = putils.get_stack(fname)

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
      latencies = putils.init_latencies()
      for run in parsed_md[cores][stack]:
        fname_c0 = out_dir + parsed_md[cores][stack][run]['0']['0']
        fname_c1 = out_dir + parsed_md[cores][stack][run]['1']['0']
        putils.append_latencies(latencies, fname_c0)
        tp = get_avg_tp(fname_c1, fname_c0)
        if tp > 0:
          tp_x = np.append(tp_x, tp)

      data_point[stack] = {
        "tp": tp_x.mean(),
        "tp-std": tp_x.std(),
        "lat": putils.get_latency_avg(latencies),
        "lat-std": putils.get_latency_std(latencies)
      }
    data[cores] = data_point
  
  return data
  
def save_dat_file(data):
  header = "cores virt-tas-avg virt-tas-std\n"

  cores_list = list(data.keys())
  cores_list = list(map(str, sorted(map(float, cores_list))))
  stacks =  list(data[cores_list[0]].keys())
  percentiles =  list(data[cores_list[0]][stacks[0]]['lat'].keys())

  for percentile in percentiles:
      fname_lat = "./lat_{}.dat".format(percentile)
      f_lat = open(fname_lat, "w+")
      f_lat.write(header)

      for core in cores_list:
        f_lat.write("{} {} {}\n".format(
          float(core),
          data[core]['virt-tas']["lat"][percentile],
          data[core]['virt-tas']["lat-std"][percentile]))
  
  fname_tp = "./tp.dat"
  f_tp = open(fname_tp, "w+")
  f_tp.write(header)

  for core in cores_list:
    f_tp.write("{} {} {}\n".format(
      float(core),
      data[core]['virt-tas']["tp"],
      data[core]['virt-tas']["tp-std"]))

def main():
  parsed_md = parse_metadata()
  latencies = parse_data(parsed_md)
  save_dat_file(latencies)

if __name__ == '__main__':
  main()