from configs.gen_config import Defaults
from configs.gen_config import MachineConfig
from configs.gen_config import TasConfig
from configs.gen_config import VMConfig
from configs.gen_config import HostProxyConfig
from configs.gen_config import GuestProxyConfig
from configs.gen_config import ClientConfig
from configs.gen_config import ServerConfig
from configs.gen_config import CSetConfig

class Config:
    def __init__(self, exp_name, msize):
        self.exp_name = exp_name
        self.defaults = Defaults()
        
        # Configure Csets
        self.s_cset_configs = []
        self.c_cset_configs = []
        tas_cset = CSetConfig([23,25,27,29,31], "0-1", "tas_server", exclusive=True)
        self.s_cset_configs.append(tas_cset)
        tas_cset = CSetConfig([23,25,27,29,31,33,35,37,39], 1, "tas_client", exclusive=True)
        self.c_cset_configs.append(tas_cset)

        vm0_cset = CSetConfig([3,4,5,6,7,8,9,10,11,12], "0-1", "vm0_server", exclusive=True)
        self.s_cset_configs.append(vm0_cset)
        vm1_cset = CSetConfig([13,14,15,16,17,18,19,20,21,22], "0-1", "vm1_server", exclusive=True)
        self.s_cset_configs.append(vm1_cset)
        
        vm0_cset = CSetConfig([3,4,5,6,7,8,9,10,11,12], "0-1", "vm0_client", exclusive=True)
        self.c_cset_configs.append(vm0_cset)
        vm1_cset = CSetConfig([13,14,15,16,17,18,19,20,21,22], "0-1", "vm1_client", exclusive=True)
        self.c_cset_configs.append(vm1_cset)      

        # Server Machine
        self.sstack = 'virt-tas'
        self.snum = 1
        self.snodenum = 2
        self.s_tas_configs = []
        self.s_vm_configs = []
        self.s_proxyg_configs = []
        self.server_configs = []
        
        self.s_machine_config = MachineConfig(ip=self.defaults.server_ip, 
                interface=self.defaults.server_interface,
                stack=self.sstack,
                is_remote=True,
                is_server=True)

        tas_config = TasConfig(pane=self.defaults.s_tas_pane,
                machine_config=self.s_machine_config,
                project_dir=self.defaults.default_vtas_dir_bare,
                ip=self.s_machine_config.ip,
                cset="tas_server",
                cc_timely_thigh="150",
                n_cores=2)
        tas_config.args = tas_config.args + " --vm-shm-len=4294967296"
        self.s_tas_configs.append(tas_config)

        self.s_proxyh_config = HostProxyConfig(pane=self.defaults.s_proxyh_pane,
                machine_config=self.s_machine_config,
                block=0,
                comp_dir=self.defaults.default_vtas_dir_bare)
        
        vm0_config = VMConfig(pane=self.defaults.s_vm_pane,
                machine_config=self.s_machine_config,
                tas_dir=self.defaults.default_vtas_dir_bare,
                tas_dir_virt=self.defaults.default_vtas_dir_virt,
                idx=0,
                n_cores=10,
                cset="vm0_server",
                memory=10)
        vm1_config = VMConfig(pane=self.defaults.s_vm_pane,
                machine_config=self.s_machine_config,
                tas_dir=self.defaults.default_vtas_dir_bare,
                tas_dir_virt=self.defaults.default_vtas_dir_virt,
                idx=1,
                n_cores=6,
                cset="vm1_server",
                memory=10)

        self.s_vm_configs.append(vm0_config)
        self.s_vm_configs.append(vm1_config)

        proxyg0_config = GuestProxyConfig(pane=self.defaults.s_proxyg_pane,
                    machine_config=self.s_machine_config,
                    block=0,
                    comp_dir=self.defaults.default_vtas_dir_virt)
        proxyg1_config = GuestProxyConfig(pane=self.defaults.s_proxyg_pane,
                    machine_config=self.s_machine_config,
                    block=0,
                    comp_dir=self.defaults.default_vtas_dir_virt)
        
        self.s_proxyg_configs.append(proxyg0_config)
        self.s_proxyg_configs.append(proxyg1_config)

        server0_config = ServerConfig(pane=self.defaults.s_server_pane,
                idx=0, vmid=0,
                port=1234, ncores=1, max_flows=8192, max_bytes=4096,
                bench_dir=self.defaults.default_vbenchmark_dir_virt,
                tas_dir=self.defaults.default_vtas_dir_virt)
        server1_config = ServerConfig(pane=self.defaults.s_server_pane,
                idx=0, vmid=1,
                port=1235, ncores=5, max_flows=8192, max_bytes=4096,
                bench_dir=self.defaults.default_vbenchmark_dir_virt,
                tas_dir=self.defaults.default_vtas_dir_virt)
        self.server_configs.append(server0_config)
        self.server_configs.append(server1_config)

        # Client Machine
        self.cstack = 'virt-tas'
        self.cnum = 1
        self.cnodenum = 2
        self.c_tas_configs = []
        self.c_vm_configs = []
        self.c_proxyg_configs = []
        self.client_configs = []

        self.c_machine_config = MachineConfig(ip=self.defaults.client_ip, 
                interface=self.defaults.client_interface,
                stack=self.cstack,
                is_remote=False,
                is_server=False)

        tas_config = TasConfig(pane=self.defaults.c_tas_pane,
                machine_config=self.c_machine_config,
                project_dir=self.defaults.default_vtas_dir_bare,
                ip=self.c_machine_config.ip,
                cset="tas_client",
                n_cores=5)
        tas_config.args = tas_config.args + " --vm-shm-len=4294967296"
        self.c_tas_configs.append(tas_config)

        self.c_proxyh_config = HostProxyConfig(pane=self.defaults.c_proxyh_pane,
                machine_config=self.c_machine_config,
                block=0,
                comp_dir=self.defaults.default_vtas_dir_bare)
        
        vm0_config = VMConfig(pane=self.defaults.c_vm_pane,
                machine_config=self.c_machine_config,
                tas_dir=self.defaults.default_vtas_dir_bare,
                tas_dir_virt=self.defaults.default_vtas_dir_virt,
                idx=0,
                n_cores=6,
                cset="vm0_client",
                memory=10)
        vm1_config = VMConfig(pane=self.defaults.c_vm_pane,
                machine_config=self.c_machine_config,
                tas_dir=self.defaults.default_vtas_dir_bare,
                tas_dir_virt=self.defaults.default_vtas_dir_virt,
                idx=1,
                n_cores=6,
                cset="vm1_client",
                memory=10)

        self.c_vm_configs.append(vm0_config)
        self.c_vm_configs.append(vm1_config)

        proxyg0_config = GuestProxyConfig(pane=self.defaults.c_proxyg_pane,
                    machine_config=self.c_machine_config,
                    block=0,
                    comp_dir=self.defaults.default_vtas_dir_virt)
        proxyg1_config = GuestProxyConfig(pane=self.defaults.c_proxyg_pane,
                    machine_config=self.c_machine_config,
                    block=0,
                    comp_dir=self.defaults.default_vtas_dir_virt)
        
        self.c_proxyg_configs.append(proxyg0_config)
        self.c_proxyg_configs.append(proxyg1_config)

        client0_config = ClientConfig(exp_name=exp_name, 
                pane=self.defaults.c_client_pane,
                idx=0, vmid=0, stack=self.cstack,
                ip=self.s_vm_configs[0].vm_ip, port=1234, ncores=1,
                msize=64, mpending=64, nconns=1,
                open_delay=15, max_msgs_conn=0, max_pend_conns=1,
                bench_dir=self.defaults.default_vbenchmark_dir_virt,
                tas_dir=self.defaults.default_vtas_dir_virt)
        client1_config = ClientConfig(exp_name=exp_name, 
                pane=self.defaults.c_client_pane,
                idx=0, vmid=1, stack=self.cstack,
                ip=self.s_vm_configs[1].vm_ip, port=1235, ncores=5,
                msize=msize, mpending=64, nconns=100,
                open_delay=15, max_msgs_conn=0, max_pend_conns=1,
                bench_dir=self.defaults.default_vbenchmark_dir_virt,
                tas_dir=self.defaults.default_vtas_dir_virt)

        self.client_configs.append(client0_config)
        self.client_configs.append(client1_config)