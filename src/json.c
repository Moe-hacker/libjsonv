// SPDX-License-Identifier: MIT
/*
 *
 * This file is part of libjsonv, with ABSOLUTELY NO WARRANTY.
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
#include "include/jsonv.h"
/*
 * If there's no bugs, do not care how it works,
 * just use it.
 */
static char *unicode_to_char(const char *str)
{
	/*
	 * Warning: free() after use.
	 */
	size_t len = strlen(str);
	char *result = malloc(len + 1);
	if (!result) {
		return NULL;
	}
	size_t j = 0;
	for (size_t i = 0; i < len; i++) {
		if (str[i] == '\\' && i < len - 5 && str[i + 1] == 'u' && isxdigit(str[i + 2]) && isxdigit(str[i + 3]) && isxdigit(str[i + 4]) && isxdigit(str[i + 5])) {
			char hex[5] = { str[i + 2], str[i + 3], str[i + 4], str[i + 5], '\0' };
			result[j++] = (char)strtol(hex, NULL, 16);
			i += 5;
		} else {
			result[j++] = str[i];
		}
	}
	result[j] = '\0';
	return result;
}
static char *format_json(const char *buf)
{
	/*
	 * Warning: free() after use.
	 */
	char *tmp = unicode_to_char(buf);
	char *ret = malloc(strlen(tmp) + 1);
	size_t j = 0;
	bool in_string = false;
	for (size_t i = 0; i < strlen(tmp); i++) {
		if (tmp[i] == '\\') {
			i++;
			continue;
		} else if (tmp[i] == '"') {
			in_string = !in_string;
		} else if (!in_string && tmp[i] == '/' && i + 1 < strlen(tmp) && tmp[i + 1] == '/') {
			while (i < strlen(tmp) && tmp[i] != '\n') {
				i++;
			}
			i++;
		} else if (!in_string && tmp[i] == '/' && i + 1 < strlen(tmp) && tmp[i + 1] == '*') {
			i += 2;
			while (!in_string && tmp[i] != '*' && i + 1 < strlen(tmp) && tmp[i + 1] != '/') {
				i++;
			}
			i += 2;
			if (i + 1 < strlen(tmp)) {
				i++;
			}
		}
		ret[j] = tmp[i];
		ret[j + 1] = '\0';
		j++;
	}
	free(tmp);
	return ret;
}
static char *next_key(const char *buf)
{
	/*
	 * Need not to free() after use.
	 */
	if (buf == NULL) {
		return NULL;
	}
	if (strlen(buf) == 0) {
		return NULL;
	}
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
static char *next_layer(const char *buf)
{
	/*
	 * Need not to free() after use.
	 */
	const char *p = buf;
	// Reach the first layer.
	for (size_t i = 0; i < strlen(p); i++) {
		if (p[i] == '\\') {
			i++;
			continue;
		} else if (p[i] == '{') {
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
static char *parse_value(const char *buf)
{
	/*
	 * Warning: free() after use.
	 */
	if (buf == NULL) {
		return NULL;
	}
	char *tmp = strdup(buf);
	if (tmp == NULL) {
		return NULL;
	}
	if (strcmp(tmp, "\"\"") == 0) {
		free(tmp);
		return NULL;
	}
	if (strcmp(tmp, "null") == 0) {
		free(tmp);
		return NULL;
	}
	if (strcmp(tmp, "[]") == 0) {
		free(tmp);
		return NULL;
	}
	if (strcmp(tmp, "{}") == 0) {
		free(tmp);
		return NULL;
	}
	char *ret = NULL;
	// Skip space.
	for (size_t i = 0; i < strlen(tmp); i++) {
		if (tmp[i] == '\\') {
			i++;
			continue;
		} else if (tmp[i] == ' ') {
			continue;
		} else if (tmp[i] == '[' || tmp[i] == '{') {
			free(tmp);
			return strdup(buf);
		} else if (tmp[i] == '"' && i < strlen(tmp)) {
			ret = &tmp[i + 1];
			break;
		} else {
			ret = strdup(&tmp[i]);
			free(tmp);
			return ret;
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
		} else if (ret[i] == '"') {
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
	bool in_string = false;
	for (size_t i = 0; i < strlen(buf); i++) {
		if (buf[i] == '\\') {
			i++;
			continue;
		} else if (buf[i] == '"') {
			in_string = !in_string;
		} else if (buf[i] == ':' && !in_string) {
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
	ret = parse_value(ret);
	free(tmp);
	return ret;
}
static char *json_get_key_one_level(const char *buf, const char *key)
{
	/*
	 * Warning: free() after use.
	 */
	const char *p = buf;
	const char *q = p;
	char *current = NULL;
	while (p != NULL) {
		current = current_key(p);
		if (current == NULL) {
			break;
		}
		if (strcmp(current, key) == 0) {
			char *ret = current_value(p);
			free(current);
			return ret;
		}
		free(current);
		q = p;
		p = next_key(p);
		if (p == NULL) {
			p = next_layer(q);
		}
	}
	return NULL;
}
char *json_get_key(const char *buf, const char *key)
{
	/*
	 * Example json:
	 * {"foo":
	 *   {"bar":
	 *     {"buz":"xxxx"
	 *     }
	 *   }
	 * }
	 * We use key [foo][bar][buz] to get the value of buz.
	 *
	 * Warning: free() after use.
	 */
	if (buf == NULL || key == NULL) {
		return NULL;
	}
	char *keybuf = malloc(strlen(key));
	char *tmp = format_json(buf);
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
size_t json_anon_layer_get_key_array(const char *buf, const char *key, char ***array)
{
	/*
	 * Warning: free() after use.
	 * Warning: **array should be NULL.
	 * Warning: There might be NULL in the array.
	 * From json anonymous layers, get all values of key.
	 * Return: The lenth we get.
	 */
	if (buf == NULL || key == NULL) {
		return 0;
	}
	char *tmp = format_json(buf);
	(*array) = malloc(sizeof(char *));
	(*array)[0] = NULL;
	size_t ret = 0;
	const char *p = tmp;
	while (p != NULL) {
		(*array)[ret] = json_get_key_one_level(p, key);
		ret++;
		(*array) = realloc((*array), sizeof(char *) * (ret + 1));
		(*array)[ret] = NULL;
		p = next_layer(p);
	}
	free(tmp);
	return ret;
}
char *json_anon_layer_get_key(const char *buf, const char *key, const char *value, const char *key_to_get)
{
	/*
	 * Warning: free() after use.
	 * From json anonymous layers, get key_to_get in the layer that key==value.
	 * Return: The value we get.
	 */
	const char *p = buf;
	char *valtmp = NULL;
	while (p != NULL) {
		valtmp = json_get_key_one_level(p, key);
		if (valtmp == NULL) {
			p = next_layer(p);
			continue;
		}
		if (strcmp(valtmp, value) == 0) {
			char *ret = json_get_key_one_level(p, key_to_get);
			free(valtmp);
			return ret;
		}
		free(valtmp);
		p = next_layer(p);
	}
	return NULL;
}
char *json_open_file(const char *path)
{
	/*
	 * Warning: free() after use.
	 */
	struct stat st;
	if (stat(path, &st) == -1) {
		return 0;
	}
	char *ret = malloc((size_t)st.st_size + 3);
	int fd = open(path, O_RDONLY | O_CLOEXEC);
	read(fd, ret, st.st_size);
	ret[st.st_size] = '\0';
	return ret;
}