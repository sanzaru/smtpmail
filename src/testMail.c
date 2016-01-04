#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smtpMail.h"

int main(int argc, char **argv) {
	char *from = "noanswer@mail.com";
	char *to[2];
	char *sub = "TestMail";
	char *filename;
	char *body = "This is a fine test body...";

	char *server = "YOUR_MAILSERVER"; /* Your SMTP server */
	char *user = "YOUR_USERNAME"; /* Your username */
	char *pass = "YOUR_PASSWORD"; /* Your password */

	unsigned char res;
	struct smtpMail *mail;
	FILE *fd;
	long lSize;
	int i;

	/* Check arguments for filename */
	if( argc == 2 ) {
		filename = argv[1];
		fd = fopen(filename, "r");
		fseek (fd , 0 , SEEK_END);
		lSize = ftell(fd)-1;
		rewind (fd);

		body = malloc(lSize+1);
		#ifdef _WIN32
			memset(body, 0, sizeof(char*));
		#endif

		fread(body, sizeof(char), lSize, fd);
		fclose(fd);
		body[lSize] = 0;
		printf("Read %ld bytes...\n", lSize);
	}

	to[0] = "dummy@mail.com";
	to[1] = "dummy2@mail.com";

	for( i=0; i<2; i++ ) {
		/* New mail->.. */
		mail = smtpMail_init(from, to[i], sub, body);

		printf("%s", mail->info );

		/* Send mail->.. */
		res = smtpMail_send(mail, server, 25, user, pass);

		if( res == SMTP_ERR_NOERROR ) {
			printf("Mail successfully sent to: %s!\n", mail->to);
		} else {
			char *message=smtpMail_error(res);
			printf("%s", message);
			free(message);
		}
		printf("Finished mail send from test\n");
		printf("Info\n");

		printf("%s\n", mail->srv_info );

		printf("Finished Info\n");
		printf("To: %s\n",mail->to);
		printf("Return Code: %2x\n",res);

		printf("Last Command:\n%s\n",mail->last_cmd);
		printf("Last Response:\n%s\n",mail->last_resp);

		smtpMail_free(mail);
	}

	return 0;
}

