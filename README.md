The smtpmail library provides a simple interface for sending E-Mails over a SMTP server from a C or C++ language program.
I can be included in the source code directly or just dynamically linked as a DLL or .so file.
A common use would be in mailing software (e.g. Newsletter programs or E-mail clients).

### Dependencies

* A working C/C++ compiler and linker (of course)
* openssl development libraries

### Features

smtpmail is intended to be simple and so there is no complex framework or API. 
The only and most important features are:

* Send mail to one or more recipients
* Support of TLS servers
* Unfortunately it is much work to implement all the wishes I have and so there are also plans for the future:

**Support for gMail:** For now it is not possible to log in with your Googlemail account credentials. Though TLS is supported, Google seems to want more at authentication.

**To send HTML mails:** Unfortunately smtpmail cannot send HTML formatted mails. It will handle them, but in every test no HTML mail ever reached the recipient.

### To do

* SSL support
* Better TLS support


### Installation

Compile the library and test program:

```
make
```

Compile shared library:

```
make shared
```

### Examples

Include the library

```c
#include "smtpMail.h"
```

Inizialize a simple mail object

```c
struct smtpMail *mail;
char *from = "noanswer@mail.com";
char *to = "recp@mail.com";
char *sub = "Subject";
char *body = "The mail body";

mail = smtpMail_init(from, to, sub, body);
```

Send the mail object

```c
unsigned char res;
char *server = "YOUR_SMTP_SERVER"; /* Your SMTP server */
char *user = "YOUR_USERNAME"; /* Your username */
char *pass = "YOUR_PASSWORD"; /* Your password */

res = smtpMail_send(mail, server, 25, user, pass);

if( res == SMTP_ERR_NOERROR ) {
	printf("Mail successfully sent to: %s!\n", mail->to);
} else {
	fprintf(stderr, "There was an error sending the mail: %d\n", res);
	system(1);
}
```

See the testMail.c file for a full example.
