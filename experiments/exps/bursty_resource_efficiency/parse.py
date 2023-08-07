import sys
sys.path.append("../../../")

import os
import numpy as np
import experiments.plot_utils as putils
import bisect

TSC_DIV = 100000

def check_fpcores(data, fpcores):
  if fpcores not in data:
    data[fpcores] = {}

def check_stack(data, msize, stack):
  if stack not in data[msize]:
    data[msize][stack] = {}

def check_run(data, msize, stack, run):
  if run not in data[msize][stack]:
    data[msize][stack][run] = {}

def check_nid(data, msize, stack, run, nid):
  if nid not in data[msize][stack][run]:
    data[msize][stack][run][nid] = {}

def check_cid(data, msize, stack, run, nid, cid):
  if cid not in data[msize][stack][run][nid]:
    data[msize][stack][run][nid][cid] = ""

def parse_metadata():
  dir_path = "./out/"
  data = {}

  for f in os.listdir(dir_path):
    fname = os.fsdecode(f)

    if "tas_c" == fname or "latency_hist" in fname:
      continue

    run = putils.get_expname_run(fname)
    fpcores = putils.get_expname_fpcores(fname)
    cid = putils.get_client_id(fname)
    nid = putils.get_node_id(fname)
    stack = putils.get_stack(fname)

    check_fpcores(data, fpcores)
    check_stack(data, fpcores, stack)
    check_run(data, fpcores, stack, run)
    check_nid(data, fpcores, stack, run, nid)
    check_cid(data, fpcores, stack, run, nid, cid)

    data[fpcores][stack][run][nid][cid] = fname

  return data

def parse_data(parsed_md):
  data = {}
  out_dir = "./out/"
  for fpcores in parsed_md:
    for stack in parsed_md[fpcores]:
      for run in parsed_md[fpcores][stack]:
        is_virt = stack == "virt-tas"
        if is_virt:
          c0_fname = out_dir + parsed_md[fpcores][stack][run]["0"]["0"]
          c1_fname = out_dir + parsed_md[fpcores][stack][run]["1"]["0"]
          c2_fname = out_dir + parsed_md[fpcores][stack][run]["2"]["0"]
        else:
          c0_fname = out_dir + parsed_md[fpcores][stack][run]["0"]["0"]
          c1_fname = out_dir + parsed_md[fpcores][stack][run]["0"]["1"]
          c2_fname = out_dir + parsed_md[fpcores][stack][run]["0"]["2"]

        # ts_l = []
        # ts_l_idx = []
        # add_timestamps(c0_fname, ts_l, ts_l_idx, 0)
        # add_timestamps(c1_fname, ts_l, ts_l_idx, 1)
        # add_timestamps(c2_fname, ts_l, ts_l_idx, 2)
        # ts_l.sort()
        # ts_l_idx = sorted(ts_l_idx, key= lambda e: e["tsc"])
        # ts_l = []
        # add_to_ts_list(ts_l, c0_fname)
        # add_to_ts_list(ts_l, c1_fname)
        # add_to_ts_list(ts_l, c2_fname)
        # intervals, counts = np.unique(ts_l, return_counts=True)
        timestamps = get_timestamps(c0_fname)
        tps = get_throughput(c0_fname)
        c0_seconds = np.linspace(start=0, 
                                 stop=len(timestamps), 
                                 num=len(timestamps),
                                 endpoint=False).astype(int)
        assert(len(timestamps) == len(c0_seconds))
        assert(len(tps) == len(c0_seconds))

        c1_seconds = timestamps_to_seconds(timestamps, c1_fname)
        c2_seconds = timestamps_to_seconds(timestamps, c2_fname)
        count_tp(tps, c0_seconds, c1_seconds, c1_fname)
        count_tp(tps, c0_seconds, c2_seconds, c2_fname)
        # populate_frequency_bins(tp, intervals, c0_fname)
        # populate_frequency_bins(tp, intervals, c1_fname)
        # populate_frequency_bins(tp, intervals, c2_fname)
        # populate_frequency_bins(tp, ts_l, c0_fname)
        # populate_frequency_bins(tp, ts_l, c1_fname)
        # populate_frequency_bins(tp, ts_l, c2_fname)

        if stack not in data:
          data[stack] = {}

        if fpcores not in data[stack]:
          data[stack][fpcores] = {}
          
        data[stack][fpcores]["tp"] = tps
        data[stack][fpcores]["ts"] = c0_seconds

  return data

