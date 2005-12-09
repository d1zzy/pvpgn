/*
 * Copyright (C) 2001  Dizzy
 * Copyright (C) 2004  Donny Redmond (dredmond@linuxmail.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#define MAIL_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include "compat/strcasecmp.h"
#include <ctype.h>
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "compat/statmacros.h"
#include "compat/mkdir.h"
#include "compat/pdir.h"
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>
# endif
#endif
#include "message.h"
#include "connection.h"
#include "common/util.h"
#include "common/eventlog.h"
#include "common/xalloc.h"
#include "account.h"
#include "prefs.h"
#include "mail.h"
#include "common/setup_after.h"


namespace pvpgn
{

static int identify_mail_function(const char *);
static void mail_usage(t_connection*);
static void mail_func_send(t_connection*,const char *);
static void mail_func_read(t_connection*,const char *);
static void mail_func_delete(t_connection*,const char *);
static int get_mail_quota(t_account *);

/* Mail API */
/* for now this functions are only for internal use */
static t_mailbox * mailbox_open(t_account *, t_mbox_mode mode);
static int mailbox_count(t_mailbox *);
static int mailbox_deliver(t_mailbox *, const char *, const char *);
static t_mail * mailbox_read(t_mailbox *, unsigned int);
static void mailbox_unread(t_mail*);
static struct maillist_struct * mailbox_get_list(t_mailbox *);
static void mailbox_unget_list(struct maillist_struct *);
static int mailbox_delete(t_mailbox *, unsigned int);
static int mailbox_delete_all(t_mailbox *);
static void mailbox_close(t_mailbox *);

static char * clean_str(char *);


static t_mailbox * mailbox_open(t_account * user, t_mbox_mode mode) {
   t_mailbox * rez;
   char * path;
   char const * maildir;

   rez=(t_mailbox*)xmalloc(sizeof(t_mailbox));

   maildir=prefs_get_maildir();
   if (mode & mbox_mode_write)
      p_mkdir(maildir,S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

   path=(char*)xmalloc(strlen(maildir)+1+8+1);
   if (maildir[0]!='\0' && maildir[strlen(maildir)-1]=='/')
      sprintf(path,"%s%06u",maildir,account_get_uid(user));
   else
      sprintf(path,"%s/%06u",maildir,account_get_uid(user));

   if (mode & mbox_mode_write)
      p_mkdir(path,S_IRWXU | S_IXGRP | S_IRGRP | S_IROTH | S_IXOTH);

   if ((rez->maildir=p_opendir(path))==NULL) {
      if (mode & mbox_mode_write)
         eventlog(eventlog_level_error,__FUNCTION__,"error opening maildir");
      xfree(path);
      xfree(rez);
      return NULL;
   }

   rez->uid=account_get_uid(user);
   rez->path=path;
   return rez;
}

static int mailbox_count(t_mailbox *mailbox) {
   char const * dentry;
   int count=0;

   /* consider NULL mailbox as empty mailbox */
   if (mailbox==NULL) return 0;

   if (mailbox->maildir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
      return -1;
   }
   p_rewinddir(mailbox->maildir);
   while ((dentry=p_readdir(mailbox->maildir))!=NULL) if (dentry[0]!='.') count++;
   return count;
}

static int mailbox_deliver(t_mailbox * mailbox, const char * sender, const char * message) {
   FILE * fd;
   char * filename;

   if (mailbox==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL mailbox");
      return -1;
   }
   if (mailbox->maildir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
      return -1;
   }
   if (mailbox->path==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL path");
      return -1;
   }
   filename=(char*)xmalloc(strlen(mailbox->path)+1+15+1);
   sprintf(filename,"%s/%015lu",mailbox->path,(unsigned long)time(NULL));
   if ((fd=fopen(filename,"wb"))==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL file descriptor. check permissions");
      xfree(filename);
      return -1;
   }
   fprintf(fd,"%s\n",sender); /* write the sender on the first line of message */
   fprintf(fd,"%s\n",message); /* then write the actual message */
   fclose(fd);
   xfree(filename);
   return 0;
}

static t_mail * mailbox_read(t_mailbox * mailbox, unsigned int idx) {
   char const * dentry;
   unsigned int i;
   t_mail *     rez;
   FILE *       fd;
   char *       filename;

   if (mailbox==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL mailbox");
      return NULL;
   }
   if (mailbox->maildir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
      return NULL;
   }
   rez=(t_mail*)xmalloc(sizeof(t_mail));
   p_rewinddir(mailbox->maildir);
   dentry = NULL; /* if idx < 1 we should not crash :) */
   for(i=0;i<idx && (dentry=p_readdir(mailbox->maildir))!=NULL;i++)
     if (dentry[0]=='.') i--;
   if (dentry==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"index out of range");
      xfree(rez);
      return NULL;
   }
   rez->timestamp=atoi(dentry);
   filename=(char*)xmalloc(strlen(dentry)+1+strlen(mailbox->path)+1);
   sprintf(filename,"%s/%s",mailbox->path,dentry);
   if ((fd=fopen(filename,"rb"))==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"error while opening message");
      xfree(rez);
      xfree(filename);
      return NULL;
   }
   xfree(filename);
   rez->sender=(char*)xmalloc(256);
   fgets(rez->sender,256,fd); /* maybe 256 isnt the right value to bound a line but right now its all i have :) */
   clean_str(rez->sender);
   rez->message=(char*)xmalloc(256);
   fgets(rez->message,256,fd);
   clean_str(rez->message);
   fclose(fd);
   rez->timestamp=atoi(dentry);
   return rez;
}

