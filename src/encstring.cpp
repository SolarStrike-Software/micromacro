#include "encstring.h"

// Copies the unencrypted string from 'orig' to our output buffer, without exceeding maxlen.
size_t EncString::reveal(char *outBuffer, size_t maxlen, const int *orig)
{
	// Find length
	size_t len = 0;
	for(size_t i = 0; ; i++)
	{
		if( orig[i] == 0 ) {
			len = i;
			break; }
	}

	// Ensure it won't overflow the buffer
	if( len > maxlen - 2)
		len = maxlen - 2;

	for(size_t i = 0; i < len; i++)
		outBuffer[i] = orig[i] ^ EncString::key;

	outBuffer[len] = '\0';

	return len;
}
