#pragma once
#include <Windows.h>
#include "MyPlugin.h"

namespace LHCore {


typedef bool (*CoreReady)();
CoreReady pCoreReady = nullptr;
bool isCoreReady = false;

DWORD WINAPI ResolveCore(CoreReady pCallback)
{
    Misc::Print("Importing Core function");
    void* rawCoreReady;

    while (true)
    {
        Sleep(10);
        if (PmGetExported("CoreReady", rawCoreReady) == YYTK_OK)
        {
            pCoreReady = reinterpret_cast<CoreReady>(rawCoreReady);
            if (pCoreReady() == true)
            {
                Misc::Print("Core is present", CLR_GREEN);
                isCoreReady = true;

                // Callback function
                if (pCallback != nullptr)
                {
                    pCallback(); // maybe use this functions return val as return?
                }

                return TRUE;
            }
        }
        Misc::Print("Waiting for Core. Did you install LoopHeroCallbackCore.dll?", CLR_RED);
    }
}

}