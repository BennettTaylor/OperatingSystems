#include "myshell_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(void)
{
    struct pipeline *my_pipeline;
    char *input_in[] = {
        "cat < a_file\n",
        " cat < a_file \n",
        "cat<a_file\n",
        "   cat   <   a_file   \n",
    };
    int n = sizeof(input_in) / sizeof(input_in[0]);
    printf("Redirect in\n");
    for (int i = 0; i < n; i++)
    {
        printf("Case %d\n", i);
        my_pipeline = pipeline_build(input_in[i]);

        // Test that a pipeline was returned
        assert(my_pipeline != NULL);
        assert(!my_pipeline->is_background);
        assert(my_pipeline->commands != NULL);

        // Test the parsed args
        assert(strcmp("cat", my_pipeline->commands->command_args[0]) == 0);
        assert(my_pipeline->commands->command_args[1] == NULL);

        // Test the redirect state
        assert(my_pipeline->commands->redirect_in_path != NULL);
        assert(strcmp("a_file", my_pipeline->commands->redirect_in_path) == 0);
        assert(my_pipeline->commands->redirect_out_path == NULL);

        // Test that there is only one parsed command in the pipeline
        assert(my_pipeline->commands->next == NULL);

        pipeline_free(my_pipeline);
    }

    char *input_out[] = {
        "ls > txt\n",
        " ls > txt \n",
        "ls>txt \n",
        "   ls   >   txt   \n",
    };
    n = sizeof(input_out) / sizeof(input_out[0]);
    printf("Redirect out\n");
    for (int i = 0; i < n; i++)
    {
        printf("Case %d\n", i);
        my_pipeline = pipeline_build(input_out[i]);

        // Test that a pipeline was returned
        assert(my_pipeline != NULL);
        assert(!my_pipeline->is_background);
        assert(my_pipeline->commands != NULL);

        // Test the parsed args
        assert(strcmp("ls", my_pipeline->commands->command_args[0]) == 0);
        assert(my_pipeline->commands->command_args[1] == NULL);

        // Test the redirect state
        assert(my_pipeline->commands->redirect_in_path == NULL);
        assert(my_pipeline->commands->redirect_out_path != NULL);
        assert(strcmp("txt", my_pipeline->commands->redirect_out_path) == 0);

        // Test that there is only one parsed command in the pipeline
        assert(my_pipeline->commands->next == NULL);

        pipeline_free(my_pipeline);
    }
}