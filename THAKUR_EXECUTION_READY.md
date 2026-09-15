

- `executeScript()` calls `SafeCallDoString()` using `m_pfnDoString` + `m_luaState`
- Named pipe (`\pipe\ThakurIPC`) delivers payload from UI
- Fallback patterns in `patterns.json` exist
- Loader-lock fixed (`DllMain` detached thread)
- Crash guards present (timeout watchdog, exception handler, null-pointer abort)

1. Verify `luaL_dostring` + `lua_State` patterns match YOUR Roblox build
2. Build `thakur_runtime.dll`
3. Click Attach → script runs through `luaL_dostring()` inside Roblox

Execution engine is live. Pattern accuracy = only blocker.
