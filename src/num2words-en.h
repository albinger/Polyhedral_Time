#pragma once
#include "string.h"

int time_to_words(int hours, int minutes, char* words, size_t length);
int time_to_3words(int hours, int minutes, char *line1, char *line2, char *line3, size_t length);
