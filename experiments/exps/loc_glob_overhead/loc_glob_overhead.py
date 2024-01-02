import experiments as exp

from exps.loc_glob_overhead.configs.virt_tas import Config as TasVirtConf

experiments = []

cores = [1,3,5,7,9]
n_runs = 3

for n_r in range(n_runs):
  for core in cores:
      exp_name = "loc-glob-overhead-run{}-cores{}_".format(n_r, core)
      global_exp = exp.Experiment(TasVirtConf(exp_name + "global", core), name=exp_name)
      local_exp = exp.Experiment(TasVirtConf(exp_name + "local", core), name=exp_name)
      # experiments.append(global_exp)
      experiments.append(local_exp)