def get_throughput(fname):
  tps = []
  f = open(fname)
  lines = f.readlines()

  for line in lines:
    tp = float(putils.get_tp(line).replace(",", ""))
    tps.append(tp)

  return tps

def count_tp(tps, base_secs, secs, fname):
  f = open(fname)
  lines = f.readlines()
  end_sec = base_secs[len(base_secs) - 1]

  for i in range(len(lines)):
    line = lines[i]
    sec = secs[i]

    if sec > end_sec:
      break

    tp = float(putils.get_tp(line).replace(",", ""))
    print(sec)
    print(len(tps))
    print(len(base_secs))
    print(end_sec)
    print()
    tps[sec] += tp

def timestamps_to_seconds(timestamps, fname):
  f = open(fname)
  lines = f.readlines()
  first_ts = int(putils.get_ts(lines[0]))
  first_second = bisect.bisect(timestamps, first_ts)
  seconds = np.linspace(start=first_second,
                        stop=first_second + len(lines),
                        num=len(lines)).astype(int)
  assert(len(seconds) == len(lines))
  return seconds

def get_timestamps(fname):
  f = open(fname)
  lines = f.readlines()
  
  intervals = []

  for line in lines:
    tsc = int(putils.get_ts(line))
    intervals.append(tsc)
  
  return intervals

# def get_intervals(fname, n_intervals):
def add_timestamps(fname, ts_l, ts_l_idx, client):
  # intervals = []
  
  f = open(fname)
  lines = f.readlines()

  for line in lines:
    tsc = int(putils.get_ts(line))
    ts_l.append(tsc)
    ts_l_idx.append({"tsc": tsc, "id": client})
  # min_ts = int(int(putils.get_ts(lines[0])) / TSC_DIV)
  # max_ts = int(int(putils.get_ts(lines[n_intervals])) / TSC_DIV)
  # intervals = np.linspace(start=min_ts, stop=max_ts, num=max_ts - min_ts)
  # return intervals
  # for line in lines[:n_intervals]:
  #   tsc = int(putils.get_ts(line))
  #   intervals.append(tsc)
    
  # return np.array(intervals)

def add_to_ts_list(ts_l, ts_l_idx, fname):
  f = open(fname)
  lines = f.readlines()
  for i in range(len(lines)):
    line = lines[i]
    tsc = int(int(putils.get_ts(line)) / TSC_DIV)
    ts_l.append(tsc)
    ts_l_idx.append({"tsc": tsc, "i": i})

def populate_frequency_bins(bins, intervals, fname):
  f = open(fname)
  lines = f.readlines()
  for line in lines:
    tsc = int(putils.get_ts(line))  
    idx = bisect.bisect(intervals, tsc)
    if idx < len(intervals):
      bins[idx] += float(putils.get_tp(line).replace(",", ""))

def save_dat_file(data, fname):
  f = open(fname, "w+")
  header = "ts tp\n"
  f.write(header)

  for i in range(len(data["ts"])):
    ts = int(data["ts"][i]) - int(data["ts"][0])
    tp = data["tp"][i]
    f.write("{} {}\n".format(ts, tp))
        
def main():
  parsed_md = parse_metadata()
  data = parse_data(parsed_md)
  ovs_tas_1 = data["ovs-tas"]["1"]
  virt_tas_1 = data["virt-tas"]["1"]
  virt_tas_2 = data["virt-tas"]["2"]
  virt_tas_3 = data["virt-tas"]["3"]
  save_dat_file(ovs_tas_1, "./ovs_tas_1_tp.dat")
  save_dat_file(virt_tas_1, "./virt_tas_1_tp.dat")
  save_dat_file(virt_tas_2, "./virt_tas_2_tp.dat")
  save_dat_file(virt_tas_3, "./virt_tas_3_tp.dat")

if __name__ == '__main__':
  main()