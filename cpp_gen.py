import json

class CppGen:
    def __init__(self, cfg: str):
        with open(cfg, 'r') as f:
            self._cfg = json.load(f)
        
        # TODO: This will be determined from the header itself later on
        self._callbacks = {}
        for cb in self._cfg['callbacks']:
            self._callbacks[cb] = cb + "_inst"
        
    def code_gen(self, gen_tag: str) -> str:
        output = ''

        if gen_tag == 'BRIDGE_INCLUDE':
            output += f'#include "{self._cfg["header_name"]}"\n'

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