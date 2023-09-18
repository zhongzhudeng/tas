import experiments as exp

from exps.overhead_latency_short.configs.bare_tas import Config as TasBareConf
from exps.overhead_latency_short.configs.bare_vtas import Config as VTasBareConf
from exps.overhead_latency_short.configs.virt_tas import Config as TasVirtConf
from exps.overhead_latency_short.configs.ovs_tas import Config as OVSTasConf
from exps.overhead_latency_short.configs.bare_linux import Config as BareLinuxConf
from exps.overhead_latency_short.configs.ovs_linux import Config as OVSLinuxConf

experiments = []

flow_lens = [1, 64]
n_runs = 1

for n_r in range(n_runs):
  for flow_len in flow_lens:
    exp_name = "overhead-latencyshort-run{}-flowlen{}_".format(n_r, flow_len)
    tas_bare_exp = exp.Experiment(TasBareConf(exp_name + "bare-tas", flow_len), name=exp_name)
    vtas_bare_exp = exp.Experiment(VTasBareConf(exp_name + "bare-vtas", flow_len), name=exp_name)
    tas_virt_exp = exp.Experiment(TasVirtConf(exp_name + "virt-tas", flow_len), name=exp_name)
    ovs_tas_exp = exp.Experiment(OVSTasConf(exp_name + "ovs-tas", flow_len), name=exp_name)
    bare_linux_exp = exp.Experiment(BareLinuxConf(exp_name + "bare-linux", flow_len), name=exp_name)
    ovs_linux_exp = exp.Experiment(OVSLinuxConf(exp_name + "ovs-linux", flow_len), name=exp_name)

    experiments.append(tas_virt_exp)
    experiments.append(tas_bare_exp)
    experiments.append(ovs_tas_exp)
    experiments.append(bare_linux_exp)
    experiments.append(ovs_linux_exp)


