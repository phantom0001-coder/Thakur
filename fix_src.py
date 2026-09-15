import os
import re

def fix_script_hub():
    path = r'thakur_ui\src\script_hub.cpp'
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # fix includes
    if '#include <wininet.h>' not in content:
        content = content.replace('#include "script_hub.h"', '#include <windows.h>\n#include <wininet.h>\n#include <regex>\n#include "script_hub.h"')
    
    # fix https: issue at top
    content = re.sub(r'\"internal:\r?\n', r'"internal://all",\n', content)
    content = re.sub(r'\"https:\r?\n\s*\{\"scripthub', r'"https://robloxscripts.com"},\n    {"scripthub', content)
    content = re.sub(r'\"https:\r?\n\s*\{\"rscripts', r'"https://scripthub.com"},\n    {"rscripts', content)
    content = re.sub(r'\"https:\r?\n\s*\{\"scriptblox', r'"https://rscripts.net"},\n    {"scriptblox', content)
    content = re.sub(r'\"https:\r?\n\s*\}\;', r'"https://scriptblox.com"}\n};', content)

    # fix loadstring(game:HttpGet('https:
    content = re.sub(r'\"loadstring\(game:HttpGet\(\'https:\r?\n\s*\"https:\r?\n', r'"loadstring(game:HttpGet(\'https://example.com/script.lua\'))()",\n        "https://example.com/script.lua",\n', content)
    
    # remaining isolated https:
    content = re.sub(r'\"https:\r?\n', r'"https://example.com",\n', content)

    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

def fix_ipc():
    path = r'thakur_ui\src\ipc\ipc_client.cpp'
    with open(path, 'r', encoding='utf-8') as f:
        content = f.read()
    if '#include <sstream>' not in content:
        content = content.replace('#include "ipc_client.h"', '#include "ipc_client.h"\n#include <sstream>')
    with open(path, 'w', encoding='utf-8') as f:
        f.write(content)

fix_script_hub()
fix_ipc()
