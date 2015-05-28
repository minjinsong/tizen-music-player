/*
 * Copyright 2012	  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef __UG_MP_DEBUG_H_
#define __UG_MP_DEBUG_H_


#include <stdio.h>
#include <unistd.h>
#include "assert.h"
#include <linux/unistd.h>

#define ENABLE_CHECK_START_END_FUNCTION	// support enter leave debug message

#define ENABLE_LOG_SYSTEM

#ifdef ENABLE_LOG_SYSTEM

#define USE_DLOG_SYSTEM

#define gettid() syscall(__NR_gettid)

#ifdef USE_DLOG_SYSTEM
#include <dlog.h>

#ifdef LOG_TAG
#undef LOG_TAG
#endif //LOG_TAG

#define LOG_TAG "UG_MUSIC_PLAYER"

#ifndef TRUE
#define TRUE 1
#endif

#define LOG_COLOR_RESET    "\033[0m"
#define LOG_COLOR_RED      "\033[31m"
#define LOG_COLOR_YELLOW   "\033[33m"
#define LOG_COLOR_GREEN  	"\033[32m"
#define LOG_COLOR_BLUE		"\033[36m"

#define DEBUG_TRACE(fmt, arg...)	LOGD_IF(TRUE,  LOG_COLOR_GREEN"[TID:%d]   "fmt""LOG_COLOR_RESET, gettid(), ##arg)
#define INFO_TRACE(fmt, arg...) 	LOGI_IF(TRUE,  LOG_COLOR_GREEN"[TID:%d]    "fmt""LOG_COLOR_RESET, gettid(), ##arg)
#define WARN_TRACE(fmt, arg...) 	LOGW_IF(TRUE,  LOG_COLOR_YELLOW"[TID:%d]   "fmt""LOG_COLOR_RESET, gettid(), ##arg)
#define ERROR_TRACE(fmt, arg...)	LOGE_IF(TRUE,  LOG_COLOR_RED"[TID:%d]   "fmt""LOG_COLOR_RESET, gettid(), ##arg)

#else // use USE_DLOG_SYSTEM

#define DEBUG_TRACE(fmt, arg...) do{fprintf(stderr, "[%s : %s-%d]\t - \n", __FILE__, __func__, __LINE__);\
	    fprintf (stderr, __VA_ARGS__);}while(0)
#define INFO_TRACE(fmt, arg...) do{fprintf(stderr, "[%s : %s-%d]\t - \n", __FILE__, __func__, __LINE__);\
	    fprintf (stderr, __VA_ARGS__);}while(0)
#define WARN_TRACE(fmt, arg...) do{fprintf(stderr, "[%s : %s-%d]\t - \n", __FILE__, __func__, __LINE__);\
	    fprintf (stderr, __VA_ARGS__);}while(0)
#define ERROR_TRACE(fmt, arg...) do{fprintf(stderr, "[%s : %s-%d]\t - \n", __FILE__, __func__, __LINE__);\
	    fprintf (stderr, __VA_ARGS__);}while(0)
#endif //USE_DLOG_SYSTEM

#define DEBUG_TRACE_FUNC() DEBUG_TRACE("")

#else //ENABLE_LOG_SYSTEM
#define DEBUG_TRACE(fmt, arg...)
#define INFO_TRACE(fmt, arg...)
#define WARN_TRACE(fmt, arg...)
#define ERROR_TRACE(fmt, arg...)
#endif //ENABLE_LOG_SYSTEM

#ifdef ENABLE_CHECK_START_END_FUNCTION
#define startfunc   		DEBUG_TRACE("+-  START -------------------------");
#define endfunc     		DEBUG_TRACE("+-  END  --------------------------");
#define exceptionfunc	ERROR_TRACE("### CRITICAL ERROR   ###");
#else
#define startfunc
#define endfunc
#define exceptionfunc
#endif

#define mp_ret_if(expr) do { \
	if(expr) { \
		WARN_TRACE("");\
		return; \
	} \
} while (0)
#define mp_retv_if(expr, val) do { \
	if(expr) { \
		WARN_TRACE("");\
		return (val); \
	} \
} while (0)

#define mp_retm_if(expr, fmt, arg...) do { \
	if(expr) { \
		WARN_TRACE(fmt, ##arg); \
		return; \
	} \
} while (0)

#define mp_retvm_if(expr, val, fmt, arg...) do { \
	if(expr) { \
		WARN_TRACE(fmt, ##arg); \
		return (val); \
	} \
} while (0)

#define CHECK_EXCEP(expr) do { \
	if(!(expr)) { \
		ERROR_TRACE("CRITICAL ERROR ## CHECK BELOW ITEM");\
		goto mp_exception;\
	} \
} while (0)

#define MP_CHECK_VAL(expr, val) 		mp_retvm_if(!(expr),val,"INVALID PARM RETURN VAL: 0x%x", val)
#define MP_CHECK_NULL(expr) 		mp_retvm_if(!(expr),NULL,"INVALID PARM RETURN NULL")
#define MP_CHECK_FALSE(expr) 		mp_retvm_if(!(expr),FALSE,"INVALID PARM RETURN FALSE")
#define CHECK_CANCEL(expr) 		mp_retvm_if(!(expr), ECORE_CALLBACK_CANCEL, "INVALID PARAM RETURN")
#define MP_CHECK(expr) 				mp_retm_if(!(expr),"INVALID PARAM RETURN")

#define mp_assert(expr) do { \
	if(!(expr)) { \
		ERROR_TRACE("CRITICAL ERROR ## CHECK BELOW ITEM");\
		assert(FALSE); \
	} \
} while (0)


#endif // __UG_MP_DEBUG_H_
