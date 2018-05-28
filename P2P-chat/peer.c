#include <stdio.h>
#include "peer.h"
#include "stdlib.h"
#include "assert.h"

#define ARGNUM 1 // TODO: Put the number of you want to take

volatile int kill_chat;
volatile int kill_reader;
pthread_mutex_t thread_lock = PTHREAD_MUTEX_INITIALIZER;
struct user_list *show_list;
struct user_list* active_users;

struct user_message* get_info(char* buf)
{
  char** split_words = (char**)Calloc(64*128,sizeof(char));
  int index1 = 0;
  int index2 = 0;
  int s_index = 0;
  split_words[s_index] = (char*)Calloc(128,sizeof(char));
  while (buf[index1] != '\n')
  {
    char elem = buf[index1];
    if (elem == ' ')
    {
      s_index++;
      index1++;
      index2 = 0;
      split_words[s_index] = (char*)Calloc(128,sizeof(char));
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

void* reader(void* arg)
{
  int connfd = *(int*)arg;
  //we recieve the name first.
  rio_t rio;
  char* from_buf = Calloc(MAXLINE,sizeof(char));
  Rio_readinitb(&rio, connfd);
  if (Rio_readlineb(&rio,from_buf,MAXLINE)==0)
  {
    return NULL;
  }
  int size_of_name = strlen(from_buf);
  char* username = Calloc(size_of_name-1,sizeof(char));
  strcat(username,from_buf);
  username[size_of_name-1] = '\0';
  Rio_writen(connfd,"Confirmed\n",10);
  struct user* user = Malloc(sizeof(struct user));
  linked_queue* LQ = Malloc(sizeof(linked_queue));
  set_empty_queue(LQ);
  user->buffer = LQ;
  user->username = username;
  user->logged_out = Malloc(sizeof(int));
  *user->logged_out = 0;
  List_Push_User(show_list,user);
  int keep_conn = 1;
  free(from_buf);
  while (keep_conn)
  {
    char* from_buf = Calloc(MAXLINE,sizeof(char));
    if (kill_reader)
    {
      keep_conn = 0;
    }
    else
    {
      if (rio_writen(connfd,"Online\n",8)<= 0)
      {
        keep_conn = 0;
      }
      Rio_readinitb(&rio, connfd);
      if (rio_readlineb(&rio,from_buf,MAXLINE) <= 0)
      {
        keep_conn = 0;
      }
      else if (!strcmp(from_buf,"><.\n"))
      {
        /* code */
      }
      else
      {
        enqueue(user->buffer,from_buf,strlen(from_buf));
      }
    }
    free(from_buf);
  }
  *user->logged_out = 1;
  close(connfd);
  return NULL;
}

void* chatter(void* arg)
{
  struct user *user_info = (struct user*) arg;
  int clientfd = Open_clientfd(user_info->hostaddr,user_info->port);
  //we send the name first.
  int name_size = strlen(user_info->own_username);
  char* name_buf = Calloc(name_size+1,sizeof(char));
  char from_buf[MAXLINE];
  strcat(name_buf,user_info->own_username);
  strcat(name_buf,"\n");
  Rio_writen(clientfd,name_buf,name_size+1);
  //prepare to read and write to user
  rio_t rio_buf;
  Rio_readinitb(&rio_buf, clientfd);
  if (Rio_readlineb(&rio_buf,from_buf,MAXLINE)==0)
  {
    return NULL;
  }  //maintain communication
  int keep_conn = 1;
  while (keep_conn)
  {
    char* write_buf = Calloc(MAXLINE,sizeof(char));
    if (kill_chat)
    {
      keep_conn = 0;
      List_Pop_Check_User(active_users,user_info->username,0,0);
    }
    else if (*user_info->logged_out || Rio_readlineb(&rio_buf,from_buf,MAXLINE)==0)
    {
      keep_conn = 0;
      List_Pop_Check_User(active_users,user_info->username,0,0);
    }
    else
    {
      int queue_empty = dequeue(user_info->buffer, write_buf, (MAXLINE)*sizeof(char));
      if (queue_empty >= 0)
      {
        strcat(write_buf,"\n");
        if (rio_writen(clientfd,write_buf, MAXLINE)<= 0)
        {
          keep_conn = 0;
          List_Pop_Check_User(active_users,user_info->username,0,0);
        }
      }
      else
      {
        strcat(write_buf,"><.\n");
        if (rio_writen(clientfd,write_buf, MAXLINE)<= 0)
        {
          keep_conn = 0;
          List_Pop_Check_User(active_users,user_info->username,0,0);
        }
      }
    }
    free(write_buf);
  }
  close(clientfd);
  return NULL;
}

int main(int argc, char**argv) 
{
	int exit_called = 0;
  kill_chat = 0;
  if (argc != ARGNUM + 1) {
    printf("%s expects %d arguments.\n", (argv[0]+2), ARGNUM);
    return(0);
  }
  pthread_t *threads = Calloc(5000, sizeof(pthread_t));
  assert(threads != NULL);
  int thread_number = -1;
  pthread_t listen_thread;
  int clientfd = Open_clientfd(argv[1],"1430");
  rio_t server_output;
  size_t size = MAXLINE*sizeof(char);
  char server_buf[size];
  int connected_to_server = 1;
  int logged_in = 0;
  int message = 0;
  active_users = Make_list();
  char* username;
  show_list = Make_list();
  int listenfd;
  int listening = 0;
  char* port = "0000";
  while (!exit_called)
  {
    char* client_buf = Calloc(MAXLINE,sizeof(char));
    Rio_readinitb(&server_output, clientfd);
    assert(getline(&client_buf, &size, stdin)!=-1);
    struct user_message* msg = get_info(client_buf);
    if (!strcmp(msg->words[0],"/msg") && msg->number_of_words >= 3)
    {
      message = 1;
    }
    else
    {
      Rio_writen(clientfd, client_buf, MAXLINE);
    }
    if (!strcmp(msg->words[0],"/lookup") || message)
    {
      if (message)
      {
        char* server_lookup = Calloc(MAXLINE,sizeof(char));
        strcat(server_lookup,"/lookup ");
        strcat(server_lookup,msg->words[1]);
        strcat(server_lookup,"\n");
        Rio_writen(clientfd, server_lookup, MAXLINE);
        free(server_lookup);
      }
      int ended = 0;
      int length = 0;
      char* user_info = Calloc(MAXLINE,sizeof(char));
      while (!ended)
      {
        length++;
        Rio_readlineb(&server_output,server_buf,MAXLINE);
        int serv_msg_size = strlen(server_buf);
        if (serv_msg_size == 1)
        {
          ended = 1;
        }
        else if (message)
        {
          strcat(user_info,server_buf);
        }
        else
        {
          printf("%s", server_buf);
        }
      }
      if (message)
      {
        if (length <= 2) //If server responds with 2 lines.
        {
          assert(pthread_mutex_lock(&thread_lock)==0);
          struct user* listed_user = List_Pop_Check_User(active_users,msg->words[1],1,0);
          if (listed_user != NULL)
          {
            *listed_user->logged_out = 1;
            printf("Unable to send message. %s is not logged in anymore.\n", msg->words[1]);
          }
          else
          {
            printf("%s %s\n", "Unable to message",msg->words[1]);
          }
          assert(pthread_mutex_unlock(&thread_lock)==0);
        }
        else
        {
          char* input_buffer = Calloc(MAXLINE,sizeof(char)); //we make room for 64 spaces
          for (int i = 2; i < msg->number_of_words; i++)
          {
            strcat(input_buffer,msg->words[i]);
            strcat(input_buffer," ");
          }
          assert(pthread_mutex_lock(&thread_lock)==0);
          struct user* user_netinfo = extract_netinfo(user_info);
          struct user* listed_user = List_Pop_Check_User(active_users,user_netinfo->username,1,0);
          if (listed_user == NULL)
          {
            user_netinfo->own_username = username;
            List_Push_User(active_users,user_netinfo);
            enqueue(user_netinfo->buffer, input_buffer, strlen(input_buffer)*sizeof(char));
            thread_number++;
            Pthread_create(&threads[thread_number], NULL, &chatter, (void*)user_netinfo);          
          }
          if (listed_user != NULL)
          {
            enqueue(listed_user->buffer, input_buffer, strlen(input_buffer)*sizeof(char));
          }
          free(user_info);
          assert(pthread_mutex_unlock(&thread_lock)==0);
        }
        message = 0;
      }
    }
    else
    { 
      connected_to_server = Rio_readlineb(&server_output,server_buf,MAXLINE);
      if (!strcmp(msg->words[0],"/logout") && logged_in)
      {
        logged_in = 0;
        shutdown(listenfd,SHUT_RD);
        kill_reader = 1;
        kill_chat = 1;
        free(username);
      }
      if (!logged_in && !strcmp(msg->words[0],"/login"))
      {
        if (!strcmp(server_buf,"You are now logged in.\n"))
        {
          username = strdup(msg->words[1]);
          port = msg->words[4];
          logged_in = 1;
          kill_reader = 0;
          kill_chat = 0;
        }
      }
      if (logged_in && !listening)
      {
        listenfd = Open_listenfd(port);
        Pthread_create(&listen_thread, NULL, &listener, (void*)&listenfd);
        listening = 1;
      }
      if (!logged_in && listening)
      {
        Pthread_join(listen_thread, NULL);
        listening = 0;
      }
      if (!connected_to_server || !strcmp(client_buf,"/exit\n"))
      {
        exit_called=1;
      }
      else if (!strcmp(msg->words[0],"/show") && msg->number_of_words <= 2)
      {
        assert(pthread_mutex_lock(&thread_lock)==0);
        if (msg->number_of_words == 2)
        {
          struct user* user_to_show = user_exist(msg->words[1]); 
          if (user_to_show != NULL)
          {
            show_message(user_to_show,0);
          }
          else
          {
            printf("%s%s\n", "Cannot show messages from ",msg->words[1]);
          }
        }
        else
        {
          all_show_message();
        }
        assert(pthread_mutex_unlock(&thread_lock)==0);
      }
      else
      {
        printf("%s", server_buf);
      }
    }
    for (int i = 0; i < msg->number_of_words; i++)
    {
      free(msg->words[i]);
    }
    free(msg->words);
    free(msg);
    free(client_buf);
  }
  if (exit_called && logged_in && listening)
  {
    free(username);
    shutdown(listenfd,SHUT_RD);
    kill_reader = 1;
    kill_chat = 1;
    Pthread_join(listen_thread, NULL);
  }

  for (int i = 0; i < (thread_number + 1); i++) {
    Pthread_join(threads[i], NULL);
  }
  free(threads);
  close(clientfd);
  return 1;
}


void* listener(void* arg) {
  int listenfd = *(int*)arg;
  int connfd;
  SA sockaddress;
  unsigned int sockaddr_len = sizeof(SA);
  pthread_t *threads = Calloc(5000, sizeof(pthread_t));
  int *conns = Calloc(5000, sizeof(int));
  assert(threads != NULL);
  assert(conns != NULL);
  int thread_number = -1;
  int logout = 0;
  while (!logout)
  {
    //In here we make the client threads
    connfd = accept(listenfd,(SA*) &sockaddress,&sockaddr_len);
    //If connfd is less than 0, the server has been terminated,
    //or some other error occured in accept.
    if (connfd < 0)
    {
      logout = 1;
    }
    else
    {
      thread_number++;
      conns[thread_number] = connfd;
      Pthread_create(&threads[thread_number], NULL, &reader, (void*)&conns[thread_number]);
    }
  }
  //Here we free all clients at once, we should free them each
  //time we accept a new one in the future.
  for (int i = 0; i < (thread_number + 1); i++) {
    shutdown(conns[i],SHUT_RDWR);
    Pthread_join(threads[i], NULL);
  }
  free(threads);
  free(conns);
  close(listenfd);
  return NULL;
}


struct newlines get_newlines(char* string)
{
  struct newlines retval;
  char cur = ' ';
  int index = 0;
  int n_newlines = 0;
  while (cur != '\0')
  {
    cur = string[index];
    if (cur == '\n')
    {
      string[index] = '\0';
      if (3 > n_newlines) //we want to know the placement of 2 newline chars
      {
        retval.index[n_newlines] = index + 1;
        n_newlines++;
      }
    }
    index++;
  }
  return retval;
}

struct user* extract_netinfo(char* raw_netinfo)
{
  struct user *retval = Malloc(sizeof(struct user));
  char* data = strdup(raw_netinfo);
  struct newlines NL = get_newlines(data);
  retval->logged_out = Malloc(sizeof(int));
  retval->username = strdup(&data[NL.index[0]+6]); //we skip Nick: formatting of username
  retval->hostaddr = strdup(&data[NL.index[1]+4]); //we skip IP: formatting of IP
  retval->port = strdup(&data[NL.index[2]+6]); //we skip Port: formatting of port.
  retval->buffer = Malloc(sizeof(linked_queue));
  *retval->logged_out = 0;
  free(data);
  set_empty_queue(retval->buffer); 
  return retval;
}

struct user_list* Make_list()
{
  struct user_list* UL_input = Malloc(sizeof(struct user_list));
  assert(UL_input != NULL);
  UL_input->length = Malloc(sizeof(int));
  struct node* head = Malloc(sizeof(struct node));
  UL_input->head = head;
  assert(UL_input->length != NULL);
  *UL_input->length = 0;
  return UL_input;
}

void List_Push_User(struct user_list* UL_input, struct user* add_user)
{
  struct node* new_head = Malloc(sizeof(struct node));
  struct node* old_head = UL_input->head;
  new_head->cur = add_user;
  new_head->next = old_head;
  old_head->prev = new_head;
  *UL_input->length += 1;
  UL_input->head = new_head;
  return;
}

//If it returns NULL user does not exist.
struct user* List_Pop_Check_User(struct user_list* UL_input, char* username,int check,int logcheck)
{
  int count = *UL_input->length;
  int index = 0;
  struct node* cur_node = UL_input->head;
  struct node* prev_node;
  int found = 0;
  while (index < count && !found)
  {
    struct user* user = cur_node->cur;
    if (strcmp(user->username,username) == 0)
    {
      if (logcheck)
      {
        if (*user->logged_out)
        {
          found = 1;
        }
      }
      else
      {
        found = 1;
      }
    }
    prev_node = cur_node;
    cur_node = prev_node->next;
    index++;
  }
  if (found && !check)
  {
    if (prev_node->prev == NULL)
    {
      prev_node->next->prev = prev_node->prev;
    }
    else
    {
      prev_node->prev->next = cur_node;
      prev_node->next->prev = prev_node->prev;
    }
    struct user* user_memory = prev_node->cur;
    free(user_memory->username);
    free(user_memory->hostaddr);
    free(user_memory->port);
    free(user_memory->buffer);
    user_memory->buffer = NULL;
    *UL_input->length -= 1;
    return user_memory;
  }
  else if (found && check)
  {
    return prev_node->cur;
  }
  else
  {
    struct user* return_val = NULL;
    return return_val;
  }
}

struct user* user_exist(char* username_in)
{
  struct user* retval = List_Pop_Check_User(show_list, username_in ,1,0);
  return retval;
}

int show_message(struct user* user_in, int all)
{
  char* username_none = strdup(user_in->username);
  int username_size = strlen(user_in->username);
  char* username = Calloc(username_size+2,sizeof(char));
  strcat(username,user_in->username);
  username[username_size] = ':';
  username[username_size+1] = ' ';
  int done = 0;
  int shown = 0;
  while(done != 1)
  {
    char* show_buf = Calloc(MAXLINE,sizeof(char));
    if (dequeue(user_in->buffer, show_buf, (MAXLINE)*sizeof(char))>= 0)
    {
      shown++;
      char* show_msg = Calloc(username_size+1+MAXLINE,sizeof(char));
      strcat(show_msg,username);
      strcat(show_msg,show_buf);
      printf("%s", show_msg);
      free(show_msg);
    }
    else
    {
      done = 1;
    }
    free(show_buf);
  }
  if (*user_in->logged_out == 1)
  {
    List_Pop_Check_User(show_list, user_in->username,0,1);
    if (shown == 0 && !all)
    {
      printf("%s%s\n", "Cannot show messages from ",username_none);
      free(username_none);
    }
    else
    {
      free(username_none);
    }
  }
  else if (*user_in->logged_out == 0 && shown == 0 && !all)
  {
    printf("%s%s\n", "No new messages from ",username_none);
    free(username_none);
  }
  else
  {
    free(username_none);
  }
  return shown;
}

void all_show_message()
{
  struct user_list* userlist = show_list;
  int count = *userlist->length;
  if (count > 0)
  {
    struct user** all_users = Malloc(count*sizeof(struct user*));
    struct node* cur_node = userlist->head;
    struct node* prev_node;
    int index = 0;
    int shown = 0;
    while (count > index)
    {
      struct user* user = cur_node->cur;
      all_users[index] = user;
      prev_node = cur_node;
      cur_node = prev_node->next;
      index++;
    }
    for (int i = 0; i < index; i++)
    {
      shown += show_message(all_users[i],1);
    }
    if (shown == 0)
    {
      printf("%s\n", "No new messages from any user");
    }
    free(all_users);
  }
  else
  {
    printf("%s\n", "No new messages from any user");
  }
}

