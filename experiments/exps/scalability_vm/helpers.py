from configs.gen_config import CSetConfig

def select_cores(n_cores, end_core, skip_cores):
    cores = []
    i = 0
    n = 0
    while n < n_cores and i < end_core:
        if i in skip_cores:
            i += 1
            continue
        else:
            cores.append(i)
            i += 1
            n += 1

    return cores

def create_app_csets(n_vms, n_cores, end_core=43, skip_cores=[], exclusive=False):
    csets = []
    for i in range(n_vms):
        cores = select_cores(n_cores, end_core, skip_cores)
        skip_cores = skip_cores + cores
        cset = CSetConfig(cores, "0-1", "app{}".format(i), exclusive)
        csets.append(cset)

    return csets

def create_vm_csets(n_vms, n_cores, end_core=43, skip_cores=[], exclusive=False):
    csets = []
    for i in range(n_vms):
        cores = select_cores(n_cores, end_core, skip_cores)
        skip_cores = skip_cores + cores
        cset = CSetConfig(cores, "0-1", "vm{}".format(i), exclusive)
        csets.append(cset)

    return csets
