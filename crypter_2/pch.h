#pragma once

#include <windows.h>
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include <iostream>
#include <vector>
#include <string>
#include <commdlg.h>
#include <fstream>
#include <memory>
#include <thread>
#include <future>
#include <openssl/evp.h>
#include <openssl/aes.h>
