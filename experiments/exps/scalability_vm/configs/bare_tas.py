from exps.scalability_vm.helpers import *
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
    def __init__(self, exp_name, n_apps):
        self.exp_name = exp_name
        self.defaults = Defaults()
        app_cores = 2

        # Configure Csets
        tas_cores = [1,3,5,7,9,11,13,15,17,19,21,23,25,27,29,31,33,35,37,39,41,43]
        app_cores = list(range(0,44))
        skip_cores = tas_cores

        self.s_cset_configs = []
        self.c_cset_configs = []
        tas_cset = CSetConfig(tas_cores, "0-1", "tas", exclusive=False)
        self.s_cset_configs.append(tas_cset)
        self.c_cset_configs.append(tas_cset)

        # app_cset = create_app_csets(n_apps, app_cores, skip_cores=skip_cores, exclusive=False)
        # self.c_cset_configs = self.c_cset_configs + app_cset
        # self.s_cset_configs = self.s_cset_configs + app_cset
        # app_cset = CSetConfig(app_cores, "0-1", "app", exclusive=False)
        # self.c_cset_configs.append(app_cset)
        # self.s_cset_configs.append(app_cset)

        # Server Machine
        self.sstack = 'bare-tas'
        self.snum = n_apps
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

        self.s_cset_configs.append(tas_cset)

        tas_config = TasConfig(pane=self.defaults.s_tas_pane,
                               machine_config=self.s_machine_config,
                               project_dir=self.defaults.default_otas_dir_bare,
                               ip=self.s_machine_config.ip,
                               cset="tas",
                               n_cores=4)
        tas_config.args = tas_config.args + " --shm-len=4294967296"
        self.s_tas_configs.append(tas_config)

        for idx in range(self.snum):
            port = 1230 + idx
            server_config = ServerConfig(pane=self.defaults.s_server_pane,
                                         idx=idx, vmid=1,
                                         port=port, ncores=1, max_flows=4096, max_bytes=4096,
                                         cset=None,
                                         bench_dir=self.defaults.default_obenchmark_dir_bare,
                                         tas_dir=self.defaults.default_otas_dir_bare)
            self.server_configs.append(server_config)

        # Client Machine
        self.cstack = 'bare-tas'
        self.cnum = n_apps
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
                               cset="tas",
                               n_cores=4)
        tas_config.args = tas_config.args + " --shm-len=4294967296"
        self.c_tas_configs.append(tas_config)

        for idx in range(self.cnum):
            port = 1230 + idx
            client_config = ClientConfig(exp_name=exp_name,
                                         pane=self.defaults.c_client_pane,
                                         idx=0, vmid=idx, stack=self.cstack,
                                         ip=self.s_machine_config.ip, port=port, ncores=1,
                                         msize=64, mpending=64, nconns=100,
                                         open_delay=10, max_msgs_conn=0, max_pend_conns=1,
                                         cset=None,
                                         bench_dir=self.defaults.default_obenchmark_dir_bare,
                                         tas_dir=self.defaults.default_otas_dir_bare)
            self.client_configs.append(client_config)
