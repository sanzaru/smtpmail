/********************************************************************************************
	smtpMail C library

	Author: Martin Albrecht <martin.albrecht@javacoffee.de>
	Contributor: Eric Nichols <eric@dirwiz.com>
	Date: August 2009
	Website: http://code.javacofee.de


DESCRIPTION:
------------
	The smtpMail library provides a simple class like interface for sending E-Mails
	over SMTP from a C language program.

	See the testMail.c for a usage example!

	Compiled and tested on:
	-----------------------
	Windows XP SP3 with MinGW
	Ubuntu Linux 9 with gcc


DEPENDENCIES:
-------------
	- openssl

LICENSE:
--------
  Copyright (C) 2010 Martin Albrecht <martin.albrecht@javacoffee.de>

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License along
  with this library; if not, write to the Free Software Foundation, Inc.,
  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

********************************************************************************************/

/* Internal includes */
#include "smtpMail.h"
#include "base64.h"


/*
	Constructor
*/
struct smtpMail *smtpMail_init(char *f, char *t, char *s, char *b) {
	struct smtpMail *mail=malloc (sizeof (struct smtpMail));
	char *infoFormat = "----------\nMail info:\n----------\nTo: %s\nFrom: %s\nSubject: %s\nBody size: %d\nBody: %s\n\n";
	int lenFormat;
	int lenData;

	/* Set sender address */
	mail->from = malloc(strlen(f) + 1);
	sprintf(mail->from, "%s", f);

	/* Set recipient address */
	mail->to = malloc(strlen(t) + 1);
	sprintf(mail->to, "%s", t);

	/* Set subject text */
	mail->subj = malloc(strlen(s) + 1);
	sprintf(mail->subj, "%s", s);

	/* Set body text */
	mail->body = malloc(strlen(b) + 1);
	sprintf(mail->body, "%s", b);

	/* Set info text */
	lenFormat = strlen(infoFormat);
	lenData = strlen(mail->to) + strlen(mail->from) + strlen(mail->subj) + strlen(mail->body);
	mail->info = malloc(lenFormat + lenData + 1);
	sprintf(mail->info, infoFormat, mail->to, mail->from, mail->subj, strlen(mail->body), mail->body);

	/* Set the variables to false,
	   because mail has not been sent, yet and no login happened */
	mail->sent = FALSE;
	mail->login = FALSE;

	mail->hostname = malloc(1024);
	(mail->hostname)[0] = '\0';

	if( gethostname(mail->hostname, 1024) <= 0 ) {
		strcat(mail->hostname,"localhost");
	}

	mail->srv_info = NULL;
	mail->last_resp = NULL;
	mail->last_cmd = NULL;

	mail->user=NULL;
	mail->pass=NULL;

	/* Return the filled mail struct */
	return mail;
}

/*
	Destructor
*/
void smtpMail_free(struct smtpMail *mail) {
	free(mail->from);
	free(mail->to);
	free(mail->subj);
	free(mail->body);
	free(mail->server);

	if (mail->user != NULL)
		free(mail->user);
	if (mail->pass != NULL)
		free(mail->pass);
	free(mail->info);
	free(mail->hostname);
	if (mail->srv_info != NULL)
		free(mail->srv_info);
	if (mail->last_resp != NULL)
		free(mail->last_resp);
	if (mail->last_cmd != NULL)
		free(mail->last_cmd);
	free(mail);
}

/*
	Check mail for completness or return error code
*/
unsigned char smtpMail_checkMail(struct smtpMail *mail) {
	/* No sender */
	if( strlen(mail->from) <= 0 )
		return SMTP_ERR_NOFROM;

	/* No recipient */
	else if( strlen(mail->to) <= 0 )
		return SMTP_ERR_NORECP;

	/* No subject */
	else if( strlen(mail->subj) <= 0 )
		return SMTP_ERR_NOSUBJ;

	/* No body */
	else if( strlen(mail->body) <= 0 )
		return SMTP_ERR_NOBODY;

	/* No server */
	else if( strlen(mail->server) <= 0 )
		return SMTP_ERR_NOSERV;

	/* Mail okay! */
	else
		return SMTP_ERR_NOERROR;
}



