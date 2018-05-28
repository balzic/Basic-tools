#include <stdio.h>
#include "name_server.h"
#include "stdlib.h"
#include <err.h>
#include "string.h"
#define ARGNUM 0 // TODO: Put the number of you want to take

int listenfd;
int silent = 0;
int clients_connected = 0;
pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;
UL *user_list;

void* worker(void* cl_handler) {
  struct c_handle_input *CHI = (struct c_handle_input*) cl_handler;
  int* connfd = &(CHI->connfd);
  rio_t from_rio;
  char from_buf[MAXLINE];
  char* to_buf;
  char* logged_username;
  int end_of_conn = 0;
  int logged_in = 0;
  int stream_open = 1;
  while (!end_of_conn)
  {
    Rio_readinitb(&from_rio, *connfd);
    stream_open = Rio_readlineb(&from_rio,from_buf,MAXLINE);
    if (stream_open == 0)
    {
      end_of_conn = 1;
    }
    else
    {
      struct user_message* user_input;
      user_input = get_info(from_buf);
      char* msg = user_input->words[0];
      if (user_input->number_of_words > 0 && user_input->number_of_words <= 5)
      {
        if (user_input->number_of_words == 5 && !strcmp(msg,"/login") && !logged_in)
        {
          char* username = user_input->words[1];
          char* password = user_input->words[2];
          char* hostaddr = user_input->words[3];
          char* port = user_input->words[4];
          pthread_mutex_lock(&client_lock);
          if (user_login(username,password,hostaddr,port)==1)
          {
            logged_username = strdup(username);
            logged_in = 1;
            to_buf = "You are now logged in.\n";
            Rio_writen(*connfd, to_buf, 23*sizeof(char));
          }
          else
          {
            to_buf = "Login failed.\n";
            Rio_writen(*connfd, to_buf, 14*sizeof(char));
          }
          pthread_mutex_unlock(&client_lock);
        }
        else if (user_input->number_of_words <= 2 && !strcmp(msg,"/lookup") && logged_in)
        {
          char* user;
          if (user_input->number_of_words == 2)
          {
            user = user_input->words[1];
          }
          else
          {
            user = "\0";
          }
          to_buf = do_lookup(user);
          Rio_writen(*connfd, to_buf, strlen(to_buf));
          Rio_writen(*connfd, "\n", 1*sizeof(char));
          free(to_buf);
        }
        else if (user_input->number_of_words == 1 && !strcmp(msg,"/logout") && logged_in)
        {
          if (!user_logout(logged_username))
          {
            logged_in = 0;
            to_buf = "You are now logged out.\n";
            Rio_writen(*connfd, to_buf, 24*sizeof(char));
          }
          else
          {
            to_buf = "Logout failed.\n";
            Rio_writen(*connfd, to_buf, 15*sizeof(char));
          }
        }
        else
        {
          if (!logged_in)
          {
            if (!strcmp(msg,"/lookup"))
            {
              to_buf = "You are not logged in.\n";
              Rio_writen(*connfd, to_buf, 23*sizeof(char));
              Rio_writen(*connfd, "\n", 1*sizeof(char));
            }
            else
            {
              to_buf = "You are not logged in.\n";
              Rio_writen(*connfd, to_buf, 23*sizeof(char));
            }
          }
          else
          {
            to_buf = "Invalid command.\n";
            Rio_writen(*connfd, to_buf, 17*sizeof(char));
          }        
        }
      } 
      for (int i = 0; i < user_input->number_of_words; i++)
      {
        free(user_input->words[i]);
      }
      free(user_input->words);
      free(user_input);
    }
  }
  if (logged_in)
  {
    user_logout(logged_username);
  }
  assert(pthread_mutex_lock(&client_lock)==0);
  clients_connected -= 1;
  close(*connfd);
  free(CHI);
  printf("\nname_server: %d client(s) connected\n", clients_connected);
  assert(pthread_mutex_unlock(&client_lock)==0);
  return NULL;
}

