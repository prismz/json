#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

HMItem *new_item(char *key, void *val,
                void (*k_free_func)(void *), void (*v_free_func)(void *))
{
	HMItem *item = calloc(1, sizeof(*item));
        if (item == NULL)
                return NULL;

	item->key = key;
	item->val = val;
	item->k_free_func = k_free_func;
	item->v_free_func = v_free_func;

	return item;
}

void free_item(HMItem *item)
{
	if (item == NULL)
		return;

	if (item->key != NULL && item->k_free_func != NULL)
		item->k_free_func(item->key);
	if (item->val != NULL && item->v_free_func != NULL)
		item->v_free_func(item->val);

	free(item);
}

HashMap *new_hashmap(size_t capacity)
{
	HashMap *map = json_calloc(1, sizeof(*map));
        if (map == NULL)
                return NULL;

	map->can_store = capacity;
	map->stored = 0;
	map->items = json_calloc(map->can_store, sizeof(HMItem *));
        if (map->items != NULL) {
                free(map);
                return NULL;
        }

	return map;
}

void free_hashmap(HashMap *map)
{
	if (map == NULL)
		return;

	for (size_t i = 0; i < map->can_store; i++)
		free_item(map->items[i]);

	free(map->items);
	free(map);
}

int check_hashmap_capacity(HashMap *map, size_t n)
{
	if (!((map->stored + n + 1) > map->can_store))
		return 0;

        int ns_a = ((n + 8) > 32) ? (n + 8) : 32;
	size_t new_size = map->can_store + ns_a;
	HMItem **new = calloc(new_size, sizeof(HMItem *));
        if (new == NULL)
                return 1;

	for (size_t i = 0; i < map->can_store; i++) {
		HMItem *item = map->items[i];
		if (item != NULL)
			new[i] = item;
	}

	free(map->items);
	map->can_store = new_size;
	map->items = new;

        return 0;
}

void hashmap_set(HashMap *map, HMItem *item)
{
	check_hashmap_capacity(map, 1);
	uint64_t hash = hashmap_hash_func(item->key);
	size_t index = (size_t)(hash & (uint64_t)(map->can_store - 1));

	/* find an empty entry */
	while (map->items[index] != NULL) {
		if (strcmp(item->key, map->items[index]->key) == 0) {
			free_item(map->items[index]);
			map->items[index] = item;
			return;
		}

		index++;
		if (index >= map->can_store)
			index = 0;
	}

	map->items[index] = item;
	map->stored++;
}

/* TODO: https://github.com/benhoyt/ht/blob/master/ht.c */
uint64_t hashmap_hash_func(char *key)
{
	uint64_t hash = FNV_OFFSET;
	for (const char* p = key; *p; p++) {
		hash ^= (uint64_t)(unsigned char)(*p);
		hash *= FNV_PRIME;
	}
	return hash;
}

void *hashmap_index(HashMap *map, char *key)
{
	uint64_t hash = hashmap_hash_func(key);
	size_t index = (size_t)(hash & (uint64_t)(map->can_store - 1));

	while (map->items[index] != NULL) {
		if (strcmp(map->items[index]->key, key) == 0)
			return map->items[index]->val;

		/* collision */
		index++;
		if (index >= map->can_store)
			index = 0;
	}

	return NULL;
}

void hashmap_remove(HashMap *map, char *key)
{
	uint64_t hash = hashmap_hash_func(key);
	size_t index = (size_t)(hash & (uint64_t)(map->can_store - 1));

	while (map->items[index] != NULL) {
		if (strcmp(map->items[index]->key, key) == 0) {
			free_item(map->items[index]);
			map->items[index] = NULL;
			map->stored--;
		}

		/* collision */
		index++;
		if (index >= map->can_store)
			index = 0;
	}
}

/* Begin JSON implementation */

#ifndef json_error

void json_error(char *fmt, ...)
{
        va_list ap;
        va_start(ap, fmt);

        fprintf(stderr, "error: ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");

        va_end(ap);
        exit(1);
}

#endif  /* json_error */

struct json *new_json(void)
{
        struct json *j = calloc(1, sizeof(struct json));
        if (j == NULL)
                return NULL;

        j->type = json_null;

        return j;
}

void free_json_item(struct json *j)
{
        if (j == NULL)
                return;

        switch (j->type) {
        case json_string:
                free(j->data.string);
                break;
        case json_array:
                for (int i = 0; i < j->n_data_items; i++)
                        free_json_item(j->data.json_data_array[i]);

                free(j->data.json_data_array);
                break;
        case json_dict:
                free_hashmap(j->data.json_data_dict);
                break;
        default:
                break;
        }

        free(j);
}

