#ifndef JSON_H
#define JSON_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef json_error
void json_error(char *fmt, ...);
#endif  /* json_error */

/* hashmap */
#define JSONLIB_MAX(a, b) ((a) > (b)) ? (a) : (b)

typedef struct HMItem HMItem;

struct HMItem {
	char *key;
	void *val;

	void (*k_free_func)(void *);
	void (*v_free_func)(void *);
};

#define hashmap_item_free_func(a) (void (*)(void *))a

typedef struct {
	HMItem **items;  /* buckets */

	size_t can_store;
	size_t stored;
} HashMap;

HMItem *new_item(char *key, void *val,
                void (*k_free_func)(void *), void (*v_free_func)(void *));
void free_item(HMItem *item);
HashMap *new_hashmap(size_t capacity);
void free_hashmap(HashMap *map);
int check_hashmap_capacity(HashMap *map, size_t n);
int hashmap_set(HashMap *map, HMItem *item);

/*
 * 64-bit FNV-1a hash:
 * https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
 * https://github.com/benhoyt/ht/blob/master/ht.c
 */
#define FNV_OFFSET 14695981039346656037UL
#define FNV_PRIME 1099511628211UL
uint64_t hashmap_hash_func(char *key);

void *hashmap_index(HashMap *map, char *key);
void hashmap_remove(HashMap *map, char *key);

/* Begin JSON implementation */

typedef enum json_t {
	json_bool,
	json_string,
	json_array,
	json_dict,
	json_number,
	json_null
} json_t;

typedef union {
	double number;
	char *string;
	bool boolean;
	struct json *json_data;
        struct json **json_data_array;
        HashMap *json_data_dict;  
} json_data;

struct json {
	json_t type;
	char *key;
        int n_data_items;  /* length of array or dict (if present) */
        size_t data_list_capacity;  /* capacity of array/dict */
	json_data data;
};

struct json *new_json(void);
void free_json_item(struct json *j);
void print_json(struct json *j);
HMItem *json_parse_dict_tuple(char *str, int *idx);
struct json *json_parse_dict(char *str, int *idx);
struct json *json_parse_string(char *str, int *idx);
struct json *json_parse_array(char *str, int *idx);
struct json *json_parse_number(char *str, int *idx);
struct json *json_parse_bool(char *str, int *idx);
struct json *json_parse_null(char *str, int *idx);
struct json *json_parse_item(char *str, int *idx);

#define json_parse(str) json_parse_item(str, NULL)

struct json *json_get_array_item(struct json *arr, int idx);
struct json *json_get_dict_item(struct json *dict, char *key);
int json_get_size(struct json *arr);
int json_get_capacity(struct json *arr);
char *json_read_file(char *path);

#define json_get_string(j) (j->data.string)
#define json_get_bool(j) (j->data.boolean)
#define json_get_number(j) (j->data.number)
#define json_get_data(j) (j->data.json_data)
#define json_get_array(j) (j->data.json_data_array)
#define json_get_dict(j) (j->data.json_data_dict)

#endif  /* JSON_H */
