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

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdarg.h>

#include "mp-ta.h"

#define MP_TA_BUFF_SIZE 256

#ifdef ENABLE_MP_TA
// internal func.
static void __free_cps(void);
static int __get_cp_index(char *name);

static void __free_accums(void);
static int __get_accum_index(char *name);

// global var.
mp_ta_checkpoint **mm_g_cps = NULL;
static int mm_g_cp_index = 0;

mp_ta_accum_item **mm_g_accums = NULL;
static int mm_g_accum_index = 0;
static int mm_g_accum_longest_name = 0;
static unsigned long mm_g_accum_first_time = 0xFFFFFFFF;	// jmlee
static int mm_g_enable = 1;

int
mp_ta_init(void)
{
	if(!ecore_file_exists(MP_TA_ENABLE_FILE))
		return -1;

	if (mm_g_accums)
		return 0;

	mm_g_cps = (mp_ta_checkpoint **) malloc(MP_TA_MAX_CHECKPOINT * sizeof(mp_ta_checkpoint *));
	if (!mm_g_cps)
		return -1;

	mm_g_accums = (mp_ta_accum_item **) malloc(MP_TA_MAX_CHECKPOINT * sizeof(mp_ta_accum_item *));
	if (!mm_g_accums)
		return -1;

	mm_g_accum_first_time = 0xFFFFFFFF;

	return 0;
}

int
mp_ta_release(void)
{
	if (!mm_g_accums)
		return 0;

	__free_cps();
	__free_accums();

	mm_g_accum_first_time = 0xFFFFFFFF;

	return 0;
}

void
mp_ta_set_enable(int enable)
{
	printf("MP_TA : setting enable to %d\n", enable);
	mm_g_enable = enable;
}

int
mp_ta_get_numof_checkpoints()
{
	return mm_g_cp_index;
}

char *
mp_ta_fmt(const char *fmt, ...)
{
	static char ta_buf[512];
	va_list args;

	memset(ta_buf, '\0', 512);

	va_start(args, fmt);
	vsnprintf(ta_buf, 512, fmt, args);
	va_end(args);

	return ta_buf;
}


int
mp_ta_add_checkpoint(char *name, int show, char *filename, int line)
{
	mp_ta_checkpoint *cp = NULL;
	struct timeval t;

	if (!mm_g_enable)
		return -1;

	if (!mm_g_accums)
		return 0;

	if (mm_g_cp_index == MP_TA_MAX_CHECKPOINT)
		return -1;

	if (!name)
		return -1;

	if (strlen(name) == 0)
		return -1;

	cp = (mp_ta_checkpoint *) malloc(sizeof(mp_ta_checkpoint));
	if (!cp)
		return -1;

	int name_len = strlen(name);
	cp->name = (char *)malloc(name_len + 1);
	if (!cp->name)
	{
		free(cp);
		return -1;
	}
	strncpy(cp->name, name, name_len);
	cp->name[name_len] = 0;
	if (show)
		printf("[CHECK-POINT] %s...(%s:%d)\n", name, filename, line);

	gettimeofday(&t, NULL);
	cp->timestamp = t.tv_sec * 1000000L + t.tv_usec;
#ifdef MP_TA_UNIT_MSEC
	cp->timestamp = (cp->timestamp >= 1000) ? cp->timestamp / 1000 : 0;
#endif

	mm_g_cps[mm_g_cp_index] = cp;

	mm_g_cp_index++;

	return 0;
}

void
mp_ta_show_checkpoints(void)
{
	int i = 0;
	if (!mm_g_accums)
		return;

	printf("BEGIN RESULT ============================\n");
	for (i = 0; i < mm_g_cp_index; i++)
	{
		printf("[%d] %s : %ld us.\n", i, mm_g_cps[i]->name, mm_g_cps[i]->timestamp);
	}
	printf("END RESULT   ============================\n");
}

void
mp_ta_show_diff(char *name1, char *name2)
{
	if (!mm_g_accums)
		return;


	printf("Time takes from [%s] to [%s] : %ld us.\n", name1, name2, mp_ta_get_diff(name1, name2));
}

unsigned long
mp_ta_get_diff(char *name1, char *name2)
{
	int cp1, cp2;

	if (!mm_g_accums)
		return 0;


	// fail if bad param.
	if (!name1 || !name2)
		return -1;

	// fail if same.
	if (strcmp(name1, name2) == 0)
		return -1;

	// get index
	if ((cp1 = __get_cp_index(name1)) == -1)
		return -1;

	if ((cp2 = __get_cp_index(name2)) == -1)
		return -1;

	// NOTE :
	// return value must be positive value.
	// bcz the value of higher index of mm_g_cps always higher than lower one.
	return mm_g_cps[cp2]->timestamp - mm_g_cps[cp1]->timestamp;

}

static int
__get_cp_index(char *name)
{
	int i;

	assert(name);

	// find index
	for (i = 0; i < mm_g_cp_index; i++)
	{
		if (strcmp(name, mm_g_cps[i]->name) == 0)
			return i;
	}

	return -1;
}

