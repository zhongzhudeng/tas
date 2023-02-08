import experiments as exp

from exps.perf_iso_latmsize.configs.bare_tas import Config as TasBareConf
from exps.perf_iso_latmsize.configs.bare_vtas import Config as VTasBareConf
from exps.perf_iso_latmsize.configs.virt_tas import Config as TasVirtConf

experiments = []

msize = [64, 128, 256, 512, 1024, 2048, 4096]

for n in msize:
  exp_name = "perf-iso-latmsize_msize{}_".format(n)
  tas_bare_exp = exp.Experiment(TasBareConf(exp_name + "bare-tas", n), name=exp_name)
  vtas_bare_exp = exp.Experiment(VTasBareConf(exp_name + "bare-vtas", n), name=exp_name)
  tas_virt_exp = exp.Experiment(TasVirtConf(exp_name + "virt-tas", n), name=exp_name)

  experiments.append(tas_bare_exp)
  experiments.append(vtas_bare_exp)
  experiments.append(tas_virt_exp)
  
