#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>

// ! Constant
#define MAX_COMMAND_LENGTH 80
#define PROMPT "tsh > "
#define EXIT_COMMAND "exit"
#define HISTORY_COMMAND "!!"
#define NO_COMMAND_NOTIFICATION "No commands in history !"
#define DELIM " \n\t\v\a\r\f"

// ! Command history
char *historyCmd = NULL;

// fn: Checks if a character is a white space ?
bool isSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == '\v';
}

// fn: Remove white space in command */
void trim(char *str)
{
  //null string
  if (!str)
    return;
  //empty string
  if (!(*str))
    return;

  //left trim
  while (isSpace(*str))
    str++;

  //right trim
  char *ptr;
  for (ptr = str + strlen(str) - 1; (ptr >= str) && isSpace(*ptr); --ptr)
    ;

  ptr[1] = '\0';
}

// fn: Checks if there exists & at the end of the statement
bool isIncludeAmpersand(char *command)
{
  if (command)
  {
    for (int i = strlen(command) - 1; i >= 0; i--)
    {
      char c = command[i];
      if (!isSpace(c) && c != '\0')
      {
        if (c == '&')
          return true;
        return false;
      }
    }
  }
}

// fn: Remove ampersand in user command (if exist) */
void removeAmpersand(char *command)
{
  if (command)
  {
    char *ptr;
    for (ptr = command + strlen(command) - 1; (ptr >= command) && (isSpace(*ptr) || *ptr == '&'); --ptr)
      ;
    ptr[1] = '\0';
  }
}

// fn: Enter & get a user's command
void getCommand(char *command)
{
  fgets(command, MAX_COMMAND_LENGTH, stdin);
  command[strlen(command) - 1] = '\0';

  //remove excess white space
  trim(command);

  //if the command # "!!" then save history command if command # NULL
  if (strcmp(command, HISTORY_COMMAND) != 0)
    if (command[0] != '\0')
    {
      if (historyCmd)
      {
        free(historyCmd);
        historyCmd = NULL;
      }
      historyCmd = (char *)malloc(strlen(command) + 1);
      strcpy(historyCmd, command);
    }
}

// fn: Token user command (ex: "ls -l" => ["ls", "-l"])
char **tokenUserCommand(char *command)
{
  if (command)
  {
    //count space to know element number of result
    int countWord = 0;
    for (int i = 0; i < strlen(command) - 1; ++i)
    {
      if (isSpace(command[i]) && !isSpace(command[i + 1]))
        ++countWord;
    }
    char **result = (char **)malloc((countWord + 2) * sizeof(char *));

    char *token = strtok(command, DELIM);
    int n = 0;
    // walk through other tokens
    while (token != NULL)
    {
      result[n] = (char *)malloc(strlen(token) * sizeof(char));
      strcpy(result[n++], token);
      token = strtok(NULL, DELIM);
    }
    result[n] = NULL;

    return result;
  }
  return NULL;
}

// fn: handle error
void handleError(const char *errorMessage)
{
  perror(errorMessage);
  exit(0);
}

// fn: Simple commands with child processes - Simple command with &
void execvpCommand(char *command, bool isAmpersand)
{
  pid_t pid = fork();
  char **params = tokenUserCommand(command);

  // case: child process then exec
  if (pid == 0)
  {
    execvp(params[0], params);
    handleError("Error");
  }
  // case: the fork failed
  else if (pid < 0)
    handleError("Error");
  // case: parent process
  else
  {
    // case: if included & then the parent and child processes will run concurrently
    //else the parent must wait for the child
    if (!isAmpersand)
    {
      waitpid(pid, NULL, 0);
    }
  }
}

// fn: Main shell loop
void mainShellLoop()
{
  while (1)
  {
    //print prompt command
    printf(PROMPT);
    fflush(stdout);

    //get the user's command has been trimmed
    char *command = (char *)malloc(MAX_COMMAND_LENGTH);
    getCommand(command);

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

    //exec command
    bool isAmpersand = isIncludeAmpersand(command);
    removeAmpersand(command);
    execvpCommand(command, isAmpersand);
  }
}

/* Main function */
int main()
{
  mainShellLoop();
  return 0;
}
