#ifndef STATIC_ITOA_H
#define STATIC_ITOA_H

  static char *static_itoa(unsigned pos) {
    static char buf[] = "01234567890123456789"; // to put end of string character at buf[20]
	static const unsigned bufsize=sizeof(buf);
	//Assert(bufsize==21);    
    // decimal string for int
    char *num=buf+bufsize-1; // place at the '\0'
    if ( !pos ) {      
	  *--num='0';
    } else {
      char rem;
	  // 3digit lookup table, divide by 1000 faster?
      while ( pos ) {
		rem = pos;
		pos /= 10;
		rem -= 10*pos;		// maybe this is faster than mod because we are already dividing
		*--num = '0' + (char)rem;
      }	
    }
    return num;
  }


#endif
