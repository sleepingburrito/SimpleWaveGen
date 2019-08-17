//2019 sleeping burrito MIT
//sampleRate 8372

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

double * samples = NULL; 
int32_t sampleLength = 0;
int32_t timeLengthMs = 0;

char * fileBuffer = NULL;
size_t fileSize = 0;

double sampleRate = 0;
double sampleRateOverOne = 0;
const double piTwo = 3.14159265359 * 2.0;

typedef struct{
	int32_t startMs;
	uint8_t note;
	uint8_t number;
	uint8_t sfn; //sharp square non
	int32_t lengthMs;
	uint8_t sq; //sine or square
}note;



double NoteToFrequency(const int32_t note) {
	if (note < 1 || note > 88){
		fprintf(stderr, "bad note: %i \n", note);
		assert(0);
	}
	return pow(pow(2.0, 1.0 / 12.0), (double)note - 49.0) * 440.0;
}

int32_t DecodeNotation(uint8_t note, const uint8_t number, uint8_t sharpFlatNon) {

	//decode note
	int32_t returnNote = 0;
	note = (uint8_t)toupper(note);
	switch(note){
		case 'A':
			returnNote = 1;
			break;
		case 'B':
			returnNote = 3;
			break;
		case 'C':
			returnNote = 4;
			break;
		case 'D':
			returnNote = 6;
			break;
		case 'E':
			returnNote = 8;
			break;
		case 'F':
			returnNote = 9;
			break;
		case 'G':
			returnNote = 11;
			break;
		default:
			fprintf(stderr, "bad note: %c \n", note);
			assert(0);
	}

	//sharp flat
	sharpFlatNon = (uint8_t)toupper(sharpFlatNon);
	if (sharpFlatNon == 'S')
		++returnNote;
	else if (sharpFlatNon == 'F')
		--returnNote;
	else if (sharpFlatNon != 'N'){
		fprintf(stderr, "bad (S)hap/(F)lat/(N)on: %c \n", sharpFlatNon);
		assert(0);
	}

	//number
	if (isalnum(number) == false) {
		fprintf(stderr, "number was not a number: %c \n", number);
		assert(0);
	}

	uint8_t numberString[2];
	numberString[0] = number;
	numberString[1] = 0;
	int32_t numberOut = (int32_t)atoi((char *)(&numberString));
	
	if (numberOut < 0 ||  numberOut > 8){
		fprintf(stderr, "bad number: %i \n", numberOut);
		assert(0);
	}
	returnNote += 12 * ((--numberOut < 0) ? 0 : numberOut);

	//fix error where A/B notes are off by 12
	if (number != '0' && (note == 'A' || note == 'B'))
		returnNote += 12;

	//errors
	if (returnNote < 1 || returnNote > 88){
		fprintf(stderr, "bad note: %c %c %c \n", note, number, sharpFlatNon);
		assert(0);
	}

	return returnNote;
}

void InitSampleRateAndOverOne(void) {
	sampleRate = NoteToFrequency(DecodeNotation('C', '8', 'N')) * 2.0;
	if (sampleRate <= 0){
		fprintf(stderr, "bad sample rate %f \n", sampleRate);
		assert(0);
	}
	sampleRateOverOne = 1.0 / sampleRate;
}

double SamplePerMs(const int32_t ms){
	if (ms < 0){
		fprintf(stderr, "bad time ms: %i \n", ms);
		assert(0);
	}
	return sampleRate / 1000.0 * (double)ms;
}

void WriteSample(const double sample, const int32_t position) {
	if (position < 0) {
		fprintf(stderr, "negative position \n");
		assert(0);
	}
	if (position >= sampleLength) {
		fprintf(stderr, "position past length \n");
		assert(0);
	}
	if (samples == NULL){
		fprintf(stderr, "samples where null \n");
		assert(0);
	}
	samples[position] = sample;
}

void InitSamples(const int32_t lengthMs){
	timeLengthMs = lengthMs;
	if (timeLengthMs < 0){
		fprintf(stderr, "negative length \n");
		assert(0);
	}

	sampleLength = (int32_t)SamplePerMs(lengthMs);
	if (sampleLength < 0){
		fprintf(stderr, "length too long \n");
		assert(0);
	}

	samples = calloc(sampleLength, sizeof(double));
	if (samples == NULL) {
		fprintf(stderr, "calloc failed \n");
		assert(0);
	}
}