char* do_lookup(char* user)
{
  int end_of_list = 0;
  UL *cur_node = user_list;
  char *output = Calloc(4096,sizeof(char));
  char *real_output = Calloc(4096,sizeof(char));
  if (strcmp(user,"\0"))
  {
    int match = 0;
    int online = 0;
    while (!end_of_list)
    {
      struct user *cur_user = cur_node->cur;
      if (!strcmp(cur_user->username,user))
      {
        if (cur_user->logged_in==1)
        {
          strcat(output,"Nick: ");
          strcat(output,cur_user->username);
          strcat(output,"\n");
          strcat(output,"IP: ");
          strcat(output,cur_user->hostaddr);
          strcat(output,"\n");
          strcat(output,"Port: ");
          strcat(output,cur_user->port);
          strcat(output,"\n");
          online = 1;
        }
        end_of_list = 1;
        match = 1;
      }
      if (*(cur_node->end_of_list))
      {
        end_of_list = 1;
        break;
      }
      else
      {
        cur_node = cur_node->next;
      }
    }
    if (match)
    {
      if (online)
      {
        sprintf(real_output,"%s is online.\n%s",user,output);
        free(output);
        return real_output;
      }
      else
      {
        sprintf(real_output,"%s is offline.\n",user);
        free(output);
        return real_output;
      }
    }
    else
    {
      sprintf(real_output,"%s is not a valid user.\n",user);
      free(output);
      return real_output;
    }
  }
  else
  {
    int number_of_users = 0;
    while (!end_of_list) 
    {
      struct user *cur_user = cur_node->cur;
      if (cur_user->logged_in == 1)
      {
        number_of_users++;
        strcat(output,"Nick: ");
        strcat(output,cur_user->username);
        strcat(output,"\n");
        strcat(output,"IP: ");
        strcat(output,cur_user->hostaddr);
        strcat(output,"\n");
        strcat(output,"Port: ");
        strcat(output,cur_user->port);
        strcat(output,"\n");
      }
      if (*(cur_node->end_of_list))
      {
        end_of_list = 1;
      }
      else
      {
        cur_node = cur_node->next;
      }
    }
    sprintf(real_output,"%d %s\n%s",number_of_users, "user(s) online. The list follows",output);
    free(output);
    return real_output;
  }
}

int user_login(char* username, char* password, char* hostaddr, char* port)
{
  int end_of_list = 0;
  int logged_in = 0;
  UL *cur_node = user_list;
  while (!end_of_list)
  {
    struct user *cur_user = cur_node->cur;
    if (!strcmp(cur_user->username,username) && !strcmp(cur_user->password,password) && cur_user->logged_in != 1)
    {
      cur_user->hostaddr = strdup(hostaddr);
      cur_user->port = strdup(port);
      cur_user->logged_in = 1;
      logged_in = 1;
      end_of_list = 1;
    }
    else if (*(cur_node->end_of_list))
    {
      end_of_list = 1;
    }
    else
    {
      cur_node = cur_node->next;
    }
  }
  return logged_in;
}

int user_logout(char* username)
{
  int end_of_list = 0;
  int logged_in = 1;
  UL *cur_node = user_list;
  while (!end_of_list)
  {
    struct user *cur_user = cur_node->cur;
    if (!strcmp(cur_user->username,username) && cur_user->logged_in == 1)
    {
      free(cur_user->hostaddr);
      free(cur_user->port);
      cur_user->logged_in = 0;
      logged_in = 0;
      end_of_list = 1;
    }
    else if (*(cur_node->end_of_list))
    {
      end_of_list = 1;
    }
    else
    {
      cur_node = cur_node->next;
    }
  }
  free(username);
  return logged_in;
}

struct user_message* get_info(char* buf)
{
  char** split_words = Calloc(64,sizeof(char));
  int index1 = 0;
  int index2 = 0;
  int s_index = 0;
  split_words[s_index] = Calloc(128,sizeof(char));
  while (buf[index1] != '\n')
  {
    char elem = buf[index1];
    if (elem == ' ')
    {
      s_index++;
      index1++;
      index2 = 0;
      split_words[s_index] = Calloc(128,sizeof(char));
    }
    else
    {
      split_words[s_index][index2] = elem;
      index2++;
      index1++;
    }
  }
  struct user_message* retval = Malloc(sizeof(struct user_message));
  retval->words = split_words;
  retval->number_of_words = s_index+1;
  return retval;
}



