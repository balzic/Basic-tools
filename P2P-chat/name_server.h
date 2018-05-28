#include "csapp.h" //You can remove this if you do not wish to use the helper functions
#include "assert.h"


struct user
{
  char* username;
  char* password;
  char* hostaddr;
  char* port;
  int logged_in;
};

struct user_list
{
  struct user *cur;
  struct user_list *next;
  int *end_of_list; //0 if false, 1 if true
};

struct user_message
{
	char** words;
	int number_of_words;
};

struct c_handle_input
{
  int connfd;
  int thread_number;
};

typedef struct user_list UL;


struct user_message* get_info(char* buf);

int user_login(char* username, char* password, char* hostaddr, char* port);

char* do_lookup(char* user);

int user_logout(char* username);