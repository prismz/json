#include "../json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test1(void)
{
        char *str = "{} extra garbage";
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
        char *str = "{}";
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

int main(void)
{
        test1();
        test2();
}
