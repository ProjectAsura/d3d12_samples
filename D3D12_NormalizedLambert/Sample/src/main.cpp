//-----------------------------------------------------------------------------
// File : main.cpp
// Desc : Application Main Entry Point.
// Copyright(c) Pocol. All right reserved.
//-----------------------------------------------------------------------------

#if defined(DEBUG) || defined(_DEBUG)
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
#endif//defined(DEBUG) || defined(_DEBUG)

//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "SampleApp.h"


//-----------------------------------------------------------------------------
//      メインエントリーポイントです.
//-----------------------------------------------------------------------------
int wmain(int argc, wchar_t** argv, wchar_t** envp)
{
#if defined(DEBUG) || defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif//defined(DEBUG) || defined(_DEBUG)

    SampleApp(960, 540).Run();
    return 0;
}