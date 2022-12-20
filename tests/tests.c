#include "../json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test1(void)
{
        char *str = "{   } extra garbage";
        int end;
        struct json *j = json_parse_dict(str, &end);

        if (strcmp(str + end, " extra garbage") != 0) {
                printf("%s\n", str + end);
                goto fail;
        }


        printf("Test #01: Pass\n");
        free_json_item(j);

        return;

fail:
        printf("Test #01: FAIL\n");
        free_json_item(j);
}

void test2(void)
{
        char *str = "{   }";
        int end;
        struct json *j = json_parse_dict(str, &end);

        if (json_get_size(j) != 0)
                goto fail;

        printf("Test #02: Pass\n");
        free_json_item(j);

        return;

fail:
        printf("Test #02: FAIL\n");
        free_json_item(j);
}

void test3(void)
{
        char *str = "[   ] extra";
        char *data;
        int end;
        struct json *j = json_parse_array(str, &end);

        if (j == NULL) {
                data = "Value is NULL";
                goto fail;
        } if (json_get_size(j) != 0) {
                data = "Zero array does not have size zero"; 
                printf("%d\n", json_get_size(j));
                goto fail;
        } if (strcmp(str + end, " extra") != 0) {
                data = "Ending failed";
                goto fail;
        }

        printf("Test #03: Pass\n");
        free_json_item(j);

        return;

fail:
        printf("Test #03: FAIL: %s\n", data);
        print_json(j);
}


int main(void)
{
        test1();
        test2();
        test3();

        char *contents = json_read_file("../samples/sample2");
        struct json *j = json_parse(contents);

        print_json(j);

        free_json_item(j);
        free(contents);
}
