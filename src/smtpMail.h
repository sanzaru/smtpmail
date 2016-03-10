/********************************************************************************************
	Header for the smtpMail C library

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
	- openssl development libraries

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
#ifndef __SMTPMAIL_H__
#define __SMTPMAIL_H__

/* General includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  /* Socket includes (LINUX / WIN) */
#ifndef _WIN32
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
  	#include <unistd.h>
#else
	#include <windows.h>
	#include <winsock2.h>

	int startWinsock() {
		WSADATA wsa;
		return WSAStartup(MAKEWORD(2,0),&wsa);
	}

#endif

/* Some globals */
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef TRUE
	#define TRUE  1
#endif
#define SMTP_PORT 25

/* SMTP server commands */
#define STMP_CMD_ATRN		"ATRN" /* Authenticated TURN. RFC 2645 */
#define SMTP_CMD_AUTH 		"AUTH" /* Authentication. RFC 2554 */
#define SMTP_CMD_BDAT		"BDAT" /* Binary data. RFC 3030 */
#define SMTP_CMD_ETRN		"ETRN" /* RFC 1985 */
#define SMTP_CMD_EXPN		"EXPN" /* Expand RFC 2821 */
#define SMTP_CMD_HELP		"HELP" /* Help RFC 2821 */
#define SMTP_CMD_MAIL		"MAIL" /* Mail RFC 2821 */
#define SMTP_CMD_NOOP		"NOOP" /* No operation RFC 2821 */
#define SMTP_CMD_RCPT		"RCPT" /* Recipient RFC 2821 */
#define SMTP_CMD_RSET		"RSET" /* Reset RFC 2821 */
#define SMTP_CMD_SAML		"SAML" /* Send and mail RFC 821 */
#define SMTP_CMD_SEND		"SEND" /* Send RFC 821 */
#define SMTP_CMD_SOML		"SOML" /* Send or mail RFC 821 */
#define SMTP_CMD_TURN		"TURN" /* Turn RFC 821 */
#define SMTP_CMD_VRFY		"VRFY" /* Verify RFC 2821 */
#define SMTP_CMD_HELO		"HELO"
#define SMTP_CMD_TLSHELO	"EHLO"
#define SMTP_CMD_LOGIN		"AUTH LOGIN"
#define SMTP_CMD_STARTTLS	"STARTTLS"
#define SMTP_CMD_FROM		"MAIL FROM:"
#define SMTP_CMD_TO			"RCPT TO:"
#define SMTP_CMD_BODY		"DATA"
#define SMTP_CMD_EOF		"\r\n.\r\n"
#define SMTP_CMD_QUIT		"QUIT"

/* SMTP server codes */
#define SMTP_SERV_NOSTDOKAY	200 /* (nonstandard success response, see rfc876) */
#define SMTP_SERV_SYSSTAT	211 /* System status, or system help reply */
#define SMTP_SERV_HELP		214 /* Help message */
#define SMTP_SERV_WELCOME	220 /* Server Welcome */
#define SMTP_SERV_QUIT		221 /* Server Quit */
#define SMTP_SERV_AUTHOK	235 /* Server authentication okay */
#define SMTP_SERV_OK		250 /* Server common "okay" */
#define SMTP_SERV_USRFRWD	251 /* User not local; will forward to <forward-path> */

#define SMTP_SERV_AUTH		334 /* Server authentication needed */
#define SMTP_SERV_GOAHEAD	354 /* Server wait for body text */

#define SMTP_SERV_NOSERVICE 421 /* Server error: <domain> Service not available, closing transmission channel */
#define SMTP_SERV_NOMAILBOX 450 /* Server error: Requested mail action not taken: mailbox unavailable */
#define SMTP_SERV_ACTABORT	451 /* Server error: Requested action aborted: local error in processing */
#define SMTP_SERV_NOSTORAGE 452 /* Server error: Requested action not taken: insufficient system storage */
#define SMTP_SERV_NOTLS		454 /* Server error: TLS not available */
#define SMTP_SERV_SYNTAX	500 /* Syntax error, command unrecognised */
#define SMTP_SERV_SYNTAX_P	501 /* Server error: Syntax error in parameters or arguments*/
#define SMTP_SERV_UNKNOWN	502 /* Server error: Unknown command */
#define SMTP_SERV_BADSEQ	503 /* Server error: Bad sequence of commands */
#define SMTP_SERV_NCMDPAR	504 /* Server error: Command parameter not implemented */
#define SMTP_SERV_NOACCMAIL	521 /* Server error: <domain> does not accept mail (see rfc1846) */
#define SMTP_SERV_STARTTLS  530 /* Server error: Start TLS needed */
#define SMTP_SERV_AUTHERR	535 /* Server error: Authentication failed */
#define SMTP_SERV_NOMBOXNM	550 /* Server error: Requested action not taken: mailbox unavailable */
#define SMTP_SERV_USRNOLOCL 551 /* Server error: User not local; please try <forward-path> */
#define SMTP_SERV_EXCSTOR	552 /* Server error: Requested mail action aborted: exceeded storage allocation */
#define SMTP_SERV_MBXNMNALL 553 /* Server error: Requested action not taken: mailbox name not allowed */
#define SMTP_SERV_TMUNKCOM  554 /* Server error: Transaction failed*/

/* Internal error codes */
#define SMTP_ERR_NOFROM		0x00
#define SMTP_ERR_NORECP		0x01
#define SMTP_ERR_NOSUBJ		0x02
#define SMTP_ERR_NOBODY		0x03
#define SMTP_ERR_ALLRDSENT	0x04
#define SMTP_ERR_NOSERV		0x05
#define SMTP_ERR_NOUSER		0x06
#define SMTP_ERR_NOPASS		0x07
#define SMTP_ERR_SERVNFND	0x08
#define SMTP_ERR_NOCON		0x09
#define SMTP_ERR_NOSOCK		0x0A
#define SMTP_ERR_CMDFAILED  0x0B
#define SMTP_ERR_LOGINFAIL	0x0C
#define SMTP_ERR_MBOXNFND	0x0D

#define SMTP_ERR_NOERROR	0xFF

/* Typedef for the structure */
typedef struct smtpMail SMTPMAIL;

/* Typedefs for unix systems */
#ifndef _WIN32
	typedef int SOCKET;
	typedef struct sockaddr_in SOCKADDR_IN;
	typedef struct sockaddr SOCKADDR;
#endif

/*
	Mail structure
*/
struct smtpMail
{
	char *from;		/* The sender address */
	char *to;   	/* The recipient address */
	char *subj;		/* The mail subject */
	char *body;		/* The mail body */
	char *fMail;	/* Formatted mail with EOF */

	unsigned char sent;		/* Variable to check if mail already sent */
	unsigned char login;		/* Variable to check if logged in */

	char *server;	/* The SMTP server */
	char *hostname; /* The client's Host Name */
	int  port;		/* The SMTP server port */
	char *user;		/* The username for login */
	char *pass;		/* The password for login */

	char *info;		/* General info about the mail */
	char *srv_info;	/* General server info */

	char *last_cmd; /* Last Command Sent */
	char *last_resp; /* Last Response Received */
};

/* Contructor prototype */
struct smtpMail *smtpMail_init(const char *f, const char *t, const char *s, const char *b);

/* Destructor prototype */
void smtpMail_free(struct smtpMail *mail);

/* Send mail function prototype */
unsigned char smtpMail_send(struct smtpMail *mail, const char *serv, int port, const char *usr, const char *pwd);

/* Parse error code and return message */
const char * smtpMail_error(unsigned char code);

#endif
