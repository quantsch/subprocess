#include "subprocess.h"

#include <stdio.h>
#include <string.h>

static char * strjoin (char *const* _array);




int spawn_process (process_handle_t * _handle, const char * _command, char *const _arguments[], char *const _environment[])
{
  memset(_handle, 0, sizeof(process_handle_t));
  _handle->state = NOT_STARTED;
  
  /* if the command is part of arguments, pass NULL to CreateProcess */
  if (!strcmp(_arguments[0], _command)) {
    _command = NULL;
  }

  /* put all arguments into one line */
  char * command_line = strjoin(_arguments);

  PROCESS_INFORMATION pi;
  STARTUPINFO si = {sizeof(STARTUPINFO)};

  memset(&pi, 0, sizeof(PROCESS_INFORMATION));

  BOOL rc = CreateProcess(_command,     // lpApplicationName
                          command_line, // lpCommandLine
                          NULL,         // lpProcessAttributes
                          NULL,         // lpThreadAttributes
                          FALSE,        // bInheritHandles
                          CREATE_NO_WINDOW, // dwCreationFlags
                          (void**)_environment, // lpEnvironment
                          NULL,         // lpCurrentDirectory
                          &si,          // lpStartupInfo
                          &pi);         // lpProcessInformation

  free(command_line);

  /* translate from Windows to Linux; -1 means error */
  if (!rc) {
    return -1;
  }

  /* close thread handle but keep the process handle */
  CloseHandle(pi.hThread);
  
  _handle->child_id = pi.dwProcessId;
  
  /* again, in Linux 0 is "good" */
  return 0;
}


int teardown_process (process_handle_t * _handle)
{
  if (_handle->state != RUNNING) {
    return 0;
  }

  // Wait until child process exits.
  WaitForSingleObject(_handle->child_handle, INFINITE);
  
  DWORD status;
  GetExitCodeProcess(_handle->child_handle, &status);

  if (status == STILL_ACTIVE) {
    return -1;
  }
    
  _handle->return_code = (int)status;
    
  // Close process and thread handles. 
  CloseHandle(_handle->child_handle);

  _handle->state = EXITED;
  
  
  return 0;
}

ssize_t process_write (process_handle_t * _handle, const void * _buffer, size_t _count)
{
  return 0;
}

ssize_t process_read (process_handle_t * _handle, pipe_t _pipe, void * _buffer, size_t _count)
{
  return 0;
}

int process_poll (process_handle_t * _handle, int _wait)
{
  return 0;
}


/**
 * You have to Free() the buffer returned from this function
 * yourself - or let R do it, since we allocate it with Calloc().
 */
static char * strjoin (char *const* _array)
{
  /* total length is the sum of lengths plus spaces */
  size_t total_length = 0;
  char *const* ptr;

  for ( ptr = _array ; *ptr != NULL; ++ptr) {
    total_length += strlen(*ptr) + 1; /* +1 for space */
  }

  char * buffer = (char*)malloc(total_length + 1);

  /* combine all parts, put spaces between them */
  char * tail = buffer;
  for ( ptr = _array ; *ptr != NULL; ++ptr, ++tail) {
    size_t len = strlen(*ptr);
    strncpy(tail, *ptr, len);
    tail += len;
    *tail = ' ';
  }

  *tail = 0;

  return buffer;
}


#ifdef WINDOWS_TEST
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  const char * command = "x";
  char * args[] = { "x", NULL };
  char * env[]  = { NULL };
  
  process_handle_t handle;
  if (spawn_process(&handle, command, args, env) < 0) {
    fprintf(stderr, "error in spawn_process\n");
    exit(EXIT_FAILURE);
  }
  
  teardown_process(&handle);
  
  return 0;
}
#endif /* WINDOWS_TEST */