import experiments as exp

from exps.bursty_resource_efficiency.configs.virt_tas import Config as TasVirtConf
from exps.bursty_resource_efficiency.configs.ovs_tas import Config as OVSTasConf

experiments = []

fp_cores = [1, 2, 3]
n_runs = 1

for n_r in range(n_runs):
  for n_c in fp_cores:
    exp_name = "bursty-resource-efficiency-run{}-fpcores{}_".format(n_r, n_c)
    tas_virt_exp = exp.Experiment(TasVirtConf(exp_name + "virt-tas", n_c), name=exp_name)
    experiments.append(tas_virt_exp)

  exp_name = "bursty-resource-efficiency-run{}-fpcores{}_".format(1, 1)
  ovs_tas_exp = exp.Experiment(OVSTasConf(exp_name + "ovs-tas", 1), name=exp_name)
  experiments.append(ovs_tas_exp)