/*
	Communicate with the mail server
*/

int smtpMail_sendcmd(struct smtpMail *mail,SOCKET sock,const char *cmd) {
	char serv_ans[256];
	int  serv_ans_len;

	/* Store the cmd sent in last_cmd */
	if (cmd!=NULL)
	{
		if (mail->last_cmd!=NULL)
			free(mail->last_cmd);
		mail->last_cmd=(char *)malloc(strlen(cmd)+1);
		(mail->last_cmd)[0]='\0';
		strcat(mail->last_cmd,cmd);
	}

	/* If cmd is NULL it will only check for a response (used on initial connection) */
	#ifndef _WIN32
		if (cmd!=NULL)
			write(sock, cmd, strlen(cmd));
		serv_ans_len=read(sock, serv_ans, 255);
	#else
		if (cmd!=NULL)
			send(sock, cmd, strlen(cmd), 0);
		serv_ans_len=recv(sock, serv_ans, 255, 0);
	#endif

	/* Store the response in last_Resp */
	if (mail->last_resp!=NULL)
		free(mail->last_resp);
	mail->last_resp=(char *)malloc(serv_ans_len+1);
	(mail->last_resp)[serv_ans_len]='\0';
	strncpy(mail->last_resp,serv_ans,serv_ans_len);

	/* Parse out the response code and return it. */
	return atoi(mail->last_resp);
}

unsigned char smtpMail_comServ(struct smtpMail *mail, SOCKET sock) {
	char command[65536];
	char *tmpMail;
	int rc;
	int lenFormat, lenMail;
	char *formatMail = "From: <%s>\r\nTo: <%s>\r\nSubject: %s\r\n\r\n%s%s";

	#ifdef _WIN32
		char *mailBuf;
	#endif

	/* Create the mail in right format and put EOF at the end */
	lenFormat = strlen(mail->from) + strlen(mail->to) + strlen(mail->subj) + strlen(mail->body) + strlen(SMTP_CMD_EOF);
	lenMail = strlen(formatMail) + lenFormat;
	tmpMail = malloc(lenMail);

	/* Initial Connection */
	#ifdef _WIN32
		mailBuf = malloc(lenMail);
		sprintf(mailBuf, formatMail, mail->from, mail->to, mail->subj, mail->body, SMTP_CMD_EOF);
		memcpy(tmpMail, mailBuf, lenMail);
		free(mailBuf);
	#else
		sprintf(tmpMail, formatMail, mail->from, mail->to, mail->subj, mail->body, SMTP_CMD_EOF);
	#endif

	rc=smtpMail_sendcmd(mail,sock,NULL);
	if (rc != SMTP_SERV_WELCOME)
	{
		free(tmpMail);
		return rc;
	}

	/* Say hello with TLS*/
	sprintf(command, "%s %s\r\n", SMTP_CMD_TLSHELO, mail->hostname);
	rc=smtpMail_sendcmd(mail,sock,command);
	if (rc != SMTP_SERV_OK)
	{
		free(tmpMail);
		return rc;
	}

	if (mail->user != NULL && strlen(mail->user)>0 && mail->pass != NULL) {
	/* Say you wanna login with AUTH LOGIN
	   Check if server wants you to start TLS first */
		sprintf(command, "%s\r\n", SMTP_CMD_LOGIN);
		rc=smtpMail_sendcmd(mail,sock,command);

		if( rc == SMTP_SERV_STARTTLS )
		{
			sprintf(command, "%s\r\n", SMTP_CMD_STARTTLS);
			if (smtpMail_sendcmd(mail,sock,command) != SMTP_SERV_AUTH)
				return SMTP_ERR_CMDFAILED;
		}

		if( rc != SMTP_SERV_AUTH )
			return SMTP_ERR_CMDFAILED;


		/* Tell the username */
		sprintf(command, "%s\r\n", mail->user);
		if (smtpMail_sendcmd(mail,sock,command) != SMTP_SERV_AUTH)
			return SMTP_ERR_LOGINFAIL;

		/* Tell the password */
		sprintf(command, "%s\r\n", mail->pass);

		rc=smtpMail_sendcmd(mail,sock,command);
		if( rc != SMTP_SERV_AUTHOK)
			return SMTP_ERR_LOGINFAIL;

		/* Now we are logged in, so we can set check variable to true */
		mail->login = TRUE;
	}

	/* Say sender address */
	sprintf(command, "%s %s\r\n", SMTP_CMD_FROM, mail->from);
	rc=smtpMail_sendcmd(mail,sock,command);

	if( rc == SMTP_SERV_NOMBOXNM)
	{
		free(tmpMail);
		return SMTP_ERR_MBOXNFND;
	}
	if( rc != SMTP_SERV_OK )
	{
		free(tmpMail);
		return SMTP_ERR_CMDFAILED;
	}

	/* Say recipient address */
	sprintf(command, "%s %s\r\n", SMTP_CMD_TO, mail->to);
	if (smtpMail_sendcmd(mail,sock,command) != SMTP_SERV_OK)
	{
		free(tmpMail);
		return SMTP_ERR_CMDFAILED;
	}

	/* Say you wanna write data */
	sprintf(command, "%s\r\n", SMTP_CMD_BODY);
	if (smtpMail_sendcmd(mail,sock,command) != SMTP_SERV_GOAHEAD)
	{
		free(tmpMail);
		return SMTP_ERR_CMDFAILED;
	}

	/* Send body with EOF */
	sprintf(command, "%s", tmpMail);
	free(tmpMail);
	smtpMail_sendcmd(mail,sock,command);

	/* Close and go... */
	sprintf(command, "%s\r\n", SMTP_CMD_QUIT);
	smtpMail_sendcmd(mail,sock,command);

	/* Close the socket */
	#ifdef _WIN32
		closesocket(sock);
	#else
		close(sock);
	#endif

	return SMTP_ERR_NOERROR;
}


