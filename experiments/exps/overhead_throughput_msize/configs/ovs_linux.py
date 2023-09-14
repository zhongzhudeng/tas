from configs.gen_config import Defaults
from configs.gen_config import MachineConfig
from configs.gen_config import VMConfig
from configs.gen_config import ClientConfig
from configs.gen_config import ServerConfig
from configs.gen_config import CSetConfig

class Config:
    def __init__(self, exp_name, msize):
        self.exp_name = exp_name
        self.defaults = Defaults()
        
        # Configure csets
        self.s_cset_configs = []
        self.c_cset_configs = []

        vm0_cset = CSetConfig([1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43], "0-1", "vm0_server")
        self.s_cset_configs.append(vm0_cset)

        vm0_cset = CSetConfig([1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43], "0-1", "vm0_client")
        self.c_cset_configs.append(vm0_cset)

        # Server Machine
        self.sstack = 'ovs-linux'
        self.snum = 1
        self.snodenum = 1
        self.s_tas_configs = []
        self.s_vm_configs = []
        self.s_proxyg_configs = []
        self.server_configs = []
        
        self.s_machine_config = MachineConfig(ip=self.defaults.server_ip, 
                interface=self.defaults.server_interface,
                stack=self.sstack,
                is_remote=True,
                is_server=True)
        
        vm0_config = VMConfig(pane=self.defaults.s_vm_pane,
                machine_config=self.s_machine_config,
                tas_dir=self.defaults.default_vtas_dir_bare,
                tas_dir_virt=self.defaults.default_vtas_dir_virt,
                idx=0,
                n_cores=22,
                cset="vm0_server",
                memory=10,
                n_queues=10)

        self.s_vm_configs.append(vm0_config)

        server0_config = ServerConfig(pane=self.defaults.s_server_pane,
                idx=0, vmid=0,
                port=1234, ncores=12, max_flows=4096, max_bytes=4096,
                bench_dir=self.defaults.default_obenchmark_dir_virt,
                tas_dir=self.defaults.default_otas_dir_virt)
        self.server_configs.append(server0_config)

        # Client Machine
        self.cstack = 'ovs-linux'
        self.cnum = 1
        self.cnodenum = 1
        self.c_tas_configs = []
        self.c_vm_configs = []
        self.c_proxyg_configs = []
        self.client_configs = []

        self.c_machine_config = MachineConfig(ip=self.defaults.client_ip, 
                interface=self.defaults.client_interface,
                stack=self.cstack,
                is_remote=False,
                is_server=False)
        
        vm0_config = VMConfig(pane=self.defaults.c_vm_pane,
                machine_config=self.c_machine_config,
                tas_dir=self.defaults.default_vtas_dir_bare,
                tas_dir_virt=self.defaults.default_vtas_dir_virt,
                idx=0,
                n_cores=22,
                cset="vm0_client",
                memory=10,
                n_queues=10)

        self.c_vm_configs.append(vm0_config)

        client0_config = ClientConfig(exp_name=exp_name, 
                pane=self.defaults.c_client_pane,
                idx=0, vmid=0, stack=self.cstack,
                ip=self.s_vm_configs[0].vm_ip, port=1234, ncores=12,
                msize=msize, mpending=64, nconns=100,
                open_delay=10, max_msgs_conn=0, max_pend_conns=1,
                bench_dir=self.defaults.default_obenchmark_dir_virt,
                tas_dir=self.defaults.default_otas_dir_virt)

        self.client_configs.append(client0_config)