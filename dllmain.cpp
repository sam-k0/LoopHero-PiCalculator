// dllmain.cpp : Defines the entry point for the DLL application.
#include <Windows.h>
// YYTK is in this now
#include "MyPlugin.h"
#include "Assets.h"
#include "LHSprites.h"
// Plugin functionality
#include <fstream>
#include <iterator>
#include "LHCore.h"
#define _CRT_SECURE_NO_WARNINGS
#include <thread>
#include <atomic>
#include <iomanip>


std::atomic<bool> gCalcDone(false);
std::atomic<double> gInterimResult(0.0);
int gIter = 100000000;
double gPiResult = 0.0;
int printPrecision = 32;
std::thread gPiThread;
uint8_t FrameNumber = 0;

TRoutine gml_draw_set_halign;
TRoutine gml_draw_get_halign;
TRoutine gml_draw_text;
// Function to calculate pi using the Bailey–Borwein–Plouffe (BBP) formula
void calculatePi(int iterations, double& result) {
    result = 0.0;
    double a = 1e-100;
    double sm = 1;
    double sum = 1;

    for (int m = 1; m < gIter; m++)
    {
        sm *= m / (2.0 * m + 1);
        sum += sm;
        //Sleep(1000);
        gInterimResult.store(2*sum);

        std::ostringstream ss;
        ss << "Number of iterations: " << m << " result: " << std::fixed << std::setprecision(printPrecision) << 2 * sum;
        Misc::Print(ss.str());
        if (sm < a) // If there's only insignificant changes, abort
        {                        
            //break; 
        }
    }
    
    result = 2 * sum;

    gCalcDone.store(true);
}


// Unload function, remove callbacks here
YYTKStatus PluginUnload()
{
    PmRemoveCallback(CodeCallbackAttr);
    return YYTK_OK;
}

// Game events
// This function will get called with every game event
YYTKStatus ExecuteCodeCallback(YYTKCodeEvent* codeEvent, void*)
{
    CCode* codeObj = std::get<CCode*>(codeEvent->Arguments());
    CInstance* selfInst = std::get<0>(codeEvent->Arguments());
    CInstance* otherInst = std::get<1>(codeEvent->Arguments());


    // If we have invalid data???
    if (!codeObj)
        return YYTK_INVALIDARG;

    if (!codeObj->i_pName)
        return YYTK_INVALIDARG;

    // Do event specific stuff here.
    if (Misc::StringHasSubstr(codeObj->i_pName, "gml_Object_o_hero_Draw_0") )
    {

        std::ostringstream ss;
        ss << "Pi: " << std::fixed << std::setprecision(printPrecision) << gInterimResult.load();
        YYRValue hal;
        CallBuiltin(hal, "draw_get_halign", nullptr, nullptr, {});
        Misc::CallBuiltin("draw_set_halign", nullptr, nullptr, { 0.0 });
        Misc::CallBuiltin("draw_set_font", nullptr, nullptr, {4.0});
        Misc::CallBuiltin("draw_text", nullptr, nullptr, { 16.0,32.0, ss.str().c_str() });
        Misc::CallBuiltin("draw_set_halign", nullptr, nullptr, { hal });
    }

    if (Misc::StringHasSubstr(codeObj->i_pName, "gml_Room_rm_game_Create"))
    {

    }

    return YYTK_OK;
}

YYTKStatus FrameCallback(YYTKEventBase* pEvent, void* OptionalArgument)
{
    FrameNumber++;

    if (FrameNumber % 100 == 0)
    {
        double currentPi = gInterimResult.load();
        
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(printPrecision) << std::fabs(currentPi);
        std::string formattedPi = oss.str();

        // Get the absolute value to handle negative values
        double absPi = std::fabs(currentPi);
    
        // Create a string with the progress and formatted current Pi
        std::string progressString = "Current Pi: " + std::to_string(absPi);

        Misc::Print(progressString, CLR_AQUA);

        if (gCalcDone.load())
        {
            Misc::Print("Done", CLR_RED);
        }
    }      
    // Tell the core the handler was successful.
    return YYTK_OK;
}



bool CoreFoundCallback() // This function is ran once the core is resolved
                         // In this case, we want to wait with registering code callback until the core has registered itself to ensure safe calling
{

    // Create the thread to calculate pi
    gPiThread = std::thread(calculatePi, gIter, std::ref(gPiResult));
    
    PluginAttributes_t* pluginAttributes = nullptr;
    if (PmGetPluginAttributes(gThisPlugin, pluginAttributes) == YYTK_OK)
    { // Register a game event callback handler
        PmCreateCallback(pluginAttributes, CodeCallbackAttr, reinterpret_cast<FNEventHandler>(ExecuteCodeCallback), EVT_CODE_EXECUTE, nullptr);
    }

    Misc::Print("Registering the frame cb");

    { // Register a game event callback handler
        PmCreateCallback(pluginAttributes, FrameCallbackAttr, reinterpret_cast<FNEventHandler>(FrameCallback), static_cast<EventType>(EVT_PRESENT | EVT_ENDSCENE), nullptr);
    }

    // Get func ptrs
    GetFunctionByName("draw_get_halign", gml_draw_get_halign);
    GetFunctionByName("draw_set_halign", gml_draw_set_halign);
    GetFunctionByName("draw_text", gml_draw_text);

    gPiThread.join();
    return true;
}

// Entry
DllExport YYTKStatus PluginEntry(
    YYTKPlugin* PluginObject // A pointer to the dedicated plugin object
)
{
    gThisPlugin = PluginObject; // save that pointer
    gThisPlugin->PluginUnload = PluginUnload;

    // Check if the Callback Core Module is loaded, and wait for it to load
    // As soon as the core is resolved, the Callback function "CoreFoundCallback" will be triggered.
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LHCore::ResolveCore, static_cast<LHCore::CoreReady>(CoreFoundCallback), 0, NULL); 

    return YYTK_OK; // Successful PluginEntry.
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DllHandle = hModule; // save our module handle
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

