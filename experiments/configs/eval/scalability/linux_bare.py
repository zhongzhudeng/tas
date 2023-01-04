from configs.gen_config import HostConfig

class Config:
    def __init__(self):
        self.pane_prefix = 'e_'
        self.remote_connect_cmd = 'ssh swsnetlab04'

        self.server = HostConfig(
            name='server',
            is_server=True,
            is_remote=True,
            is_virt=False)

        self.client = HostConfig(
            name='client',
            is_server=False,
            is_remote=False,
            is_virt=False)

        self.snode = HostConfig(
            name='node',
            is_server=True,
            is_remote=True,
            is_virt=False)

        self.cnode = HostConfig(
            name='node',
            is_server=False,
            is_remote=False,
            is_virt=False)

        self.stype = 'bare'
        self.sstack = 'linux'
        self.snum = 1
        self.ctype = 'bare'
        self.cstack = 'linux'
        self.cnum = 3
        self.connum = 1
        self.msize = 64

        self.benchmark_server_args = "1234 1 foo 4096 1024"
        self.benchmark_client_args = "192.168.10.14 1234 1 foo " + \
                str(self.msize) + " 64 " + str(self.connum) + " 0 0 16"