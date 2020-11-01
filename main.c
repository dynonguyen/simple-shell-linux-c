#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

//constant
#define MAX_COMMAND_LENGTH 80
#define PROMPT "tsh > "
#define EXIT_COMMAND "exit"
#define HISTORY_COMMAND "!!"
#define NO_COMMAND_NOTIFICATION "No commands in history !"

//command history
char *historyCmd = NULL;

/* checks if a character is a white space ? */
bool isSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == '\v';
}

/* remove white space in command */
char *trim(char *str)
{
  //null string
  if (!str)
    return NULL;
  //empty string
  if (!(*str))
    return str;

  //left trim
  while (isSpace(*str))
    str++;

  //right trim
  char *ptr;
  for (ptr = str + strlen(str) - 1; (ptr >= str) && isSpace(*ptr); --ptr)
    ;

  ptr[1] = '\0';

  return str;
}

/* enter & get a user's command */
char *getCommand()
{
  char command[MAX_COMMAND_LENGTH];
  char *userCmd = fgets(command, sizeof(command), stdin);
  userCmd[strlen(userCmd) - 1] = '\0';

  //remove excess white space
  char *result = trim(userCmd);

  //if the command # "!!" then save history command if command # NULL
  if (strcmp(result, HISTORY_COMMAND) != 0)
    if (result[0] != '\0')
    {
      if (historyCmd)
      {
        free(historyCmd);
        historyCmd = NULL;
      }
      historyCmd = (char *)malloc(strlen(result) + 1);
      strcpy(historyCmd, result);
    }

  return result;
}

/* main shell loop */
void mainShellLoop()
{
  while (1)
  {
    //print
    printf(PROMPT);
    fflush(stdout);

    //get the user's command has been trimmed
    char *command = getCommand();

    /* ----- handle command ----- */
    //Case: empty or null => skip
    if (command[0] == '\0' || command == NULL)
      continue;

    // Case: "exit" => break
    if (strcmp(command, EXIT_COMMAND) == 0)
      break;

    //Case: "!!" => history (if exist) else error notification
    if (strcmp(command, HISTORY_COMMAND) == 0)
    {
      if (historyCmd)
      {
        printf("history command: %s\n", historyCmd);
      }
      else
      {
        puts(NO_COMMAND_NOTIFICATION);
      }
    }
  }
}

/* main function */
int main()
{
  mainShellLoop();
  return 0;
}