void print_json(struct json *j)
{
        HashMap *dict;
        switch (j->type) {
        case json_bool:
                if (j->data.boolean)
                        printf("true\n");
                else
                        printf("false\n");

                break;
        case json_string:
                printf("%s\n", j->data.string);
                break;
        case json_array:
                for (int i = 0; i < j->n_data_items; i++)
                        print_json(j->data.json_data_array[i]);
                break;
        case json_dict:
                dict = j->data.json_data_dict;
                for (int i = 0; i < (int)dict->can_store; i++) {
                        HMItem *item = dict->items[i];

                        if (item == NULL)
                                continue;

                        printf("\"%s\": ", dict->items[i]->key);
                        print_json(dict->items[i]->val);
                }
                break;
        case json_number:
                printf("%f\n", j->data.number);
                break;
        case json_null:
                printf("null\n");
                break;
        }
}

HMItem *json_parse_dict_tuple(char *str, int *idx)
{
        size_t len = strlen(str);
        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }

        int key_end_idx;
        struct json *json_key = json_parse_string(str + start, &key_end_idx);
        if (json_key == NULL)
                return NULL;

        key_end_idx += start;

        size_t val_start;
        for (val_start = key_end_idx; val_start < len; val_start++) {
                if (!isspace(str[val_start]) && str[val_start] != ':')
                        break;
        }

        int val_end_idx;
        struct json *val = json_parse_item(str + val_start, &val_end_idx);
        if (val == NULL) {
                json_free_item(json_key);
                return NULL;
        }

        val_end_idx += val_start;

        char *key = json_key->data.string;
        free(json_key);

        if (idx != NULL)
                *idx = val_end_idx;

        struct HMItem *tuple = new_item(key, val, free,
                        hashmap_item_free_func(free_json_item));

        return tuple;
}

struct json *json_parse_dict(char *str, int *idx)
{
        struct json *j = new_json();
        j->type = json_dict;

        size_t len = strlen(str);
        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }

        HashMap *dict = new_hashmap(16);
        j->data.json_data_dict = dict;

        bool done = false;
        size_t current_idx = start + 1;
        for (size_t i = start + 1; i < len; i++) {
                char c = str[i];
                if (c == '}')
                        break;

                int end_idx;
                HMItem *current_tuple = json_parse_dict_tuple(
                                str + current_idx, &end_idx);

                current_idx += end_idx;

                for (; current_idx < len; current_idx++) {
                        char cc = str[current_idx];
                        if (cc == '}') {
                                current_idx++;
                                done = true;
                                goto append;
                        }

                        if (!isspace(cc) && cc != ',')
                                break;
                }

append:
                i = current_idx;

                hashmap_set(dict, current_tuple);

                if (done)
                        break;
        }

        j->n_data_items = j->data.json_data_dict->stored;
        j->data_list_capacity = j->data.json_data_dict->can_store;

        if (idx != NULL)
                *idx = current_idx;

        return j;
}

struct json *json_parse_string(char *str, int *idx)
{
        struct json *j = new_json();
        j->type = json_string;

        size_t len = strlen(str);

        if (strncmp(str, "\"\"", strlen("\"\"")) == 0) {
                j->data.string = strdup("");
                if (idx != NULL)
                        *idx = 2;

                return j;
        }

        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]) && str[start] != '"')
                        break;
        }

        size_t size = 1024;
        size_t buffer_i = 0;
        char *buffer = json_malloc(sizeof(char) * size);

        bool escaped = false;
        size_t i;
        for (i = start; i < len; i++) {
                char c = str[i];
                if (c == '\\') {
                        escaped = true;
                        continue;
                }

                if (!escaped && c == '"')
                        break;

                if (buffer_i + 2 > size) {
                        buffer = json_realloc(buffer, sizeof(char) *
                                        (size += 512));
                }

                buffer[buffer_i++] = c;
                buffer[buffer_i] = 0;

                if (escaped)
                        escaped = false;
        }

        if (i != start)
                i++;

        if (idx != NULL)
                *idx = i;

        j->data.string = buffer;

        return j;
}

struct json *json_parse_array(char *str, int *idx)
{
        struct json *j = new_json();
        j->type = json_array;