void MakeSineWave(const int32_t startTimeMs, const int32_t lengthMs, const double frequency, uint8_t sq) {
	if (startTimeMs < 0 || lengthMs < 0) {
		fprintf(stderr, "bad 0 or negative time ms: start time %i length time %i \n", startTimeMs, lengthMs);
		assert(0);
	}
	if (frequency <= 0) {
		fprintf(stderr, "bad negative frequency: start time %f \n", frequency);
		assert(0);
	}
	
	sq = (uint8_t)toupper(sq);
	const int32_t startSample = (int32_t)SamplePerMs(startTimeMs);
	const int32_t sampleLength = startSample + (int32_t)SamplePerMs(lengthMs);
	
	if (sampleLength < startSample){
		fprintf(stderr, "sample length overflow \n");
		assert(0);
	}

	const double spinPerSample = piTwo / (1.0 / frequency / sampleRateOverOne);
	double accumulator = 0;

	for (int32_t i = startSample; i < sampleLength; ++i){ 
		if (sq == 'S'){
			WriteSample( sin( accumulator += spinPerSample ), i );
		}else if (sq == 'Q'){
			WriteSample( (sin( accumulator += spinPerSample) > 0.0 ? 1.0 : -1.0), i );
		}else{
			fprintf(stderr, "bad sq: %c \n", sq);
			assert(0);
		}
	}

}

void SaveSamplesToFile(const char * const filename){
	
	FILE * file = fopen(filename, "wb");
	if (file == NULL){
		fprintf(stderr, "failed to open write file: %s \n", filename);
		assert(0);
	}
	
	if (fwrite(samples, sizeof(double), sampleLength, file) != sampleLength){
		fprintf(stderr, "failed to save all data to file: %s \n", filename);
		assert(0);
	}

	fclose(file);
}

void LoadFileBuffer(const char * const filename){
	FILE * file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "failed to open read file: %s \n", filename);
		assert(0);
	}

	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	fileBuffer = calloc(fileSize + 1, sizeof(char)); //+1 for null term
	if (fileBuffer == NULL) {
		fprintf(stderr, "failed to calloc for file read \n");
		assert(0);
	}

	if (fread(fileBuffer, sizeof(char), fileSize, file) != fileSize) {
		fprintf(stderr, "failed to read all of file data \n");
		assert(0);
	}

	fileBuffer[fileSize] = 0; //null term
	fclose(file);
}

note ParseNoteFromString(const char* const str) {
	note tmpNote;
	if (sscanf(str, "%i%c%c%c%i%c", 
				&tmpNote.startMs, 
				&tmpNote.note, 
				&tmpNote.number, 
				&tmpNote.sfn, 
				&tmpNote.lengthMs,
				&tmpNote.sq) != 6) {
		fprintf(stderr, "bad formatting in note: %s \n", str);
		assert(0);
	}
	return tmpNote;
}


int main(int argc, char** argv) {

	//init
	if (argc != 3) {
		fprintf(stderr, "wrong number of arguments, should be 2 (outfile, infile), input was: %i \n", argc);
		assert(0);
	}
	InitSampleRateAndOverOne();

	//load file into mem
	LoadFileBuffer(argv[2]);

	//divide into strings
	for (size_t i = 0; i < fileSize; ++i) {
		if (isalnum(fileBuffer[i]))
			continue;
		fileBuffer[i] = 0;
	}

	//get total duration
	bool inNote = false;
	for (size_t i = 0; i < fileSize; ++i) {
		if (isalnum(fileBuffer[i])) {
			//ran at start of note
			if (!inNote){
				note tmpNote = ParseNoteFromString(&fileBuffer[i]);
				if (tmpNote.startMs + tmpNote.lengthMs > timeLengthMs)
					timeLengthMs = tmpNote.startMs + tmpNote.lengthMs;

				inNote = true;
			}
			continue;
		}
		inNote = false;
	}

	//allocate mem for generating samples
	InitSamples(timeLengthMs);

	//decode notes make samples
	inNote = false;
	for (size_t i = 0; i < fileSize; ++i) {
		if (isalnum(fileBuffer[i])) {
			//ran at start of note
			if (!inNote) {
				note tmpNote = ParseNoteFromString(&fileBuffer[i]);
				MakeSineWave(tmpNote.startMs, 
						tmpNote.lengthMs, 
						NoteToFrequency(DecodeNotation(
								tmpNote.note, 
								tmpNote.number, 
								tmpNote.sfn)),
						tmpNote.sq);
				inNote = true;
			}
			continue;
		}
		inNote = false;
	}

	//saving
	SaveSamplesToFile(argv[1]);

	//free file buffer and sample data
	free(fileBuffer);
	free(samples);
	return EXIT_SUCCESS; 
}
