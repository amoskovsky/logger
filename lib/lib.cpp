// lib.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "logger.h"
using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
    logger::setup("test.log", true, 3);
    logger::truncate();
    trace_log << "test5" << endl;
    debug_log << "test4" << endl;
    info_log << "test3" << endl;
    warn_log << "test2" << endl;
    error_log << "test1" << endl;
	return 0;
}

