#include "csapp.h" //You can remove this if you do not wish to use the helper functions
#include "buffer.h"

struct user_message
{
	char** words;
	int number_of_words;
};

struct user
{
  char* own_username;
  char* username;
  char* hostaddr;
  char* port;
  linked_queue *buffer;
  int* logged_out;
  int* connfd;
};

struct node
{
  struct user *cur;
  struct node *next;
  struct node *prev;
};

struct user_list
{
  struct node *head;
  int *length; //0 if false, 1 if true
};

struct newlines
{
  int index[3]; //We need to know where 2 newlines are
};

struct newlines get_newlines(char* string);

struct user* extract_netinfo(char* raw_netinfo);

struct user_list* Make_list();

void List_Push_User(struct user_list* UL_input, struct user* add_user);

struct user* List_Pop_Check_User(struct user_list* UL_input, char* username,int check,int logcheck);

void* listener(void* arg);

struct user* user_exist(char* username_in);

int show_message(struct user* user_in, int all);

void all_show_message();