static int
__get_accum_index(char *name)
{
	int i;

	assert(name);

	// find index
	for (i = 0; i < mm_g_accum_index; i++)
	{
		if (strcmp(name, mm_g_accums[i]->name) == 0)
			return i;
	}

	return -1;
}

static void
__free_cps(void)
{
	int i = 0;

	if (!mm_g_cps)
		return;

	for (i = 0; i < mm_g_cp_index; i++)
	{
		if (mm_g_cps[i])
		{
			if (mm_g_cps[i]->name)
				free(mm_g_cps[i]->name);

			free(mm_g_cps[i]);

			mm_g_cps[i] = NULL;
		}
	}

	free(mm_g_cps);
	mm_g_cps = NULL;

	mm_g_cp_index = 0;
}

static void
__free_accums(void)
{
	int i = 0;

	if (!mm_g_accums)
		return;

	for (i = 0; i < mm_g_accum_index; i++)
	{
		if (mm_g_accums[i])
		{
			if (mm_g_accums[i]->name)
				free(mm_g_accums[i]->name);

			free(mm_g_accums[i]);

			mm_g_accums[i] = NULL;
		}
	}

	mm_g_accum_index = 0;
	mm_g_accum_longest_name = 0;

	free(mm_g_accums);
	mm_g_accums = NULL;
}


int
mp_ta_accum_item_begin(char *name, int show, char *filename, int line)
{
	mp_ta_accum_item *accum = NULL;
	int index = 0;
	int name_len = 0;
	struct timeval t;

	if (!mm_g_enable)
		return -1;

	if (!mm_g_accums)
		return 0;



	if (mm_g_accum_index == MP_TA_MAX_ACCUM)
		return -1;

	if (!name)
		return -1;

	name_len = strlen(name);
	if (name_len == 0)
		return -1;

	// if 'name' is new one. create new item.
	if ((index = __get_accum_index(name)) == -1)
	{
		accum = (mp_ta_accum_item *) malloc(sizeof(mp_ta_accum_item));
		if (!accum)
			return -1;

		// clear first.
		memset(accum, 0, sizeof(mp_ta_accum_item));
		accum->elapsed_min = 0xFFFFFFFF;

		accum->name = (char *)malloc(name_len + 1);
		if (!accum->name)
		{
			free(accum);
			return -1;
		}
		strncpy(accum->name, name, name_len);
		accum->name[name_len] = 0;
		// add it to list.
		mm_g_accums[mm_g_accum_index] = accum;
		mm_g_accum_index++;

		if (mm_g_accum_longest_name < name_len)
			mm_g_accum_longest_name = name_len;

	}
	else
	{
		accum = mm_g_accums[index];
	}

	// verify pairs of begin, end.
	if (accum->on_estimate)
	{
		printf("[%s] is not 'end'ed!\n", accum->name);
		accum->num_unpair++;
		return -1;
	}

	// get timestamp
	gettimeofday(&t, NULL);
	accum->timestamp = t.tv_sec * 1000000L + t.tv_usec;
#ifdef MP_TA_UNIT_MSEC
	accum->timestamp = (accum->timestamp >= 1000) ? accum->timestamp / 1000 : 0;
#endif
	accum->on_estimate = 1;

	if (accum->first_start == 0)
	{			// assum that timestamp never could be zero.
		accum->first_start = accum->timestamp;

		if (mm_g_accum_first_time > accum->first_start)
			mm_g_accum_first_time = accum->first_start;
	}

	if (show)
		printf("[ACCUM BEGIN] %s : %ld ---(%s:%d)\n", name, accum->timestamp, filename, line);

	accum->num_calls++;

	return 0;
}

int
mp_ta_accum_item_end(char *name, int show, char *filename, int line)
{
	mp_ta_accum_item *accum = NULL;
	unsigned int tval = 0;
	int index = 0;
	struct timeval t;

	if (!mm_g_enable)
		return -1;

	if (!mm_g_accums)
		return 0;


	// get time first for more accuracy.
	gettimeofday(&t, NULL);

	if (mm_g_accum_index == MP_TA_MAX_ACCUM)
		return -1;

	if (!name)
		return -1;

	if (strlen(name) == 0)
		return -1;

	// varify the 'name' is already exist.
	if ((index = __get_accum_index(name)) == -1)
	{
		printf("[%s] is not added before!\n", name);
		return -1;
	}

	accum = mm_g_accums[index];

	// verify pairs of begin, end.
	if (!accum->on_estimate)
	{
		printf("[%s] is not 'begin' yet!\n", accum->name);
		accum->num_unpair++;
		return -1;
	}

	// get current timestamp.
	tval = t.tv_sec * 1000000L + t.tv_usec;
#ifdef MP_TA_UNIT_MSEC
	tval = (tval >= 1000) ? tval / 1000 : 0;
#endif

	// update last_end
	accum->last_end = tval;

	// make get elapsed time.
	tval = tval - accum->timestamp;

	// update min/max
	accum->elapsed_max = tval > accum->elapsed_max ? tval : accum->elapsed_max;
	accum->elapsed_min = tval < accum->elapsed_min ? tval : accum->elapsed_min;

	if (show)
		printf("[ACCUM END] %s : %ld + %u ---(%s:%d)\n", name, accum->elapsed_accum, tval, filename, line);

	// add elapsed time
	accum->elapsed_accum += tval;
	accum->on_estimate = 0;

	return 0;
}

