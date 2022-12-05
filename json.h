#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include <stdbool.h>

#ifndef json_custom_alloc

#define json_malloc malloc
#define json_calloc calloc
#define json_realloc realloc
#define json_free free

#endif  /* json_custom_alloc */

#ifndef json_error
void json_error(char *fmt, ...);
#endif  /* json_error */

typedef enum json_t {
	json_bool,
	json_string,
	json_array,
	json_dict,
        json_dict_item,
	json_number,
	json_null
} json_t;

typedef union {
	double number;
	char *string;
	bool boolean;
	struct json *json_data;
        struct json **json_data_array;
        struct json **json_data_dict;  /* TODO: hashmap */
} json_data;

struct json {
	json_t type;
	char *key;
        int n_data_items;  /* length of array or dict (if present) */
        size_t data_list_capacity;  /* capacity of array/dict */
	json_data data;
};

struct hashmap_item {
        char *key;
        void *val; 
        bool used;
};

struct hashmap {
        struct hashmap_item **items;
        size_t n;
        size_t capacity;
};

struct json *new_json(void);
void print_json(struct json *j);
struct json *json_parse_dict_tuple(char *str, int *idx);
struct json *json_parse_dict(char *str, int *idx);
struct json *json_parse_string(char *str, int *idx);
struct json *json_parse_array(char *str, int *idx);
struct json *json_parse_number(char *str, int *idx);
struct json *json_parse_bool(char *str, int *idx);
struct json *json_parse_null(char *str, int *idx);
struct json *json_parse_item(char *str, int *idx);

struct json *json_get_array_item(struct json *arr, int idx);
struct json *json_get_dict_item(struct json *dict, char *key);
int json_get_size(struct json *dict_or_list);
int json_get_capacity(struct json *dict_or_list);

char *json_read_file(char *path);

#endif  /* JSON_H */
