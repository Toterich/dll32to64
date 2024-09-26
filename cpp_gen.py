import json
import os


class CppGen:
    def __init__(self, cfg_path: str):
        with open(cfg_path, 'r') as f:
            self._cfg = json.load(f)

        self._callbacks = {}
        for cb in self._cfg['callbacks']:
            self._callbacks[cb] = cb + "_inst"
        
    def code_gen(self, gen_tag: str) -> str:
        output = ''

        if gen_tag == 'BRIDGE_INCLUDE':
            header_name = os.path.basename(self._cfg["header"])
            output += f'#include "{header_name}"\n'

        elif gen_tag == 'BRIDGE_CALLBACK_DECL':
            for cb_type, cb_name in self._callbacks.items():
                output += f"{cb_type} {cb_name} = NULL;\n"

        elif gen_tag == 'BRIDGE_CALLBACK_IMPL':
            pass

        elif gen_tag == 'BRIDGE_IMPL':
            pass

        else:
            assert False
        
        return output