        size_t len = strlen(str);

        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }
        start++;  /* skip first bracket */

        for (; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }


        size_t capacity = 4;
        size_t array_i = 0;
        struct json **array = json_calloc(capacity,
                        sizeof(struct json *));

        size_t current_point = start;
        size_t i;
        for (i = start; i < len; i++) {
                int current_element_end;
                struct json *elem = json_parse_item(str + current_point,
                                &current_element_end);

                current_point += current_element_end;
                for (; current_point < len; current_point++) {
                        char cc = str[current_point];
                        if (!isspace(cc) && cc != ',')
                                break;
                }

                if (array_i + 1 > capacity) {
                        array = json_realloc(array,
                                    sizeof(struct json *) * (capacity += 4));
                }

                array[array_i++] = elem;

                if (str[current_point] == ']') {
                        i = current_point + 1;
                        break;
                }

                i = current_point;
        }

        j->data.json_data_array = array;
        j->n_data_items = array_i;
        j->data_list_capacity = capacity;

        if (idx != NULL)
                *idx = i;

        return j;
}

struct json *json_parse_number(char *str, int *idx)
{
        struct json *j = new_json();
        j->type = json_number;

        size_t len = strlen(str);
        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }

        size_t i;

        size_t capacity = 16;
        size_t data_i = 0;
        char *data = json_malloc(sizeof(char) * capacity);

        for (i = start; i < len; i++) {
                char c = str[i];
                if (!isdigit(c) && c != '.' && c != 'e')
                        break;

                if (data_i + 2 > capacity) {
                        data = json_realloc(data,
                                        sizeof(char) * (capacity += 4));
                }

                data[data_i++] = c;
                data[data_i] = 0;
        }

        sscanf(data, "%lf", &j->data.number);
        free(data);

        if (idx != NULL)
                *idx = i;

        return j;
}

struct json *json_parse_bool(char *str, int *idx)
{
        struct json *j = new_json();
        j->type = json_bool;

        size_t len = strlen(str);
        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }

        int stop_idx = start;
        size_t tl = strlen("true");
        size_t fl = strlen("false");

        if (strncmp(str + start, "true", tl) == 0) {
                j->data.boolean = true;
                stop_idx += tl;
        } else {
                j->data.boolean = false;
                stop_idx += fl;
        }

        if (idx != NULL)
                *idx = stop_idx;

        return j;
}

struct json *json_parse_null(char *str, int *idx)
{
        struct json *j = new_json();
        j->type = json_null;

        size_t len = strlen(str);
        size_t start;
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }

        int end_idx = start + strlen("null");
        if (idx != NULL)
                *idx = end_idx;

        return j;
}

struct json *json_parse_item(char *str, int *idx)
{
        size_t len = strlen(str);
        size_t start;

        /* skip leading whitespace if there is any */
        for (start = 0; start < len; start++) {
                if (!isspace(str[start]))
                        break;
        }

        int end_idx = 0;
        struct json *j;

        char c = str[start];
        switch (c) {
        case '{':
                j = json_parse_dict(str + start, &end_idx);
                break;
        case '[':
                j = json_parse_array(str + start, &end_idx);
                break;
        case '"':
                j = json_parse_string(str + start, &end_idx);
                break;
        default:
                if (c == 'n')  /* null */
                        j = json_parse_null(str + start, &end_idx);
                else if (isdigit(c))  /* number */
                        j = json_parse_number(str + start, &end_idx);
                else if (c == 't' || c == 'f')  /* true / false */
                        j = json_parse_bool(str + start, &end_idx);

                break;
        }

        if (idx != NULL)
                *idx = end_idx;

        return j;
}

struct json *json_get_array_item(struct json *arr, int idx)
{
        if (idx >= arr->n_data_items)
                return NULL;

        return arr->data.json_data_array[idx];
}

struct json *json_get_dict_item(struct json *dict, char *key)
{
        HashMap *map = dict->data.json_data_dict;
        return (struct json *)hashmap_index(map, key);
}

int json_get_size(struct json *arr)
{
        return arr->n_data_items;
}

int json_get_capacity(struct json *arr)
{
        return arr->data_list_capacity;
}

char *json_read_file(char *path)
{
        FILE *fp = fopen(path, "rb");
        if (!fp)
                return NULL;

        size_t capacity = 1024;
        size_t buffer_i = 0;
        char *buffer = json_calloc(1, sizeof(char) * capacity);

        char c;
        while ((c = fgetc(fp)) != EOF) {
                if (buffer_i + 2 > capacity) {
                        buffer = json_realloc(buffer,
                                        sizeof(char) * (capacity += 512));
                }

                buffer[buffer_i++] = c;
                buffer[buffer_i] = 0;
        }

        fclose(fp);

        return buffer;
}

int main(void)
{
        char *str = json_read_file("example2");
        struct json *j = json_parse(str);

        print_json(j);

        free_json_item(j);
        free(str);
}
