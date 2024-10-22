// SPDX-License-Identifier: MIT
/*
 *
 * This file is part of rurima, with ABSOLUTELY NO WARRANTY.
 *
 * MIT License
 *
 * Copyright (c) 2024 Moe-hacker
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *
 */
#include "include/rurima.h"
static char *del_all_comments(const char *buf)
{
	char *ret = malloc(strlen(buf));
	size_t j = 0;
	bool in_string = false;
	for (size_t i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\\') {
			i++;
			continue;
		} else if (buf[i] == '"') {
			in_string = !in_string;
		} else if (!in_string && buf[i] == '/' && i + 1 < strlen(buf) && buf[i + 1] == '/') {
			while (i < strlen(buf) && buf[i] != '\n') {
				i++;
			}
			i++;
		} else if (!in_string && buf[i] == '/' && i + 1 < strlen(buf) && buf[i + 1] == '*') {
			i += 2;
			while (!in_string && buf[i] != '*' && i + 1 < strlen(buf) && buf[i + 1] != '/') {
				i++;
			}
			i += 2;
			if (i + 1 < strlen(buf)) {
				i++;
			}
		}
		ret[j] = buf[i];
		ret[j + 1] = '\0';
		j++;
	}
	return ret;
}
static char *next_key(const char *buf)
{
	const char *p = buf;
	// Reach the first key.
	for (size_t i = 0; i < strlen(p); i++) {
		if (p[i] == '\\') {
			i++;
			continue;
		} else if (p[i] == '"') {
			p = &p[i];
			break;
		}
	}
	int level = 0;
	bool in_string = false;
	// Get the next key.
	for (size_t i = 0; i < strlen(p); i++) {
		if (p[i] == '\\') {
			i++;
			continue;
		} else if (p[i] == '"') {
			in_string = !in_string;
		} else if ((p[i] == '{' || p[i] == '[') && !in_string) {
			level++;
		} else if ((p[i] == '}' || p[i] == ']') && !in_string) {
			level--;
			if (level == -1) {
				return NULL;
			}
		} else if (p[i] == ',' && !in_string && level == 0) {
			if (i < strlen(p) - 1) {
				return (char *)&p[i + 1];
			} else {
				return NULL;
			}
		}
	}
	return NULL;
}
static char *current_key(const char *buf)
{
	/*
	 * Warning: free() after use.
	 */
	char *tmp = strdup(buf);
	char *ret = NULL;
	// Skip space.
	for (size_t i = 0; i < strlen(tmp); i++) {
		if (tmp[i] == '\\') {
			i++;
			continue;
		} else if (tmp[i] == '"') {
			ret = &tmp[i + 1];
			break;
		}
	}
	if (ret == NULL) {
		free(tmp);
		return NULL;
	}
	for (size_t i = 0; i < strlen(ret); i++) {
		if (ret[i] == '\\') {
			i++;
			continue;
		} else if (ret[i] == '"' && i < strlen(ret)) {
			ret[i] = '\0';
			break;
		}
		if (i == strlen(ret) - 1) {
			free(tmp);
			return NULL;
		}
	}
	ret = strdup(ret);
	free(tmp);
	return ret;
}
static char *current_value(const char *buf)
{
	/*
	 * Warning: free() after use.
	 */
	int level = 0;
	char *tmp = strdup(buf);
	char *ret = NULL;
	// Skip key.
	for (size_t i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\\') {
			i++;
			continue;
		} else if (buf[i] == ':') {
			ret = &tmp[i + 1];
			break;
		}
	}
	if (ret == NULL) {
		free(tmp);
		return NULL;
	}
	// Skip space.
	for (size_t i = 0; i < strlen(ret); i++) {
		if (ret[i] == '\\') {
			i++;
			continue;
		} else if (ret[i] != ' ' && ret[i] != '\n') {
			ret = &ret[i];
			break;
		}
	}
	bool in_string = false;
	for (size_t i = 0; i < strlen(ret); i++) {
		if (ret[i] == '\\') {
			i++;
			continue;
		} else if (ret[i] == '"') {
			in_string = !in_string;
		} else if ((ret[i] == '{' || ret[i] == '[') && !in_string) {
			level++;
		} else if ((ret[i] == '}' || ret[i] == ']') && !in_string) {
			level--;
			// The last value might not have a comma.
			if (level == -1) {
				ret[i] = '\0';
				break;
			}
		} else if (ret[i] == ',' && !in_string && level == 0) {
			if (i < strlen(ret) - 1) {
				ret[i] = '\0';
				break;
			} else {
				free(tmp);
				return NULL;
			}
		}
	}
	ret = strdup(ret);
	free(tmp);
	return ret;
}
static char *json_get_key_one_level(const char *buf, char *key)
{
	const char *p = buf;
	while (p != NULL) {
		char *current = current_key(p);
		if (current == NULL) {
			break;
		}
		if (strcmp(current, key) == 0) {
			char *ret = current_value(p);
			free(current);
			return ret;
		}
		free(current);
		p = next_key(p);
	}
	return NULL;
}
char *json_get_key(const char *buf, char *key)
{
	if (buf == NULL || key == NULL) {
		return NULL;
	}
	char *keybuf = malloc(strlen(key));
	char *tmp = del_all_comments(buf);
	char *ret;
	for (size_t i = 0; i < strlen(key); i++) {
		if (key[i] == '[') {
			for (size_t j = i + 1; j < strlen(key); j++) {
				if (key[j] == ']') {
					ret = json_get_key_one_level(tmp, keybuf);
					free(tmp);
					tmp = ret;
					break;
				}
				keybuf[j - i - 1] = key[j];
				keybuf[j - i] = '\0';
			}
		}
	}
	free(keybuf);
	return ret;
}