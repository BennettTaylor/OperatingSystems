#include "myshell_parser.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main()
{
    struct pipeline *my_pipeline = pipeline_build("cat < x > y\n");

    // Test that a pipeline was returned
    assert(my_pipeline != NULL);
    assert(!my_pipeline->is_background);
    assert(my_pipeline->commands != NULL);

    // Test the parsed args
    assert(strcmp("cat", my_pipeline->commands->command_args[0]) == 0);
    assert(my_pipeline->commands->command_args[1] == NULL);

    // Test the redirect state
    assert(strcmp("x", my_pipeline->commands->redirect_in_path) == 0);
    assert(strcmp("y", my_pipeline->commands->redirect_out_path) == 0);

    // Test that there is only one parsed command in the pipeline
    assert(my_pipeline->commands->next == NULL);

    pipeline_free(my_pipeline);
}