static void mailbox_unread(t_mail * mail) {
   if (mail==NULL)
     eventlog(eventlog_level_error,__FUNCTION__,"got NULL mail");
   else {
      if (mail->sender==NULL)
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL sender");
      else xfree(mail->sender);
      if (mail->message==NULL)
	eventlog(eventlog_level_error,__FUNCTION__,"got NULL message");
      else xfree(mail->message);
      xfree(mail);
   }
}

static struct maillist_struct * mailbox_get_list(t_mailbox *mailbox) {
   char const * dentry;
   FILE * fd;
   struct maillist_struct *rez=NULL, *p=NULL,*q;
   char *sender,*filename;

   if (mailbox==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL mailbox");
      return NULL;
   }
   if (mailbox->maildir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
      return NULL;
   }
   filename=(char*)xmalloc(strlen(mailbox->path)+1+15+1);
   p_rewinddir(mailbox->maildir);
   for(;(dentry=p_readdir(mailbox->maildir))!=NULL;)
     if (dentry[0]!='.') {
	q=(struct maillist_struct*)xmalloc(sizeof(struct maillist_struct));
	sprintf(filename,"%s/%s",mailbox->path,dentry);
	if ((fd=fopen(filename,"rb"))==NULL) {
	   eventlog(eventlog_level_error,__FUNCTION__,"error while opening message file");
	   xfree(filename);
	   xfree(q);
	   return rez;
	}
	sender=(char*)xmalloc(256);
	fgets(sender,256,fd);
	clean_str(sender);
	fclose(fd);
	q->sender=sender;
	q->timestamp=atoi(dentry);
	q->next=NULL;
	if (p==NULL) rez=q;
	else p->next=q;
	p=q;
     }
   xfree(filename);
   return rez;
}

static void mailbox_unget_list(struct maillist_struct * maill) {
   struct maillist_struct *p, *q;

   for(p=maill;p!=NULL;p=q) {
      if (p->sender!=NULL) xfree(p->sender);
      q=p->next;
      xfree(p);
   }
}

static int mailbox_delete(t_mailbox * mailbox, unsigned int idx) {
   char *       filename;
   char const * dentry;
   unsigned int i;
   int          rez;

   if (mailbox==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL mailbox");
      return -1;
   }
   if (mailbox->maildir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
      return -1;
   }
   p_rewinddir(mailbox->maildir);
   dentry = NULL; /* if idx < 1 we should not crash :) */
   for(i=0;i<idx && (dentry=p_readdir(mailbox->maildir))!=NULL;i++)
     if (dentry[0]=='.') i--;
   if (dentry==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"index out of range");
      return -1;
   }
   filename=(char*)xmalloc(strlen(dentry)+1+strlen(mailbox->path)+1);
   sprintf(filename,"%s/%s",mailbox->path,dentry);
   rez=remove(filename);
   if (rez<0) {
       eventlog(eventlog_level_info,__FUNCTION__,"could not remove file \"%s\" (remove: %s)",filename,pstrerror(errno));
    }
   xfree(filename);
   return rez;
}

