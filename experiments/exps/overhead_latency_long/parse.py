import sys
sys.path.append("../../../")

import os
import pandas as pd
import experiments.plot_utils as putils


def check_stack(data, stack):
  if stack not in data:
    data[stack] = {}

def check_run(data, stack, run):
  if run not in data[stack]:
    data[stack][run] = {}

def check_nid(data, stack, run, nid):
  if nid not in data[stack][run]:
    data[stack][run][nid] = {}

def check_cid(data, stack, run, nid, cid):
  if cid not in data[stack][run][nid]:
    data[stack][run][nid][cid] = ""

def parse_metadata():
  dir_path = "./out/"
  data = {}

  putils.remove_cset_dir(dir_path)
  for f in os.listdir(dir_path):
    fname = os.fsdecode(f)

    if "latency_hist" not in fname:
      continue

    run = putils.get_expname_run(fname)
    cid = putils.get_client_id(fname)
    nid = putils.get_node_id(fname)
    stack = putils.get_stack(fname)

    check_stack(data, stack)
    check_run(data, stack, run)
    check_nid(data, stack, run, nid)
    check_cid(data, stack, run, nid, cid)

    data[stack][run][nid][cid] = fname

  return data

def parse_data(parsed_md):
  data = {}
  out_dir = "./out/"
  for stack in parsed_md:
    hist = None
    data[stack] = {"lat": None, "count": None}
    for run in parsed_md[stack]:
      fname_c0 = out_dir + parsed_md[stack][run]['0']['0']
      df = pd.read_table(fname_c0, delimiter=" ")
      if hist is None:
        hist = df
      else:
        # For some reason pandas named the column "0.1"
        hist["0.1"] = hist["0.1"] + df["0.1"]
    
    data[stack]["count"] = hist["0.1"]
  
  return data

def save_dat_file(data):
  stacks = ["bare-tas", "virt-tas", "ovs-tas", "bare-linux", "ovs-linux"]

  for stack in stacks:
    fname = "./{}_hist.dat".format(stack)
    data[stack]["count"].to_csv(fname, header=False, sep=" ")

def main():
  parsed_md = parse_metadata()
  latencies = parse_data(parsed_md)
  save_dat_file(latencies)

if __name__ == '__main__':
  main()