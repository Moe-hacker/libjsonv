# About:
This lib is part of rurima.      
I use it to parse json from docerhub.       
In fact I really recommend you to use json-c.       
# Function list:
```C
char *json_get_key(const char *buf, const char *key);
size_t json_anon_layer_get_key_array(const char *buf, const char *key, char ***array);
char *json_anon_layer_get_key(const char *buf, const char *key, const char *value, const char *key_to_get);
char *json_open_file(const char *path);
```
