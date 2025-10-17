#include "myshell_parser.h"
#include "stddef.h"
#include <string.h>
#include <stdlib.h>

struct token {
    char *token;
    struct token *next;
};

struct token *lex_pipeline(const char *command_line) {
    struct token *head = (struct token*)malloc(sizeof(struct token));
    struct token *temp = head;
    int word_start = 0;
    int word_end = 0;
    int state_prev = 0;
    int i = 0;
    for (char character = *command_line; character != '\0'; character = *++command_line) {
        if ((character == ' ') || (character == '\t') || (character == '\n')) {
            if (state_prev == 2) {
                temp->next = (struct token*)malloc(sizeof(struct token));
                temp = temp->next;
                char* token = (char *)malloc((word_end - word_start) * sizeof(char));
                strncpy(token, command_line - word_end + word_start, word_end - word_start);
                temp->token = token;
            }
            word_start = i + 1;
            state_prev = 0;
        }
        else if ((character == '|') || (character == '>') || (character == '<') || (character == '&')) {
            if (state_prev == 2) {
                temp->next = (struct token*)malloc(sizeof(struct token));
                temp = temp->next;
                char* token = (char *)malloc((word_end - word_start) * sizeof(char));
                strncpy(token, command_line - word_end + word_start, word_end - word_start);
                temp->token = token;
            }
            temp->next = (struct token*)malloc(sizeof(struct token));
            temp = temp->next;
            char *token = (char *)malloc(2 * sizeof(char));
            token[0] = character;
            token[1] = '\0';
            temp->token = token;
            word_start = i + 1;
            state_prev = 1;
        }
        else {
            word_end = i + 1;
            state_prev = 2;
        }
        i++;
    }
    if (state_prev == 2) {
        temp->next = (struct token*)malloc(sizeof(struct token));
        temp = temp->next;
        char* token = (char *)malloc((word_end - word_start) * sizeof(char));
        strncpy(token, command_line - word_end + word_start, word_end - word_start);
        temp->token = token;
    }
    temp = head->next;
    free(head);
    head = temp;
    return head;
}

struct pipeline *pipeline_build(const char *command_line)
{
    struct token *head = lex_pipeline(command_line);
    struct token *temp_token = head;
    if ((temp_token->token[0] == '|') || (temp_token->token[0] == '&') || (temp_token->token[0] == '>') || (temp_token->token[0] == '<')) {
        return NULL;
    }
    struct pipeline *pipeline = (struct pipeline*)malloc(sizeof(struct pipeline));
    pipeline->commands = (struct pipeline_command*)malloc(sizeof(struct pipeline_command));
    struct pipeline_command *temp_command = pipeline->commands;
    int state = 0; // looking for command or args
    int arg_num = 0;
    while (temp_token != NULL) {
        if (temp_token->token[0] == '|') {
            if (state != 0) {
                return NULL;
            }
            temp_command->command_args[arg_num+1] = NULL;
            temp_command->next = (struct pipeline_command*)malloc(sizeof(struct pipeline_command));
            temp_command = temp_command->next;
            state = 3;
            arg_num = 0;
        }
        else if (temp_token->token[0] == '>') {
            if (state != 0) {
                return NULL;
            }
            state = 1;
        }
        else if (temp_token->token[0] == '<') {
            if (state != 0) {
                return NULL;
            }
            state = 2;
        }
        else if (temp_token->token[0] == '&') {
            if (!pipeline->is_background) {
                pipeline->is_background = true;
                state = 0;
            }
            else {
                return NULL;
            }
        }
        else {
            if ((state == 0) || (state == 3)) {
                char* token = (char *)malloc(sizeof(temp_token->token));
                strcpy(token, temp_token->token);
                temp_command->command_args[arg_num] = token;
                arg_num++;
                state = 0;
            }
            else if (state == 1) {
                if (temp_command->redirect_out_path == NULL) {
                    char* token = (char *)malloc(sizeof(temp_token->token));
                    strcpy(token, temp_token->token);
                    temp_command->redirect_out_path = token;
                    state = 0;
                }
                else {
                    return NULL;
                }
            }
            else {
                if (temp_command->redirect_in_path == NULL) {
                    char* token = (char *)malloc(sizeof(temp_token->token));
                    strcpy(token, temp_token->token);
                    temp_command->redirect_in_path = token;
                    state = 0;
                }
                else {
                    return NULL;
                }
            }
        }
        temp_token = temp_token->next;
    }
    temp_command->command_args[arg_num+1] = NULL;
    temp_token = head;
    struct token *deleted;
    while (temp_token != NULL) {
        free(temp_token->token);
        deleted = temp_token;
        temp_token = temp_token->next;
        free(deleted);
    }
    if (pipeline->is_background != true) {
        pipeline->is_background = false;
    }
    return pipeline;
}

void pipeline_free(struct pipeline *pipeline)
{
    struct pipeline_command *temp = pipeline->commands;
    struct pipeline_command *deleted;
    while (temp != NULL) {
        int j = 0;
        char* arg = temp->command_args[j];
        while (arg != NULL) {
            free(arg);
            j++;
            arg = temp->command_args[j];
        }
        free(temp->redirect_in_path);
        free(temp->redirect_out_path);
        deleted = temp;
        temp = temp->next;
        free(deleted);
    }
    free(pipeline);
}