static int mailbox_delete_all(t_mailbox * mailbox) {
   char *       filename;
   char const * dentry;
   int          count;

   if (mailbox==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL mailbox");
      return -1;
   }
   if (mailbox->maildir==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
      return -1;
   }
   filename=(char*)xmalloc(strlen(mailbox->path)+1+15+1);
   p_rewinddir(mailbox->maildir);
   count = 0;
   while ((dentry=p_readdir(mailbox->maildir))!=NULL)
     if (dentry[0]!='.') {
	sprintf(filename,"%s/%s",mailbox->path,dentry);
	if (!remove(filename)) count++;
     }
   xfree(filename);
   return count;
}

static void mailbox_close(t_mailbox *mailbox) {
   if (mailbox==NULL) return;
   if (mailbox->maildir!=NULL) {
      p_closedir(mailbox->maildir);
   } else {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL maildir");
   }
   if (mailbox->path) xfree(mailbox->path);
   xfree(mailbox);
}

static char * clean_str(char * str) {
   char *p;

   for(p=str;*p!='\0';p++)
     if (*p=='\n' || *p=='\r') {
	*p='\0'; break;
     }
   return str;
}

extern int handle_mail_command(t_connection * c, char const * text)
{
   unsigned int i,j;
   char         comm[MAX_FUNC_LEN];

   if (!prefs_get_mail_support()) {
      message_send_text(c,message_type_error,c,"This server has NO mail support.");
      return -1;
   }

   for (i=0; text[i]!=' ' && text[i]!='\0'; i++); /* skip /mail command */
   for (; text[i]==' '; i++); /* skip any spaces after it */

   for (j=0; text[i]!=' ' && text[i]!='\0' && j<sizeof(comm)-1; i++) /* get function */
     if (j<sizeof(comm)-1) comm[j++] = text[i];
   comm[j] = '\0';

   switch (identify_mail_function(comm)) {
    case MAIL_FUNC_SEND:
      mail_func_send(c,text+i);
      break;
    case MAIL_FUNC_READ:
      mail_func_read(c,text+i);
      break;
    case MAIL_FUNC_DELETE:
      mail_func_delete(c,text+i);
      break;
    case MAIL_FUNC_HELP:
      message_send_text(c,message_type_info,c,"The mail command supports the following patterns.");
      mail_usage(c);
      break;
    default:
      message_send_text(c,message_type_error,c,"The command its incorrect. Use one of the following patterns.");
      mail_usage(c);
   }
   return 0;
}

static int identify_mail_function(const char *funcstr) {
    if (strcasecmp(funcstr,"send")==0 ||
        strcasecmp(funcstr,"s")==0)
	return MAIL_FUNC_SEND;
    if (strcasecmp(funcstr,"read")==0 ||
        strcasecmp(funcstr,"r")==0 ||
        strcasecmp(funcstr,"")==0)
	return MAIL_FUNC_READ;
    if (strcasecmp(funcstr,"delete")==0 ||
        strcasecmp(funcstr,"del")==0)
	return MAIL_FUNC_DELETE;
    if (strcasecmp(funcstr,"help")==0 ||
        strcasecmp(funcstr,"h")==0)
	return MAIL_FUNC_HELP;
    return MAIL_FUNC_UNKNOWN;
}

static int get_mail_quota(t_account * user) {
   int quota;
   char const * user_quota;

   user_quota=account_get_strattr(user,"BNET\\auth\\mailquota");
   if (user_quota==NULL) quota=prefs_get_mail_quota();
   else {
      quota=atoi(user_quota);
      if (quota<1) quota=1;
      if (quota>MAX_MAIL_QUOTA) quota=MAX_MAIL_QUOTA;
   }
   return quota;
}

