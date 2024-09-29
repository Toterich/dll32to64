import json
import os


class CppGen:
    def __init__(self, cfg_path: str):
        with open(cfg_path, 'r') as f:
            self._cfg = json.load(f)

        self._code = {}

        # PROTOCOL_H_MSGIDS
        self._code["PROTOCOL_H_MSGIDS"] = ""
        last_fun = None
        for f_name, f_data in self._cfg["functions"].items():
            last_fun = f_name
            self._code["PROTOCOL_H_MSGIDS"] += f"    MSGID_{f_name},\n"
        self._code["PROTOCOL_H_MSGIDS"] += f"    MSGID_LAST = MSGID_{last_fun}\n"


        # BRIDGE_INCLUDE
        header_name = os.path.basename(self._cfg["header"])
        self._code['BRIDGE_INCLUDE'] = f'#include "{header_name}"\n'

        # BRIDGE_CALLBACK_DECL
        bridge_cb_decl = ""
        self._callbacks = {}
        for cb in self._cfg['callbacks']:
            bridge_cb_decl += f"{cb} {cb + "_instance"} = NULL;\n"
            self._callbacks[cb] = cb + "_instance"
        self._code['BRIDGE_CALLBACK_DECL'] = bridge_cb_decl

        #BRIDGE_CALLBACK_IMPL