void* listener(void* arg) {
  arg = arg;
  int connfd;
  SA sockaddress;
  unsigned int sockaddr_len = sizeof(SA);
  pthread_t *threads = Calloc(5000, sizeof(pthread_t));
  assert(threads != NULL);
  int thread_number = -1;
  while (!silent)
  {
    //In here we make the client threads
    connfd = accept(listenfd,(SA*) &sockaddress,&sockaddr_len);
    //If connfd is less than 0, the server has been terminated,
    //or some other error occured in accept.
    if (connfd<0)
    {
      printf("\nname_server: server ending when all clients are disconnected\n");
    }
    else
    {
      assert(pthread_mutex_lock(&client_lock)==0);
      clients_connected++;
      thread_number++;
      struct c_handle_input *new_connection = Malloc(sizeof(struct c_handle_input));
      new_connection->connfd = connfd;
      new_connection->thread_number = thread_number;
      printf("\nname_server: %d client(s) connected\n", clients_connected);
      Pthread_create(&threads[thread_number], NULL, &worker, (void*)new_connection);
      assert(pthread_mutex_unlock(&client_lock)==0);
    }
  }
  //Here we free all clients at once, we should free them each
  //time we accept a new one in the future.
  for (int i = 0; i < (thread_number + 1); i++) {
    Pthread_join(threads[i], NULL);
  }
  free(threads);
  return NULL;
}

//The server takes no input, it always listens on port 1430
int main(int argc, char**argv) 
{
  if (argc != ARGNUM + 1) 
  {
      printf("%s expects %d arguments.\n", (argv[0]+2), ARGNUM);
      return(0);
  }
  char* listen_port = "1430";
  listenfd = Open_listenfd(listen_port);
  pthread_t listenthread;
  //We make the listen thread
  Pthread_create(&listenthread, NULL, &listener, NULL);
  size_t exitcode_size = 5*sizeof(char);
  char* exitcode = Malloc(exitcode_size);
  
  user_list = Malloc(sizeof(UL));
  user_list->next = Malloc(sizeof(UL));
  user_list->next->next = Malloc(sizeof(UL));
  user_list->cur = Malloc(sizeof(struct user));
  user_list->next->cur = Malloc(sizeof(struct user));
  user_list->next->next->cur = Malloc(sizeof(struct user));
  user_list->cur->username = "Hans";
  user_list->cur->password = "Schilling!!";
  user_list->end_of_list = Malloc(sizeof(int));
  *user_list->end_of_list = 0;
  user_list->cur->logged_in = 0;
  ((user_list->next)->cur)->username = "Figaro";
  ((user_list->next)->cur)->password = "onlythegood";
  user_list->next->end_of_list = Malloc(sizeof(int));
  *user_list->next->end_of_list = 0;
  ((user_list->next)->cur)->logged_in = 0;
  user_list->next->next->cur->username = "Subject";
  user_list->next->next->cur->password = "Seventeen";
  user_list->next->next->end_of_list = Malloc(sizeof(int));
  *user_list->next->next->end_of_list = 1;
  user_list->next->next->cur->logged_in = 0;

  //In here we make sure that the server can stop at any time
  while (!silent)
  {
    printf("name_server: write 'stop' and press enter to end this server -  ");
  	assert(getline(&exitcode, &exitcode_size, stdin)!=-1);
    if (!(strcmp(exitcode,"stop\n")))
    {
      silent = 1;
      shutdown(listenfd,SHUT_RD);
    }
  }

  Pthread_join(listenthread, NULL);
  printf("name_server: all clients are disconnected, server ending\n");
  free(exitcode);
  free(user_list->cur);
  free(user_list->end_of_list);
  free(user_list->next->cur);
  free(user_list->next->end_of_list);
  free(user_list->next);
  free(user_list->next->next->cur);
  free(user_list->next->next->end_of_list);
  free(user_list->next->next);
  free(user_list);
  close(listenfd);

  return 0;
}