import experiments as exp

from exps.overhead_latency_long.configs.bare_tas import Config as TasBareConf
from exps.overhead_latency_long.configs.bare_vtas import Config as VTasBareConf
from exps.overhead_latency_long.configs.virt_tas import Config as TasVirtConf
from exps.overhead_latency_long.configs.ovs_tas import Config as OVSTasConf
from exps.overhead_latency_long.configs.bare_linux import Config as BareLinuxConf
from exps.overhead_latency_long.configs.ovs_linux import Config as OVSLinuxConf

experiments = []

n_runs = 1

for n_r in range(n_runs):
  exp_name = "overhead-latencylong-run{}_".format(n_r)
  tas_bare_exp = exp.Experiment(TasBareConf(exp_name + "bare-tas"), name=exp_name)
  vtas_bare_exp = exp.Experiment(VTasBareConf(exp_name + "bare-vtas"), name=exp_name)
  tas_virt_exp = exp.Experiment(TasVirtConf(exp_name + "virt-tas"), name=exp_name)
  ovs_tas_exp = exp.Experiment(OVSTasConf(exp_name + "ovs-tas"), name=exp_name)
  bare_linux_exp = exp.Experiment(BareLinuxConf(exp_name + "bare-linux"), name=exp_name)
  ovs_linux_exp = exp.Experiment(OVSLinuxConf(exp_name + "ovs-linux"), name=exp_name)

  experiments.append(tas_bare_exp)
  experiments.append(tas_virt_exp)
  experiments.append(ovs_tas_exp)
  experiments.append(bare_linux_exp)
  experiments.append(ovs_linux_exp)