static void mail_func_send(t_connection * c, const char * str) {
   int i;
   char *dest;
   char const *p,*myname;
   t_account * recv;
   t_mailbox * mailbox;

   if (c==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
      return;
   }
   if (str==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL command string");
      return;
   }
   for(i=0;str[i]==' ';i++); /* skip any spaces */
   if (str[i]=='\0') { /* the %mail send command has no receiver */
      message_send_text(c,message_type_error,c,"You must specify the receiver");
      message_send_text(c,message_type_error,c,"Syntax: /mail send <receiver> <message>");
      return;
   }
   p=str+i; /* set ip at the start of receiver string */
   for(i=1;p[i]!=' ' && p[i]!='\0';i++); /* skip the receiver string */
   if (p[i]=='\0') { /* it seems user forgot to write any message */
      message_send_text(c,message_type_error,c,"Your message is empty!");
      message_send_text(c,message_type_error,c,"Syntax: /mail send <receiver> <message>");
      return;
   }
   dest=(char*)xmalloc(i+1);
   memmove(dest,p,i); dest[i]='\0'; /* copy receiver in his separate string */
   if ((recv=accountlist_find_account(dest))==NULL) { /* is dest a valid account on this server ? */
      message_send_text(c,message_type_error,c,"Receiver UNKNOWN!");
      xfree(dest);
      return;
   }
   xfree(dest); /* free dest here, the sooner the better */
   if ((mailbox=mailbox_open(recv, mbox_mode_write))==NULL) {
      message_send_text(c,message_type_error,c,"There was an error completing your request!");
      return;
   }
   if (get_mail_quota(recv)<=mailbox_count(mailbox)) { /* check quota */
      message_send_text(c,message_type_error,c,"Receiver has reached his mail quota. Your message will NOT be sent.");
      mailbox_close(mailbox);
      return;
   }
   myname=conn_get_username(c); /* who am i ? */
   if (mailbox_deliver(mailbox,myname,p+i+1)<0)
     message_send_text(c,message_type_error,c,"There was an error completing your request!");
   else
     message_send_text(c,message_type_info,c,"Your mail has been sent successfully.");
   mailbox_close(mailbox);
}

static void mail_func_read(t_connection * c, const char * str) {
   t_account * user;
   t_mailbox * mailbox;
   const char *p;
   char tmp[256];
   int i;

   if (c==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
      return;
   }
   if (str==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL command string");
      return;
   }
   for(i=0;str[i]==' ';i++);
   p=str+i;
   if ((user=conn_get_account(c))==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL account");
      return;
   }

   mailbox=mailbox_open(user, mbox_mode_read);
   if (*p=='\0') { /* user wants to see the mail summary */
      struct maillist_struct *maill, *mp;
      unsigned int idx;

      if (!mailbox_count(mailbox)) {
	 message_send_text(c,message_type_info,c,"You have no mail.");
	 mailbox_close(mailbox);
	 return;
      }
      if ((maill=mailbox_get_list(mailbox))==NULL) {
	 eventlog(eventlog_level_error,__FUNCTION__,"got NULL maillist");
	 mailbox_close(mailbox);
	 return;
      }
      sprintf(tmp,"You have %d messages. Your mail qouta is set to %d.",mailbox_count(mailbox),get_mail_quota(user));
      message_send_text(c,message_type_info,c,tmp);
      message_send_text(c,message_type_info,c,"ID    Sender          Date");
      message_send_text(c,message_type_info,c,"-------------------------------------");
      for(mp=maill,idx=1;mp!=NULL;mp=mp->next,idx++) {
	 sprintf(tmp,"%02u    %-14s %s",idx,mp->sender,ctime(&mp->timestamp));
	 clean_str(tmp); /* ctime() appends an newline that we get cleaned */
	 message_send_text(c,message_type_info,c,tmp);
      }
      message_send_text(c,message_type_info,c,"Use /mail read <ID> to read the content of any message");
      mailbox_unget_list(maill);
   }
   else { /* user wants to read a message */
      int idx;
      t_mail * mail;

      for(i=0;p[i]>='0' && p[i]<='9' && p[i]!='\0';i++);
      if (p[i]!='\0' && p[i]!=' ') {
	 message_send_text(c,message_type_error,c,"Invalid index. Please use /mail read <index> where <index> is a number.");
	 mailbox_close(mailbox);
	 return;
      }
      idx=atoi(p);
      if (idx<1 || idx>mailbox_count(mailbox)) {
	 message_send_text(c,message_type_error,c,"That index is out of range.");
	 mailbox_close(mailbox);
	 return;
      }
      if ((mail=mailbox_read(mailbox,idx))==NULL) {
	 message_send_text(c,message_type_error,c,"There was an error completing your request.");
	 mailbox_close(mailbox);
	 return;
      }
      sprintf(tmp,"Message #%d from %s on %s:",idx,mail->sender,clean_str(ctime(&mail->timestamp)));
      message_send_text(c,message_type_info,c,tmp);
      message_send_text(c,message_type_info,c,mail->message);
      mailbox_unread(mail);
   }
   mailbox_close(mailbox);
}

