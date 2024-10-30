# About:
This lib is part of rurima.      
I use it to parse json from docerhub.       
In fact I really recommend you to use json-c.       
# Example program:
[rurima/src/dockerhub.c](https://github.com/Moe-hacker/rurima/blob/main/src/dockerhub.c)
# Function list:
```C
char *json_get_key(const char *_Nonnull buf, const char *_Nonnull key);
size_t json_anon_layer_get_key_array(const char *_Nonnull buf, const char *_Nonnull key, char ***_Nullable array);
char *json_anon_layer_get_key(const char *_Nonnull buf, const char *_Nonnull key, const char *_Nonnull value, const char *_Nonnull key_to_get);
char *json_open_file(const char *_Nonnull path);
```
