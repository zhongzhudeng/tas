from exps.scalability_vm.helpers import *
from configs.gen_config import Defaults
from configs.gen_config import MachineConfig
from configs.gen_config import VMConfig
from configs.gen_config import ClientConfig
from configs.gen_config import ServerConfig

class Config:
    def __init__(self, exp_name, n_vms):
        self.exp_name = exp_name
        self.defaults = Defaults()
        
        # Configure Csets
        pmd_mask = "0xaaa"
        ovs_cores = [1,3,5,7,9,11,13]

        self.s_cset_configs = []
        self.c_cset_configs = []

        vm_cset = create_vm_csets(n_vms, 1, skip_cores=ovs_cores)
        self.c_cset_configs = self.c_cset_configs + vm_cset
        self.s_cset_configs = self.s_cset_configs + vm_cset

        # Server Machine
        self.sstack = 'ovs-linux'
        self.snum = 1
        self.snodenum = n_vms
        self.s_tas_configs = []
        self.s_vm_configs = []
        self.s_proxyg_configs = []
        self.server_configs = []
        
        self.s_machine_config = MachineConfig(ip=self.defaults.server_ip, 
                interface=self.defaults.server_interface,
                stack=self.sstack,
                ovs_pmd_mask=pmd_mask,
                is_remote=True,
                is_server=True)
        
        for idx in range(self.snodenum):
                vm_config = VMConfig(pane=self.defaults.s_vm_pane,
                        machine_config=self.s_machine_config,
                        tas_dir=self.defaults.default_vtas_dir_bare,
                        tas_dir_virt=self.defaults.default_vtas_dir_virt,
                        idx=idx,
                        n_cores=1,
                        cset="vm{}".format(idx),
                        memory=1,
                        n_queues=len(ovs_cores))
                self.s_vm_configs.append(vm_config)

                server_config = ServerConfig(pane=self.defaults.s_server_pane,
                        idx=0, vmid=idx,
                        port=1234, ncores=1, max_flows=4096, max_bytes=4096,
                        bench_dir=self.defaults.default_obenchmark_dir_virt,
                        tas_dir=self.defaults.default_vtas_dir_virt)
                self.server_configs.append(server_config)

        # Client Machine
        self.cstack = 'ovs-linux'
        self.cnum = 1
        self.cnodenum = n_vms
        self.c_tas_configs = []
        self.c_vm_configs = []
        self.c_proxyg_configs = []
        self.client_configs = []

        self.c_machine_config = MachineConfig(ip=self.defaults.client_ip, 
                interface=self.defaults.client_interface,
                stack=self.cstack,
                ovs_pmd_mask=pmd_mask,
                is_remote=False,
                is_server=False)
        
        for idx in range(self.cnodenum):
                vm_config = VMConfig(pane=self.defaults.c_vm_pane,
                        machine_config=self.c_machine_config,
                        tas_dir=self.defaults.default_vtas_dir_bare,
                        tas_dir_virt=self.defaults.default_vtas_dir_virt,
                        idx=idx,
                        n_cores=1,
                        cset="vm{}".format(idx),
                        memory=1,
                        n_queues=len(ovs_cores))

                self.c_vm_configs.append(vm_config)

                client_config = ClientConfig(exp_name=exp_name, 
                        pane=self.defaults.c_client_pane,
                        idx=0, vmid=idx, stack=self.cstack,
                        ip=self.s_vm_configs[0].vm_ip, port=1234, ncores=1,
                        msize=64, mpending=64, nconns=10,
                        open_delay=1, max_msgs_conn=0, max_pend_conns=1,
                        bench_dir=self.defaults.default_vbenchmark_dir_virt,
                        tas_dir=self.defaults.default_vtas_dir_virt)

                self.client_configs.append(client_config)