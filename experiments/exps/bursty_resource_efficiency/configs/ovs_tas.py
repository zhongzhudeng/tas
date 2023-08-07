from configs.gen_config import Defaults
from configs.gen_config import MachineConfig
from configs.gen_config import TasConfig
from configs.gen_config import VMConfig
from configs.gen_config import HostProxyConfig
from configs.gen_config import GuestProxyConfig
from configs.gen_config import ClientConfig
from configs.gen_config import ServerConfig

class Config:
    def __init__(self, exp_name, n_cores):
        self.exp_name = exp_name
        self.defaults = Defaults()
        
        # Server Machine
        self.sstack = 'ovs-tas'
        self.snum = 1
        self.snodenum = 3
        self.s_tas_configs = []
        self.s_vm_configs = []
        self.s_proxyg_configs = []
        self.server_configs = []
        
        self.s_machine_config = MachineConfig(ip=self.defaults.server_ip, 
                interface=self.defaults.server_interface,
                stack=self.sstack,
                is_remote=True,
                is_server=True)
        
        for idx in range(self.snodenum):
                vm_config = VMConfig(pane=self.defaults.s_vm_pane,
                        machine_config=self.s_machine_config,
                        tas_dir=self.defaults.default_vtas_dir_bare,
                        tas_dir_virt=self.defaults.default_vtas_dir_virt,
                        idx=idx,
                        n_cores=4,
                        memory=10,
                        n_queues=10)
                tas_config = TasConfig(pane=self.defaults.s_tas_pane,
                        machine_config=self.s_machine_config,
                        project_dir=self.defaults.default_otas_dir_virt,
                        ip=vm_config.vm_ip,
                        n_cores=n_cores, dpdk_extra="00:03.0")
                tas_config.args = tas_config.args + " --fp-no-rss --fp-no-xsumoffload"

                self.s_tas_configs.append(tas_config)
                self.s_vm_configs.append(vm_config)

                port = 1230 + idx
                server0_config = ServerConfig(pane=self.defaults.s_server_pane,
                        idx=idx, vmid=idx,
                        port=port, ncores=3, max_flows=4096, max_bytes=4096,
                        bench_dir=self.defaults.default_obenchmark_dir_virt,
                        tas_dir=self.defaults.default_otas_dir_virt)
                self.server_configs.append(server0_config)



        # self.sstack = 'ovs-linux'
        # self.snum = 1
        # self.snodenum = 1
        # self.s_tas_configs = []
        # self.s_vm_configs = []
        # self.s_proxyg_configs = []
        # self.server_configs = []
        
        # self.s_machine_config = MachineConfig(ip=self.defaults.server_ip, 
        #         interface=self.defaults.server_interface,
        #         stack=self.sstack,
        #         is_remote=True,
        #         is_server=True)
        
        # for idx in range(self.snodenum):
        #         vm_config = VMConfig(pane=self.defaults.s_vm_pane,
        #                 machine_config=self.s_machine_config,
        #                 tas_dir=self.defaults.default_vtas_dir_bare,
        #                 tas_dir_virt=self.defaults.default_vtas_dir_virt,
        #                 idx=idx,
        #                 n_cores=4,
        #                 memory=10,
        #                 n_queues=10)
        #         self.s_vm_configs.append(vm_config)

        #         port = 1230 + idx
        #         server0_config = ServerConfig(pane=self.defaults.s_server_pane,
        #                 idx=0, vmid=idx,
        #                 port=port, ncores=3, max_flows=4096, max_bytes=4096,
        #                 bench_dir=self.defaults.default_obenchmark_dir_virt,
        #                 tas_dir=self.defaults.default_otas_dir_virt)
        #         self.server_configs.append(server0_config)


        # Client Machine
        # self.cstack = 'ovs-tas'
        # self.cnum = 1
        # self.cnodenum = 1
        # self.c_tas_configs = []
        # self.c_vm_configs = []
        # self.c_proxyg_configs = []
        # self.client_configs = []

        # self.c_machine_config = MachineConfig(ip=self.defaults.client_ip, 
        #         interface=self.defaults.client_interface,
        #         stack=self.cstack,
        #         is_remote=False,
        #         is_server=False)
        
        # for idx in range(self.cnodenum):
        #         vm0_config = VMConfig(pane=self.defaults.c_vm_pane,
        #                 machine_config=self.c_machine_config,
        #                 tas_dir=self.defaults.default_vtas_dir_bare,
        #                 tas_dir_virt=self.defaults.default_vtas_dir_virt,
        #                 idx=idx,
        #                 n_cores=4,
        #                 memory=10,
        #                 n_queues=10)
        #         tas0_config = TasConfig(pane=self.defaults.c_tas_pane,
        #                 machine_config=self.c_machine_config,
        #                 project_dir=self.defaults.default_otas_dir_virt,
        #                 ip=vm0_config.vm_ip,
        #                 n_cores=1, dpdk_extra="00:03.0")
        #         tas0_config.args = tas0_config.args + " --fp-no-rss --fp-no-xsumoffload"

        #         self.c_tas_configs.append(tas0_config)
        #         self.c_vm_configs.append(vm0_config)

        #         port = 1230 + idx
        #         client0_config = ClientConfig(exp_name=exp_name, 
        #                 pane=self.defaults.c_client_pane,
        #                 idx=0, vmid=idx, stack=self.cstack,
        #                 ip=self.s_vm_configs[idx].vm_ip, port=port, ncores=1,
        #                 msize=64, mpending=64, nconns=1,
        #                 open_delay=3, max_msgs_conn=0, max_pend_conns=1,
        #                 bench_dir=self.defaults.default_obenchmark_dir_virt,
        #                 tas_dir=self.defaults.default_otas_dir_virt)

        #         self.client_configs.append(client0_config)



        # self.cstack = 'virt-tas'
        # self.cnum = 1
        # self.cnodenum = 1
        # self.c_tas_configs = []
        # self.c_vm_configs = []
        # self.c_proxyg_configs = []
        # self.client_configs = []

        # self.c_machine_config = MachineConfig(ip=self.defaults.client_ip, 
        #         interface=self.defaults.client_interface,
        #         stack=self.cstack,
        #         is_remote=False,
        #         is_server=False)
        
        # tas_config = TasConfig(pane=self.defaults.c_tas_pane,
        #         machine_config=self.c_machine_config,
        #         project_dir=self.defaults.default_vtas_dir_bare,
        #         ip=self.c_machine_config.ip,
        #         n_cores=10)
        # tas_config.args = tas_config.args + ' --vm-shm-len=4294967296'
        # self.c_tas_configs.append(tas_config)

        # self.c_proxyh_config = HostProxyConfig(pane=self.defaults.c_proxyh_pane,
        #         machine_config=self.c_machine_config,
        #         comp_dir=self.defaults.default_vtas_dir_bare)
        
        # vm0_config = VMConfig(pane=self.defaults.c_vm_pane,
        #         machine_config=self.c_machine_config,
        #         tas_dir=self.defaults.default_vtas_dir_bare,
        #         tas_dir_virt=self.defaults.default_vtas_dir_virt,
        #         idx=0,
        #         n_cores=3,
        #         memory=10)
        # self.c_vm_configs.append(vm0_config)

        # proxyg0_config = GuestProxyConfig(pane=self.defaults.c_proxyg_pane,
        #             machine_config=self.c_machine_config,
        #             comp_dir=self.defaults.default_vtas_dir_virt)
        # self.c_proxyg_configs.append(proxyg0_config)

        # for idx in range(self.cnodenum):
        #         port = 1230 + idx
        #         client_config = ClientConfig(exp_name=exp_name, 
        #                 pane=self.defaults.c_client_pane,
        #                 idx=0, vmid=idx, stack=self.cstack,
        #                 ip=self.s_vm_configs[idx].vm_ip, port=port, ncores=3,
        #                 msize=64, mpending=64, nconns=1000,
        #                 open_delay=3, max_msgs_conn=0, max_pend_conns=1,
        #                 bench_dir=self.defaults.default_vbenchmark_dir_virt,
        #                 tas_dir=self.defaults.default_vtas_dir_virt)
        #         self.client_configs.append(client_config)


        self.cstack = 'bare-tas'
        self.cnum = 3
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

        tas_config = TasConfig(pane=self.defaults.c_tas_pane,
                machine_config=self.c_machine_config,
                project_dir=self.defaults.default_otas_dir_bare,
                ip=self.c_machine_config.ip,
                n_cores=10)
        tas_config.args = tas_config.args
        self.c_tas_configs.append(tas_config)

        for idx in range(self.cnum):
                port = 1230 + idx
                client0_config = ClientConfig(exp_name=exp_name, 
                        pane=self.defaults.c_client_pane,
                        idx=idx, vmid=0, stack=self.cstack,
                        ip=self.s_vm_configs[idx].vm_ip, port=port, ncores=3,
                        msize=64, mpending=0, nconns=1000,
                        open_delay=0, max_msgs_conn=0, max_pend_conns=1,
                        bench_dir=self.defaults.default_obenchmark_dir_bare,
                        tas_dir=self.defaults.default_otas_dir_bare,
                        bursty=1, burst_length_mean=5, burst_interval_mean=20)
                self.client_configs.append(client0_config)



        # self.cstack = 'ovs-tas'
        # self.cnum = 1
        # self.cnodenum = 1
        # self.c_tas_configs = []
        # self.c_vm_configs = []
        # self.c_proxyg_configs = []
        # self.client_configs = []

        # self.c_machine_config = MachineConfig(ip=self.defaults.client_ip, 
        #         interface=self.defaults.client_interface,
        #         stack=self.cstack,
        #         is_remote=False,
        #         is_server=False)
        
        # vm0_config = VMConfig(pane=self.defaults.c_vm_pane,
        #         machine_config=self.c_machine_config,
        #         tas_dir=self.defaults.default_vtas_dir_bare,
        #         tas_dir_virt=self.defaults.default_vtas_dir_virt,
        #         idx=0,
        #         n_cores=22,
        #         memory=10,
        #         n_queues=10)
        # tas0_config = TasConfig(pane=self.defaults.c_tas_pane,
        #         machine_config=self.c_machine_config,
        #         project_dir=self.defaults.default_otas_dir_virt,
        #         ip=vm0_config.vm_ip,
        #         n_cores=1, dpdk_extra="00:03.0")
        # tas0_config.args = tas0_config.args + " --fp-no-rss --fp-no-xsumoffload"

        # self.c_tas_configs.append(tas0_config)
        # self.c_vm_configs.append(vm0_config)

        # client0_config = ClientConfig(exp_name=exp_name, 
        #         pane=self.defaults.c_client_pane,
        #         idx=0, vmid=0, stack=self.cstack,
        #         ip=self.s_vm_configs[0].vm_ip, port=1230, ncores=1,
        #         msize=64, mpending=64, nconns=1,
        #         open_delay=3, max_msgs_conn=0, max_pend_conns=1,
        #         bench_dir=self.defaults.default_obenchmark_dir_virt,
        #         tas_dir=self.defaults.default_otas_dir_virt)

        # self.client_configs.append(client0_config)