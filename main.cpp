// Cube World Alpha Server - IdleCPUFix
//
// Server.exe runs a world/zone management thread (entry base+0x149550) whose
// main loop never sleeps or waits. Once the zones around all players are
// loaded (or when nobody is connected) it keeps spinning, pinning one CPU core
// at 100%.
//
// This mod hooks the tail of that loop and inserts Sleep(10) on every pass.

// ---- build requirements -----------------------------------------------------
#include <stdint.h>   // UINTPTR_MAX - pointer width, checked portably

#if !defined(_WIN32)
#  error "Target must be Windows: this is a DLL injected into Server.exe. Cross-compile, e.g. i686-w64-mingw32-g++."
#endif
#if UINTPTR_MAX != 0xFFFFFFFFu
#  error "Target must be 32-bit: Server.exe is PE32."
#endif
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#  error "GCC 8 or newer: older GCC silently IGNORES __attribute__((naked)) on x86 and builds a broken DLL."
#endif

#include <windows.h>
#include <stdio.h>

// Loop tail in Server.exe (image base 0x400000 -> VA 0x549AEF):
//   +0x149AEF  8B 43 08     mov eax, [ebx+8]     <- hook (jmp rel32 + nop)
//   +0x149AF2  83 C4 04     add esp, 4
//   +0x149AF5  80 38 00     cmp byte [eax], 0    <- jump back here
//   +0x149AF8  0F 85 ...    jne loop_start (+0x1495A0)
static const uint32_t HOOK_OFFSET = 0x149AEF;
static const uint32_t RETURN_OFFSET = 0x149AF5;
static const BYTE EXPECTED_BYTES[] = { 0x8B, 0x43, 0x08, 0x83, 0xC4, 0x04 };

extern "C" {
    uint32_t jmp_back = 0;
    uint32_t sleep_fn = 0;
}

// Naked: no prologue, so the loop's registers and stack stay untouched.
// EFLAGS is not saved on purpose - we return to a cmp, which sets it anew.
extern "C" __attribute__((naked)) void HookStub()
{
    asm("add esp, 4");                          // original code

    asm("pushad");                              // push registers to stack
    asm("push 10");                             // push parameter for next function call
    asm("call dword ptr [_sleep_fn]");          // call sleep (in ms), stdcall pops parameter set in the line above
    asm("popad");                               // pops stack back to registers

    asm("mov eax, dword ptr [ebx+8]");          // original code
    asm("jmp dword ptr [_jmp_back]");           // -> cmp byte [eax], 0
}

static bool WriteHook(BYTE* location, void* target)
{
    DWORD oldProtection;
    if (!VirtualProtect(location, sizeof(EXPECTED_BYTES), PAGE_EXECUTE_READWRITE, &oldProtection)) {
        return false;
    }
    location[0] = 0xE9; // jmp rel32
    *(uint32_t*)(location + 1) = (uint32_t)target - (uint32_t)location - 5;
    for (size_t i = 5; i < sizeof(EXPECTED_BYTES); i++) location[i] = 0x90; // no-op the rest of add esp, 4
    VirtualProtect(location, sizeof(EXPECTED_BYTES), oldProtection, &oldProtection);
    FlushInstructionCache(GetCurrentProcess(), location, sizeof(EXPECTED_BYTES));
    return true;
}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason != DLL_PROCESS_ATTACH) {
        return TRUE;
    }

    uint32_t base = (uint32_t)GetModuleHandle(NULL);
    BYTE* hook = (BYTE*)(base + HOOK_OFFSET);

    // Refuse to patch anything that is not the expected Server.exe build.
    if (memcmp(hook, EXPECTED_BYTES, sizeof(EXPECTED_BYTES)) != 0) {
        printf("[IdleCPUFix] Unexpected code at hook location, not patching. Is this Server.exe 0.1.1?\n");
        return TRUE;
    }

    sleep_fn = (uint32_t)GetProcAddress(GetModuleHandle("kernel32.dll"), "Sleep");
    jmp_back = base + RETURN_OFFSET;

    if (sleep_fn && WriteHook(hook, (void*)&HookStub)) {
        printf("[IdleCPUFix] World thread idle loop patched.\n");
    } else {
        printf("[IdleCPUFix] Failed to install hook.\n");
    }
    return TRUE;
}