void
__print_some_info(FILE * fp)
{
	if (!fp)
		return;

	// comment
	{
		fprintf(fp, "\nb~ b~ b~\n\n");
	}

	// General infomation
	{
		time_t t_val;
		char hostname[MP_TA_BUFF_SIZE] = { '\0', };
		char buf[MP_TA_BUFF_SIZE] = {'\0', };
		struct utsname uts;
		struct rusage r_usage;

		fprintf(fp, "\n[[ General info ]]\n");

		// time and date
		time(&t_val);
		ctime_r(&t_val, buf);
		fprintf(fp, "Date : %s", buf);

		// system
		if (gethostname(hostname, 255) == 0 && uname(&uts) >= 0)
		{
			fprintf(fp, "Hostname : %s\n", hostname);
			fprintf(fp, "System : %s\n", uts.sysname);
			fprintf(fp, "Machine : %s\n", uts.machine);
			fprintf(fp, "Nodename : %s\n", uts.nodename);
			fprintf(fp, "Release : %s \n", uts.release);
			fprintf(fp, "Version : %s \n", uts.version);
		}

		// process info.
		fprintf(fp, "Process priority : %d\n", getpriority(PRIO_PROCESS, getpid()));
		getrusage(RUSAGE_SELF, &r_usage);
		fprintf(fp, "CPU usage : User = %ld.%06ld, System = %ld.%06ld\n",
			r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec,
			r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);


	}

	// host environment variables
	{
		extern char **environ;
		char **env = environ;

		fprintf(fp, "\n[[ Host environment variables ]]\n");
		while (*env)
		{
			fprintf(fp, "%s\n", *env);
			env++;
		}
	}
}

void
mp_ta_accum_show_result(int direction)
{
	int i = 0;
	char format[MP_TA_BUFF_SIZE];
	FILE *fp = stderr;

	if (!mm_g_accums)
		return;

	switch (direction)
	{
	case MP_TA_SHOW_STDOUT:
		fp = stdout;
		break;
	case MP_TA_SHOW_STDERR:
		fp = stderr;
		break;
	case MP_TA_SHOW_FILE:
		{
			fp = fopen(MP_TA_RESULT_FILE, "wt");
			if (!fp)
				return;
		}
	}
	__print_some_info(fp);

#ifdef MP_TA_UNIT_MSEC
	snprintf(format, sizeof(format),
		"[%%3d]| %%-%ds | \ttotal : %%4ld\tcalls : %%3ld\tavg : %%4ld\tmin : %%4ld\tmax : %%4ld\tstart : %%4lu\tend : %%4lu\tunpair : %%3ld\n",
		mm_g_accum_longest_name);
	fprintf(fp, "BEGIN RESULT ACCUM============================ : NumOfItems : %d, unit(msec)\n", mm_g_accum_index);
#else
	snprintf(format, sizeof(format),
		"[%%3d]| %%-%ds | \ttotal : %%ld\tcalls : %%ld\tavg : %%ld\tmin : %%ld\tmax : %%ld\tstart : %%lu\tend : %%lu\tunpair : %%ld\n",
		mm_g_accum_longest_name);
	fprintf(fp, "BEGIN RESULT ACCUM============================ : NumOfItems : %d, unit(usec)\n", mm_g_accum_index);
#endif

	for (i = 0; i < mm_g_accum_index; i++)
	{
		// prevent 'devide by zero' error
		if (mm_g_accums[i]->num_calls == 0)
			mm_g_accums[i]->num_calls = 1;

		fprintf(fp, format, i, mm_g_accums[i]->name, mm_g_accums[i]->elapsed_accum, mm_g_accums[i]->num_calls, (mm_g_accums[i]->elapsed_accum == 0) ? 0 : (int)(mm_g_accums[i]->elapsed_accum / mm_g_accums[i]->num_calls),	// Fix it! : devide by zero.
			mm_g_accums[i]->elapsed_min,
			mm_g_accums[i]->elapsed_max,
			mm_g_accums[i]->first_start - mm_g_accum_first_time,
			mm_g_accums[i]->last_end - mm_g_accum_first_time, mm_g_accums[i]->num_unpair);
	}
	fprintf(fp, "END RESULT ACCUM  ============================\n");

	if (direction == MP_TA_SHOW_FILE)
		fclose(fp);
}

bool mp_ta_is_init(void)
{
	return (bool)mm_g_accums;
}

#endif
//#endif        //_MM_TA_C_