/*
	Send a mail
*/
unsigned char smtpMail_send(struct smtpMail *mail, char *serv, int port, char *usr, char *pwd) {
	char *tmpU, *tmpP;
	unsigned char res;
	int lenFormat, lenData;

	#ifndef _WIN32
		int sock;
		struct sockaddr_in serv_addr;
		struct hostent *srv_hostent;
	#else
		unsigned long tmpIP;
		SOCKET sock;
		SOCKADDR_IN serv_addr;
		HOSTENT *srv_hostent;
	#endif

	/* Check if mail already sent... */
	if( mail->sent == TRUE )
		return SMTP_ERR_ALLRDSENT;

	/* Set internal variables for login */
	mail->server = malloc(strlen(serv)+1);
	sprintf(mail->server, "%s", serv);

	if(port <= 0)
		mail->port = SMTP_PORT;
	else
		mail->port = port;

	if (usr!=NULL)
	{
		/* Setup User ID */
		tmpU = b64_encode(usr, strlen(usr));
		mail->user = malloc(strlen(tmpU)+1);
		mail->user = tmpU;

		/* Setup PWD */
		tmpP = b64_encode(pwd, strlen(pwd));

		mail->pass = malloc(strlen(tmpP)+1);
		mail->pass = tmpP;
	}

	/* Check if mail is complete */
	if( smtpMail_checkMail(mail) != SMTP_ERR_NOERROR )
		return smtpMail_checkMail(mail);

	/* Set the socket and connect */
	#ifdef _WIN32
		startWinsock();

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock == INVALID_SOCKET)
			return SMTP_ERR_NOSOCK;

		memset(&serv_addr, 0, sizeof(SOCKADDR_IN));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(SMTP_PORT);

		/* Check if IP or hostname is given a
		   and set server field in struct*/
		tmpIP = inet_addr(mail->server);
		if( tmpIP != INADDR_NONE )
			serv_addr.sin_addr.s_addr=inet_addr(mail->server);
		else
		{
			srv_hostent = gethostbyname(mail->server);
			if( srv_hostent == NULL )
				return SMTP_ERR_SERVNFND;

			memcpy(&(serv_addr.sin_addr), srv_hostent->h_addr_list[0], 4);
		}

		if( connect(sock, (SOCKADDR*) &serv_addr, sizeof(SOCKADDR)) == SOCKET_ERROR )
			return SMTP_ERR_NOCON;
	#else
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if(sock < 0)
			return SMTP_ERR_NOSOCK;

		srv_hostent = gethostbyname(mail->server);
		if( !srv_hostent )
			return SMTP_ERR_SERVNFND;

		bzero((char*) &serv_addr, sizeof(serv_addr));
		bcopy((char *)srv_hostent->h_addr, (char *)&serv_addr.sin_addr.s_addr, srv_hostent->h_length);

		serv_addr.sin_family = AF_INET;
		serv_addr.sin_port = htons(SMTP_PORT);

		if( connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0 )
			return SMTP_ERR_NOCON;
	#endif

	/* Fill our server info field with the data */

	if (mail->user==NULL || mail->pass==NULL)
	{
		char *formatSrvInfo = "----------\nServer info:\n----------\nHost: %s\nUser: NULL\nPass: NULL\n\n";
		lenFormat = strlen(formatSrvInfo);
		lenData = strlen(mail->server);
		mail->srv_info = malloc(lenFormat+lenData);
		sprintf(mail->srv_info, formatSrvInfo, mail->server);
	}
	else
	{
		char *formatSrvInfo = "----------\nServer info:\n----------\nHost: %s\nUser: %s\nPass: %s\n\n";
		lenFormat = strlen(formatSrvInfo);
		lenData = strlen(mail->server) + (mail->user==NULL ? 0 : strlen(mail->user)) + (mail->pass==NULL ? 0 : strlen(mail->pass));
		mail->srv_info = malloc(lenFormat+lenData);
		sprintf(mail->srv_info, formatSrvInfo, mail->server, mail->user, mail->pass);
	}

	/* Communicate with the server */
	res = smtpMail_comServ(mail, sock);

	return res;
}


