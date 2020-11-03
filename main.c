#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

// ! Constant
#define MAX_COMMAND_LENGTH 80
#define PROMPT "tsh > "
#define EXIT_COMMAND "exit"
#define HISTORY_COMMAND "!!"
#define NO_COMMAND_NOTIFICATION "No commands in history."
#define SPACE_DELIM " \n\t\v\a\r\f"
#define REDIRECT_DELIM ">|<"

//Note: Command types
#define NORMAL_COMMAND 0
#define INPUT_COMMAND 1
#define OUTPUT_COMMAND 2
#define PIPE_COMMAND 3

// ! Command history
char *historyCmd = NULL;

// fn: Checks if a character is a white space ?
bool isSpace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '\r' || c == '\v';
}

// fn: Remove white space in command */
char *trim(char *str)
{
  //null string
  if (!str)
    return str;
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
  command = trim(command);

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

// fn: Token the normal user command (ex: "ls -l" => ["ls", "-l"])
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

    char *token = strtok(command, SPACE_DELIM);
    int n = 0;
    // walk through other tokens
    while (token != NULL)
    {
      result[n] = (char *)malloc(strlen(token) * sizeof(char));
      strcpy(result[n++], token);
      token = strtok(NULL, SPACE_DELIM);
    }
    result[n] = NULL;

    return result;
  }
  return NULL;
}

// fn: Token the redirect user command (ex: "ls -l > in.txt" => ["ls -l", "in.txt"])
char **tokenRedirectCommand(char *command)
{
  char **tokens = (char **)malloc(2 * sizeof(char *));
  tokens[0] = trim(strtok(command, REDIRECT_DELIM));
  tokens[1] = trim(strtok(NULL, REDIRECT_DELIM));
  return tokens;
}

// fn: Handle error
void handleError(const char *errorMessage)
{
  perror(errorMessage);
  exit(0);
}

// fn: Check command type
int commandType(char *command)
{
  for (int i = 0; i < strlen(command); i++)
  {
    switch (command[i])
    {
    case '>':
      return OUTPUT_COMMAND;
    case '<':
      return INPUT_COMMAND;
    case '|':
      return PIPE_COMMAND;
    default:
      break;
    }
  }
  return NORMAL_COMMAND;
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

// fn: Input redirection
void execvpInputCommand(char *command, bool isAmpersand)
{
}

// fn: Output redirection
void execvpOutputCommand(char *command, bool isAmpersand)
{
  char **tokens = tokenRedirectCommand(command);

  pid_t pid = fork();

  //user command token
  char **params = tokenUserCommand(tokens[0]);

  //execute
  if (pid == 0)
  {
    //open for writing only | Create file)
    int fd = open(tokens[1], O_CREAT | O_TRUNC | O_WRONLY | O_EXCL);
    if (fd < 0)
    {
      //if the file already existed then remove & try to create
      if (errno == EEXIST)
      {
        remove(tokens[1]);
        fd = open(tokens[1], O_CREAT | O_TRUNC | O_WRONLY | O_EXCL);
        if (fd < 0)
        {
          printf("Error opening the file\n");
          return;
        }
      }
      else
      {
        printf("Error opening the file\n");
        return;
      }
    }

    //dup failed
    if (dup2(fd, STDOUT_FILENO) < 0)
      handleError("Dup2 Error");

    execvp(params[0], params);
    close(fd);
    handleError("Error");
  }
  else if (pid < 0)
    handleError("Error");
  else if (!isAmpersand)
  {
    waitpid(pid, NULL, 0);
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
        strcpy(command, historyCmd);
      }
      else
      {
        puts(NO_COMMAND_NOTIFICATION);
        continue;
      }
    }

    //classify commands
    bool isAmpersand = isIncludeAmpersand(command);
    int cmdType = commandType(command);
    removeAmpersand(command);

    //Execute the user command by type
    switch (cmdType)
    {
    case NORMAL_COMMAND:
      execvpCommand(command, isAmpersand);
      break;
    case INPUT_COMMAND:
      tokenRedirectCommand(command);
      break;
    case OUTPUT_COMMAND:
      execvpOutputCommand(command, isAmpersand);
      break;
    case PIPE_COMMAND:
      tokenRedirectCommand(command);
      break;
    default:
      break;
    }
  }
}

/* Main function */
int main()
{
  mainShellLoop();
  return 0;
}
