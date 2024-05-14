/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef IM_BASE_TYPE_H
#define IM_BASE_TYPE_H

#include <math.h>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <libgen.h>
#include <dlfcn.h>
#include <signal.h>
#include <memory>
#include <limits>
#include <locale>
#include <atomic>
#include <string>
#include <list>
#include <queue>
#include <deque>
#include <set>
#include <map>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <thread>
#include <istream>
#include <ostream>
#include <fstream>

#include "securec.h"

#ifdef __LP64__
#define INT64 long int
#define UINT64 unsigned long int
#else
#define INT64 long long int
#define UINT64 unsigned long long int
#endif
#endif // IM_BASE_TYPE_H