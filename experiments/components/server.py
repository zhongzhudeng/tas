import utils
import time

class Server:
    
    def __init__(self, defaults, machine_config, 
            server_config, vm_config, wmanager):
        self.defaults = defaults
        self.machine_config = machine_config
        self.server_config = server_config
        self.vm_config = vm_config
        self.wmanager = wmanager
        self.log_paths = []
        self.pane = self.wmanager.add_new_pane(server_config.pane,
            machine_config.is_remote)
    
    def run_bare(self, w_sudo, ld_preload):
        self.run_benchmark_rpc(w_sudo, ld_preload, clean=False, cset=True)

    def run_virt(self, w_sudo, ld_preload):
        ssh_com = utils.get_ssh_command(self.machine_config, self.vm_config)
        self.pane.send_keys(ssh_com)
        time.sleep(3)
        self.pane.send_keys("tas")
        self.run_benchmark_rpc(w_sudo, ld_preload, clean=False, cset=False)

    def run_benchmark_rpc(self, w_sudo, ld_preload, clean, cset):
        self.pane.send_keys('cd ' + self.server_config.comp_dir)

        if clean:
            self.pane.send_keys(self.server_config.clean_cmd)
            time.sleep(1)

        self.pane.send_keys(self.server_config.comp_cmd)
        time.sleep(1)
        self.pane.send_keys("cd " + self.server_config.tas_dir)
        time.sleep(3)

        cmd = ''
        
        # Keep application on even cores, so it's the same NUMA node as TAS
        if w_sudo:
            cmd += 'sudo -E '
        
        if ld_preload:
            cmd += 'LD_PRELOAD=' + self.server_config.lib_so + ' '

        # Keep application on even cores, so it's the same NUMA node as TAS
        if cset:
            cmd += "sudo cset proc --set={} --exec ".format(self.server_config.cset)
            cmd += self.server_config.exec_file + ' -- '
            cmd += self.server_config.args + ' | tee ' + self.server_config.out
        else:
            cmd += self.server_config.exec_file + ' ' + \
                    self.server_config.args 
                    # + \
                    # ' | tee ' + \
                    # self.server_config.out

        self.pane.send_keys(cmd)