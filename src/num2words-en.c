#include "num2words-en.h"
#include "string.h"
#include "pebble.h"


  
  
int time_to_words(int hours, int minutes, char* words, size_t length) {

  
  memset(words, 0, length);
  char hrs[10];
  char mns[10];
  
  int hrand;
  int mrand;
  
  static const int const DICE[] = {
    4,
    6,
    8,
    10,  
    12,
    20,
    12,
    10,
    8,
    6,
    4
};
  
  //srand(time(NULL));
  
  //d4, d6, d8, d10, d12
  
  
  hrand = DICE[rand()%5];
  mrand = DICE[rand()%11];
  
 
  
  if(hours/hrand==0){
    snprintf(hrs,9,"1d%i-%i",hrand,hrand-hours);
  }else{
    if(hours%hrand != 0){
      snprintf(hrs,9,"%id%i+%i",(hours/hrand),hrand,(hours%hrand));
    }else{  
      snprintf(hrs,9,"%id%i",(hours/hrand),hrand);
    }
  }
  if(minutes/mrand==0){
    snprintf(mns,9,"1d%i-%i",mrand,mrand-minutes);
  }else{ 
    if(minutes%mrand != 0){
      snprintf(mns,9,"%id%i+%i",(minutes/mrand),mrand,(minutes%mrand));
    }else{
      snprintf(mns,9,"%id%i", (minutes/mrand),mrand);
    }
  }
  snprintf(words,length,"%s %s 0",hrs,mns);  
  APP_LOG(APP_LOG_LEVEL_INFO,"TTW:%s:",words);
   if(mrand == 20){return(0); }else{return(1);}
}

int time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length)
{
	char value[length];
  int ShowDie;
	ShowDie = time_to_words(hours, minutes, value, length);
	
	memset(line1, 0, length);
	memset(line2, 0, length);
	memset(line3, 0, length);
	
	char *start = value;
	char *pch = strstr (start, " ");
	while (pch != NULL) {
		if (line1[0] == 0) {
			memcpy(line1, start, pch-start);
		}  else if (line2[0] == 0) {
			memcpy(line2, start, pch-start);
		} else if (line3[0] == 0) {
			memcpy(line3, start, pch-start);
		}
		start += pch-start+1;
		pch = strstr(start, " ");
	}
	
	// Truncate long teen values
	if (strlen(line2) > 7) {
		char *pch = strstr(line2, "teen");
		if (pch) {
			memcpy(line3, pch, 4);
			pch[0] = 0;
		}
	}
  return(ShowDie);
}