static void mail_func_delete(t_connection * c, const char * str) {
   t_account * user;
   t_mailbox * mailbox;
   const char * p;
   char tmp[256]; /* that should be enough */
   int i;

   if (c==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
      return;
   }
   if (str==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL command string");
      return;
   }
   for(i=0;str[i]==' ';i++);
   p=str+i;
   if (*p=='\0') {
      message_send_text(c,message_type_error,c,"Please specify which message to delete. Use the following syntax: /mail delete {<index>|all} .");
      return;
   }
   if ((user=conn_get_account(c))==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL account");
      return;
   }
   if ((mailbox=mailbox_open(user, mbox_mode_write))==NULL) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL mailbox");
      return;
   }
   if (strcmp(p,"all")==0) {
      int rez;

      if ((rez=mailbox_delete_all(mailbox))<0) {
	 message_send_text(c,message_type_error,c,"There was an error completing your request.");
	 mailbox_close(mailbox);
	 return;
      }
      sprintf(tmp,"Successfuly deleted %d messages.",rez);
      message_send_text(c,message_type_info,c,tmp);
   }
   else {
      int idx;

      for(i=0;p[i]>='0' && p[i]<='9' && p[i]!='\0';i++);
      if (p[i]!='\0' && p[i]!=' ') {
	 message_send_text(c,message_type_error,c,"Invalid index. Please use /mail delete {<index>|all} where <index> is a number.");
	 mailbox_close(mailbox);
	 return;
      }
      idx=atoi(p);
      if (idx<1 || idx>mailbox_count(mailbox)) {
	 message_send_text(c,message_type_error,c,"That index is out of range.");
	 mailbox_close(mailbox);
	 return;
      }
      if (mailbox_delete(mailbox,idx)<0) {
	 message_send_text(c,message_type_error,c,"There was an error completing your request.");
	 mailbox_close(mailbox);
	 return;
      }
      sprintf(tmp,"Succesfully deleted message #%02d.",idx);
      message_send_text(c,message_type_info,c,tmp);
   }
   mailbox_close(mailbox);
}

static void mail_usage(t_connection * c) {
   message_send_text(c,message_type_info,c,"to print this information:");
   message_send_text(c,message_type_info,c,"    /mail help");
   message_send_text(c,message_type_info,c,"to print an index of you messages:");
   message_send_text(c,message_type_info,c,"    /mail [read]");
   message_send_text(c,message_type_info,c,"to send a message:");
   message_send_text(c,message_type_info,c,"    /mail send <receiver> <message>");
   message_send_text(c,message_type_info,c,"to read a message:");
   message_send_text(c,message_type_info,c,"    /mail read <index num>");
   message_send_text(c,message_type_info,c,"to delete a message:");
   message_send_text(c,message_type_info,c,"    /mail delete {<index>|all}");
   message_send_text(c,message_type_info,c,"Commands may be abbreviated as follows:");
   message_send_text(c,message_type_info,c,"    help: h");
   message_send_text(c,message_type_info,c,"    read: r");
   message_send_text(c,message_type_info,c,"    send: s");
   message_send_text(c,message_type_info,c,"    delete: del");
}

extern char const * check_mail(t_connection const * c) {
   t_account * user;
   t_mailbox * mailbox;
   static char tmp[64];
   int count;

   if (!(c)) {
      eventlog(eventlog_level_error,__FUNCTION__,"got NULL connection");
      return "";
   }

   if (!(user=conn_get_account(c)))
      return "";

   mailbox=mailbox_open(user, mbox_mode_read);
   count = mailbox_count(mailbox);
   mailbox_close(mailbox);

   if (count == 0)
   {
      return "You have no mail.";
   }
   else
   {
      sprintf(tmp,"You have %d message(s) in your mailbox.",count);
      return tmp;
   }
}

}