/*
	Return a message for the given internal error code
*/
char *smtpMail_error(unsigned char code) {
	char *message = malloc(256);

	switch(code) {
		case SMTP_ERR_NOFROM:
			sprintf(message, "Error (0x%.2x): No FROM address given!\n", code);
			break;

		case SMTP_ERR_NORECP:
			sprintf(message, "Error (0x%.2x): No TO address given!\n", code);
			break;

		case SMTP_ERR_NOSUBJ:
			sprintf(message, "Error (0x%.2x): No subject given!\n", code);
			break;

		case SMTP_ERR_NOBODY:
			sprintf(message, "Error (0x%.2x): No body text given!\n", code);
			break;

		case SMTP_ERR_ALLRDSENT:
			sprintf(message, "Error (0x%.2x): Mail has already been sent!\n", code);
			break;

		case SMTP_ERR_NOSERV:
			sprintf(message, "Error (0x%.2x): No server address given!\n", code);
			break;

		case SMTP_ERR_NOUSER:
			sprintf(message, "Error (0x%.2x): No username given!\n", code);
			break;

		case SMTP_ERR_NOPASS:
			sprintf(message, "Error (0x%.2x): No password given!\n", code);
			break;

		case SMTP_ERR_SERVNFND:
			sprintf(message, "Error (0x%.2x): Server not found!\n", code);
			break;

		case SMTP_ERR_NOCON:
			sprintf(message, "Error (0x%.2x): No connection to server possible!\n", code);
			break;

		case SMTP_ERR_NOSOCK:
			sprintf(message, "Error (0x%.2x): No socket!\n", code);
			break;

		case SMTP_ERR_NOERROR:
			return "Success! No error reported!";
			break;

		case SMTP_ERR_CMDFAILED:
			sprintf(message, "Error (0x%.2x): Server command failed!\n", code);
			break;

		case SMTP_ERR_LOGINFAIL:
			sprintf(message, "Error (0x%.2x): Server login failed!\n", code);
			break;

		case SMTP_ERR_MBOXNFND:
			sprintf(message, "Error (0x%.2x): Mailbox with this name not found!\n", code);
			break;

		default:
			sprintf(message, "Error (0x%.2x): Unknown error!\n", code);
			break;
	}

	return message;
}
