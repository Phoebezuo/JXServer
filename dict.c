#include "dict.h"

// import the binary file to a dictionary
int import_dictionary(struct dict** dictionary) {
	struct dict* local_dict = *dictionary; 

	// open dictionary binary file 
	char* filename = "compression.dict";
    FILE* fptr = fopen(filename, "rb"); 
	if (fptr == NULL) { return -1; }

	// get the size of bytes in the dictionary using struct stat 
	struct stat info;
	if (stat(filename, &info) != 0) { return -1; }

	// read through the content and store it in dictionary_content
	uint8_t* content = malloc(info.st_size);
	if (fread(content, info.st_size, 1, fptr) != 1) { return -1; }
	fclose(fptr);

	// initialize bit arrays to store values and length 
	uint8_t* value_array = malloc(info.st_size * sizeof(uint8_t));
	int* index_array = malloc((DICT_SIZE+1) * sizeof(int)); 
	bitarray_init(content, (int)info.st_size, &value_array, &index_array);
	free(content);

	// initialize dictionary to store dictionary
	dict_init(&local_dict, &value_array, &index_array); 
	free(value_array);
	free(index_array);

	return 0; 
} 

// set the individual bit to a certain value, either 0 or 1 
void set_bit(uint8_t* buf, int index, int value) {
	// get index within the array
    int array_index = index / BYTE_SIZE;

	// shift to bit position
    uint8_t mask = 1 << OFFSET(index);

	// set bit position to value
    if (value) {
        buf[array_index] |= mask;
	} else {
        buf[array_index] &= ~mask;
	}
}

// get the value of individual bit 
uint8_t get_bit(uint8_t* array, int index) {
	// get index within the array
	int array_index = index / BYTE_SIZE; 

	// shitf to bit position
	unsigned int mask = array[array_index] >> OFFSET(index);

	// get the last bit 
	uint8_t result = mask & 1;  
	return result;
}

// convert a bit array to two bit array in order to build dictionary 
void bitarray_init(uint8_t* content, int file_size, uint8_t** value_array, 
	int** index_array) {

	// initialize cursor and count
	int value_array_size = file_size * BYTE_SIZE; 
	int dict_count = 0;
	int dict_cursor  = 0; 
	int array_cursor = 0; 

	// loop through the byte array of dictionary content 
	while (array_cursor < value_array_size) { 
		uint8_t len = 0x0; 
		(*index_array)[dict_count] = dict_cursor;

		// get the value len in 1 byte 
		for (int j = 0; j < BYTE_SIZE; j++) {
			set_bit(&len, j, get_bit(content, array_cursor++)); 
		}
		
		// get the value with corresponding len indicate in previous 8 bits
		for (uint8_t j = 0; j < len; j++) {
			set_bit(*value_array, dict_cursor++, 
				get_bit(content, array_cursor++));
		}
		dict_count++;

		// store the last element in the index array 
		if (dict_count == DICT_SIZE) { 
			(*index_array)[dict_count] = dict_cursor;
			break; 
		}
	}
}

// initialize a dictionary with two bit array 
void dict_init(struct dict** dictionary, uint8_t** value_array, 
	int** index_array) {
	
	struct dict* d = *dictionary;
	uint8_t* v_arr = *value_array;
	int* i_arr = *index_array;

	// loop through the whole dictionary
	for (int i = 0; i < DICT_SIZE; i++) {
		// key is increment with the dictionary index 
		d[i].key = i;

		// value len is difference of two adjacent index array element 
		d[i].value_len = i_arr[i+1] - i_arr[i];

		// set all the value in dictionary to 0 
		memset(d[i].value, 0, sizeof(VALUE_MAX_SIZE));
		int dict_count = 0; 
		
		// loop through the value len of each key, store each bit in dictionay 
		for (int j = i_arr[i]; j < i_arr[i+1]; j++) {
			set_bit(d[i].value, dict_count++, get_bit(v_arr, j));
		}
	}
}

void destroy_dict(struct dict* dictionary) {
	free(dictionary);
}

