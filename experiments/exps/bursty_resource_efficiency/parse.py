import sys
sys.path.append("../../../")

import os
import numpy as np
import experiments.plot_utils as putils
import bisect

TSC_DIV = 100000

def check_overlap(data, overlap):
  if overlap not in data:
    data[overlap] = {}

def check_fpcores(data, overlap, fpcores):
  if fpcores not in data[overlap]:
    data[overlap][fpcores] = {}

def check_stack(data, overlap, fpcores, stack):
  if stack not in data[overlap][fpcores]:
    data[overlap][fpcores][stack] = {}

def check_run(data, overlap, fpcores, stack, run):
  if run not in data[overlap][fpcores][stack]:
    data[overlap][fpcores][stack][run] = {}

def check_nid(data, overlap, fpcores, stack, run, nid):
  if nid not in data[overlap][fpcores][stack][run]:
    data[overlap][fpcores][stack][run][nid] = {}

def check_cid(data, overlap, fpcores, stack, run, nid, cid):
  if cid not in data[overlap][fpcores][stack][run][nid]:
    data[overlap][fpcores][stack][run][nid][cid] = ""

def parse_metadata():
  dir_path = "./out/"
  data = {}

  for f in os.listdir(dir_path):
    fname = os.fsdecode(f)

    if "tas_c" == fname or "latency_hist" in fname:
      continue

    run = putils.get_expname_run(fname)
    overlap = putils.get_expname_overlap(fname)
    fpcores = putils.get_expname_fpcores(fname)
    cid = putils.get_client_id(fname)
    nid = putils.get_node_id(fname)
    stack = putils.get_stack(fname)

    check_overlap(data, overlap)
    check_fpcores(data, overlap, fpcores)
    check_stack(data, overlap, fpcores, stack)
    check_run(data, overlap, fpcores, stack, run)
    check_nid(data, overlap, fpcores, stack, run, nid)
    check_cid(data, overlap, fpcores, stack, run, nid, cid)

    data[overlap][fpcores][stack][run][nid][cid] = fname

  return data

def parse_data(parsed_md):
  data = {}
  out_dir = "./out/"
  for overlap in parsed_md:
    for fpcores in parsed_md[overlap]:
      for stack in parsed_md[overlap][fpcores]:
        for run in parsed_md[overlap][fpcores][stack]:

          c0_fname = out_dir + parsed_md[overlap][fpcores][stack][run]["0"]["0"]
          c1_fname = out_dir + parsed_md[overlap][fpcores][stack][run]["1"]["0"]
          c2_fname = out_dir + parsed_md[overlap][fpcores][stack][run]["2"]["0"]
          c3_fname = out_dir + parsed_md[overlap][fpcores][stack][run]["3"]["0"]
          c4_fname = out_dir + parsed_md[overlap][fpcores][stack][run]["4"]["0"]
          c5_fname = out_dir + parsed_md[overlap][fpcores][stack][run]["5"]["0"]
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
          c3_seconds = timestamps_to_seconds(timestamps, c3_fname)
          c4_seconds = timestamps_to_seconds(timestamps, c4_fname)
          c5_seconds = timestamps_to_seconds(timestamps, c5_fname)
          count_tp(tps, c0_seconds, c1_seconds, c1_fname)
          count_tp(tps, c0_seconds, c2_seconds, c2_fname)
          count_tp(tps, c0_seconds, c3_seconds, c3_fname)
          count_tp(tps, c0_seconds, c4_seconds, c4_fname)
          count_tp(tps, c0_seconds, c5_seconds, c5_fname)
          # populate_frequency_bins(tp, intervals, c0_fname)
          # populate_frequency_bins(tp, intervals, c1_fname)
          # populate_frequency_bins(tp, intervals, c2_fname)
          # populate_frequency_bins(tp, ts_l, c0_fname)
          # populate_frequency_bins(tp, ts_l, c1_fname)
          # populate_frequency_bins(tp, ts_l, c2_fname)

          if stack not in data:
            data[stack] = {}

          if overlap not in data[stack]:
            data[stack][overlap] = {}

          if fpcores not in data[stack][overlap]:
            data[stack][overlap][fpcores] = {}
            
          data[stack][overlap][fpcores]["tp"] = tps
          data[stack][overlap][fpcores]["ts"] = c0_seconds

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
  overlaps = ["33", "50", "83"]
  stacks = ["ovs-tas", "virt-tas"]
  fpcores = ["2", "4", "6"]
  parsed_md = parse_metadata()
  data = parse_data(parsed_md)

  for stack in stacks:
    for overlap in overlaps:
      if stack == "virt-tas":
        for fpcore in fpcores:
          save_dat_file(data[stack][overlap][fpcore], 
                        "./{}-{}-{}-tp.dat".format(stack, overlap, fpcore))
      else:
          save_dat_file(data[stack][overlap]["1"], 
                        "./ovs-tas-{}-1-tp.dat".format(overlap))

if __name__ == '__main__':
  main()