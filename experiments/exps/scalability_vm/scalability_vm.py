import experiments as exp

from exps.scalability_vm.configs.virt_tas import Config as TasVirtConf
from exps.scalability_vm.configs.ovs_tas import Config as OVSTasConf
from exps.scalability_vm.configs.ovs_linux import Config as OVSLinuxConf
from exps.scalability_vm.configs.bare_tas import Config as BareTasConf
from exps.scalability_vm.configs.bare_vtas import Config as BareVTasConf

experiments = []
n_runs = 1
n_vms = [18,15,12,9,6,3]

for n_r in range(n_runs):
  for n_v in n_vms:
    exp_name = "scalability-vm-run{}-vms{}_".format(n_r, n_v)
    tas_virt_exp = exp.Experiment(TasVirtConf(exp_name + "virt-tas", n_v), name=exp_name)
    ovs_tas_exp = exp.Experiment(OVSTasConf(exp_name + "ovs-tas", n_v), name=exp_name)
    ovs_linux_exp = exp.Experiment(OVSLinuxConf(exp_name + "ovs-linux", n_v), name=exp_name)
    # bare_tas_exp = exp.Experiment(BareTasConf(exp_name + "bare-tas", n_v), name=exp_name)
    # bare_vtas_exp = exp.Experiment(BareVTasConf(exp_name + "bare-vtas", n_v), name=exp_name)

    # experiments.append(tas_virt_exp)
    experiments.append(ovs_tas_exp)
    experiments.append(ovs_linux_exp)
    # experiments.append(bare_tas_exp)
    # experiments.append(bare_vtas_exp)
