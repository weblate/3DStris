/**
 * Copyright (c) 2017 rxi
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See `log.c` for details.
 */

#pragma once

#include <3ds.h>
#include <stdio.h>

enum Level { TRACE, DEBUG, INFO, WARN, ERROR, FATAL };

#define LOG_TRACE(...) Log::get().log(TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) Log::get().log(DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) Log::get().log(INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) Log::get().log(WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) Log::get().log(ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) Log::get().log(FATAL, __FILE__, __LINE__, __VA_ARGS__)

class Log {
   public:
	static Log& get() {
		static Log log;
		return log;
	}

	void setFile(FILE* fp);
	void setLevel(const Level level);
	void setQuiet(const bool enable);

	void log(const Level level, const char* file, int line,
			 const char* string) {
		log(level, file, line, "%s", string);
	}

	template <typename... Args>
	void log(const Level level, const char* file, int line, const char* fmt,
			 Args&&... args) {
		static const char* LEVELS[] = {"TRACE", "DEBUG", "INFO",
									   "WARN",	"ERROR", "FATAL"};

		if (level < this->level) {
			return;
		}

		// Get current time
		const time_t t = (osGetTime() - 2208988800000) / 1000;
		const tm* lt = localtime(&t);

		// Log to stderr
		if (!quiet) {
			char buf[16];
			buf[strftime(buf, sizeof buf, "%H:%M:%S", lt)] = '\0';
			fprintf(stderr, "%s %-5s %s:%d: ", buf, LEVELS[level], file, line);
			fprintf(stderr, fmt, args...);
			fputs("\n", stderr);
			fflush(stderr);
		}

		// Log to file
		if (fp) {
			char buf[32];
			buf[strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", lt)] = '\0';
			fprintf(fp, "%s %-5s %s:%d: ", buf, LEVELS[level], file, line);
			fprintf(fp, fmt, args...);
			fputs("\n", fp);
			fflush(fp);
		}
	}

   private:
	Log();
	~Log();

	Level level;
	FILE* fp;
	bool quiet = true;
};
