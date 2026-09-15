

1. Pattern signatures outdated / hardcoded — `thakur_engine.cpp` scans `TaskScheduler` bytes that break on Roblox updates
2. Loader-lock violation — `thakur_runtime.cpp` / `thakur_engine.cpp` call `VirtualQuery`, `GetModuleInformation`, memory scanning inside DLL entry
3. Manual mapping has no validation before `CreateRemoteThread`
4. `findRobloxProcesses()` picks first PID blindly
5. `patterns.json` not refreshed
6. No graceful abort when scan returns 0; writes proceed anyway

- [ ] Refresh `thakur_ui/src/injector/patterns.json` for current Roblox build
- [ ] Update byte signatures in `thakur_engine.cpp` (line 63-65) and `thakur_runtime.cpp`
- [ ] Verify `findRobloxProcesses()` PID matches `robloxplayerbeta.exe` exactly

- [ ] In `manualMap()`: abort if `VirtualAllocEx` returns NULL; don't fall through
- [ ] In `inject()`: if `method == InjectionMethod::ProcessHollow`, validate DLL bytes before mapping
- [ ] Add error message for `InjectionStatus::MappingFailed` (line 133-135)

- [ ] Move `scanMemory()` / `findTaskScheduler()` out of `DllMain`; spawn a worker thread that waits for loader-lock exit
- [ ] Add guard: if `scheduler == 0`, return clean abort instead of dereferencing
- [ ] Wrap `resolveImports()` with error check; return false on failure

- [ ] Confirm Named Pipe (`\\.\pipe\ThakurIPC`) is open before sending script payload
- [ ] Verify `lua_State*` pointer scan succeeds in `LuaHookEngine::scanSignatures()`
- [ ] Execute script only after hook engine reports `SUCCESS`

- [ ] Rebuild `thakur_runtime.dll` after pattern updates
- [ ] Re-run `build_thakur.bat` with updated MSVC / WebView2 SDK
- [ ] Test on non-critical Roblox session first