// compression the uint8_t array and return compressed message 
// as well as change the message length after compressed 
uint8_t* encode(struct dict* dictionary, uint8_t* origin_hex, 
	int origin_len, int* result_len) {

	// initialize a buffer to store results
	uint8_t* buffer = malloc(sizeof(uint8_t) * 1000); 

	// loop though the origin_hex
	int result_cursor = 0;
	for (int i = 0; i < origin_len; i++) {
		for (int j = 0; j < DICT_SIZE; j++) {

			// find corresponding key in dictionary
			if (origin_hex[i] == dictionary[j].key) {
				struct dict d = dictionary[j]; 

				// put the value in the buffer
				for (int k = 0; k < d.value_len; k++) {
					set_bit(buffer, result_cursor++, get_bit(d.value, k));
				}
			}
		}
	}

	// calculate the size of padding after compression
	int padding_len;
	if ((result_cursor % BYTE_SIZE) == 0) {
		padding_len = 0; 
	} else {
		padding_len = BYTE_SIZE - (result_cursor % BYTE_SIZE); 
	}

	// calculate the final result length 
	*result_len = (result_cursor + padding_len) / BYTE_SIZE + 1;
	int padding_cursor = result_cursor; 

	// set the padding 0 to the buffer 
	for (int i = 0; i < padding_len; i++) {
		set_bit(buffer, padding_cursor++, 0);
	}

	// put number of padding into the buffer
	buffer[(*result_len)-1] = (uint8_t)padding_len; 

	// copy from buffer to result
	uint8_t* result = malloc(sizeof(uint8_t) * (*result_len));
	memcpy(result, buffer, sizeof(uint8_t) * (*result_len));
	free(buffer);
	return result;
} 

// decompression the uint8_t array and return uncompressed message 
// as well as change the message length after decompressed 
uint8_t* decode(struct dict* dictionary, uint8_t* origin_hex, 
	int origin_len, int* result_len) {

	// get the number of bits to decode
	int padding_len = origin_hex[origin_len-1];
	int array_size = (origin_len-1) * BYTE_SIZE - padding_len;

	// initialize index array to record the bit shift for ecah key
	uint8_t* index_array = malloc(sizeof(uint8_t) * MAX_BUFFER_SIZE);
	int index_count = 0;
	int index_cursor = 0;
	int array_cursor = 0;

	// initialize buffer to store result 
	int buffer_count = 0;
	uint8_t* buffer = malloc(sizeof(uint8_t) * MAX_BUFFER_SIZE); 

	// loop though the origin_hex
	uint8_t* temp_array = NULL; 
	while (array_cursor <= array_size) {
		int temp_count = 0; 
		temp_array = malloc(sizeof(uint8_t) * (origin_len - 1));

		// record the shift index 
		index_array[index_count] = index_cursor;

		// get bit shift from the origin hex
		for (int i = index_cursor; i < array_cursor; i++) {
			set_bit(temp_array, temp_count++, get_bit(origin_hex, i));
		}

		// loop through the dictionary 
		for (int i = 0; i < DICT_SIZE; i++) {
			// only check the value if they have the same length 
			if (dictionary[i].value_len == temp_count) {
				int match_count = 0;

				// check each individual bit 
				for (int j = 0; j < temp_count; j++) {
					uint8_t dict_bit = get_bit(dictionary[i].value, j);
					uint8_t array_bit = get_bit(temp_array, j);
					
					// increment the match count when each bit matched 
					if (dict_bit == array_bit) { match_count++; }
				}

				// when we found the key in dictionary
				if (match_count == temp_count) {

					// store this key in the buffer 
					buffer[buffer_count++] = dictionary[i].key;
					free(temp_array);
					temp_array = NULL;

					// update the index cursor andd index count 
					index_cursor = array_cursor;
					index_count++;
					break;
				} 
			}
		}
		array_cursor++;
	}

	// copy from buffer to result
	*result_len = buffer_count;
	uint8_t* result = malloc(sizeof(uint8_t) * (*result_len));
	memcpy(result, buffer, sizeof(uint8_t) * (*result_len));
	free(buffer);
	free(index_array);
	if (temp_array) { free(temp_array); }
	return result;
}
