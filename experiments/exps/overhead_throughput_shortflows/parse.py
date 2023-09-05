import sys
sys.path.append("../../../")

import os
import pandas as pd
import experiments.plot_utils as putils

def check_flowlen(data, flowlen):
  if flowlen not in data:
    data[flowlen] = {}

def check_stack(data, flowlen, stack):
  if stack not in data[flowlen]:
    data[flowlen][stack] = {}

def check_run(data, flowlen, stack, run):
  if run not in data[flowlen][stack]:
    data[flowlen][stack][run] = {}

def check_nid(data, flowlen, stack, run, nid):
  if nid not in data[flowlen][stack][run]:
    data[flowlen][stack][run][nid] = {}

def check_cid(data, flowlen, stack, run, nid, cid):
  if cid not in data[flowlen][stack][run][nid]:
    data[flowlen][stack][run][nid][cid] = ""

def parse_metadata():
  dir_path = "./out/"
  data = {}

  for f in os.listdir(dir_path):
    fname = os.fsdecode(f)

    if "latency_hist" not in fname:
      continue

    run = putils.get_expname_run(fname)
    flowlen = putils.get_expname_flowlen(fname)
    cid = putils.get_client_id(fname)
    nid = putils.get_node_id(fname)
    stack = putils.get_stack(fname)

    check_flowlen(data, flowlen)
    check_stack(data, flowlen, stack)
    check_run(data, flowlen, stack, run)
    check_nid(data, flowlen, stack, run, nid)
    check_cid(data, flowlen, stack, run, nid, cid)

    data[flowlen][stack][run][nid][cid] = fname

  return data

def parse_data(parsed_md):
  data = {}
  out_dir = "./out/"
  for flowlen in parsed_md:
    data[flowlen] = {}
    for stack in parsed_md[flowlen]:
      hist = None
      data[flowlen][stack] = {"lat": None, "count": None}
      for run in parsed_md[flowlen][stack]:
        fname_c0 = out_dir + parsed_md[flowlen][stack][run]['0']['0']
        df = pd.read_table(fname_c0, delimiter=" ")
        if hist is None:
          hist = df
        else:
          # For some reason pandas named the column "0.1"
          hist["0.1"] = hist["0.1"] + df["0.1"]
      
      data[flowlen][stack]["count"] = hist["0.1"]
  
  return data

def save_dat_file(data):
  flowlens = ["1", "64", "128"]
  stacks = ["bare-tas", "virt-tas", "ovs-tas", "bare-linux", "ovs-linux"]

  for flowlen in flowlens:
    for stack in stacks:
      fname = "./{}_{}_hist.dat".format(stack, flowlen)
      data[flowlen][stack]["count"].to_csv(fname, header=False, sep=" ")

def main():
  parsed_md = parse_metadata()
  latencies = parse_data(parsed_md)
  save_dat_file(latencies)

if __name__ == '__main__':
  main()