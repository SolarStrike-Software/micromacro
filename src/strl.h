#ifndef STRL_H
#define STRL_H

	#include <string>
	#include <cstddef>

	size_t strlcpy(char *dest, const char*src, size_t max_len);
	size_t strlcat(char *dest, const char*src, size_t max_len);
	int slprintf(char *dest, size_t size, const char *fmt, ...);

	void sztolower(char *dest, const char *src, size_t max_len);
	int wildfind(const std::string &format, const std::string &checkstring);
#endif
