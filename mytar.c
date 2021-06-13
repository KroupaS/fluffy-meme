#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct posix_header_s	      // Copy-pasted from gnu.org
{                              	      /* byte offset */
	char name[100];               /*   0 */
	char mode[8];                 /* 100 */
	char uid[8];                  /* 108 */
	char gid[8];                  /* 116 */
	char size[12];                /* 124 */
	char mtime[12];               /* 136 */
	char chksum[8];               /* 148 */
	char typeflag;                /* 156 */
	char linkname[100];           /* 157 */
	char magic[6];                /* 257 */
	char version[2];              /* 263 */
	char uname[32];               /* 265 */
	char gname[32];               /* 297 */
	char devmajor[8];             /* 329 */
	char devminor[8];             /* 337 */
	char prefix[155];             /* 345 */
	                              /* 500 */
} posix_header_t;

void error_invalid_invocation(char *);									// Error function, prints out why an invalid invocation occured.  
void error_archive_notfound(char *, char *);								// Error function, prints out a GNUTAR style "archive X not found" error message.
void error_unexpected_EOF(char *);									// Error function, exits the program with 2 and a GNUTAR style "unexpected EOF" message.

unsigned char arg_parse(int, char **, unsigned int *, unsigned int *, unsigned int *);			// Argument parsing function, returns a flag which indicates that either extraction or listing should be performed.
int name_listing(char **, unsigned int *, unsigned int, unsigned int);					// Function that performs listing, eventually called after arg_parse.
int print_filename(char **, char *, unsigned int *, unsigned int, unsigned int *, unsigned int *);	// Function called by 'name_listing' when specific filenames are passed as arguments to decide which should/should not be printed.
int extracting(char **, unsigned int *, unsigned int, unsigned int, int);				// Function which performs extraction, also eventually called after arg_parse.



int main(int argc, char **argv)
{
	for(int i = strlen(argv[0])-2; i>=0; i--)							// Parse argv[0] backwards until a '/' is found on some index 'x' (skipping the last character of argv[0]) 
	{												// and assign argv[0]=arv[x+1] to format the executable's name. 
		if(argv[0][i] == '/')									
		{											
			argv[0] = &(argv[0][i+1]);						
		}	
	}

	unsigned int filename_indices[argc-2];								// Array for storing indices of 'argv' which correspond to filenames. There cannot be more than argc-2 of them.
	unsigned int file_count = 0;									
	unsigned int *filename_counter = &file_count;							// And an integer to count how many filename indices have been stored so far.
	
	unsigned int arch_index = 0;									
	unsigned int *archive_index = &arch_index;							// Initializing an integer to store the index of argv which corresponds to the archive name.
	
	unsigned char flag;										// Flag for the argument parsing function.
													// Specific bits indicate (from lowest to highest): 
													// "-t" -> "-v" -> "-f" -> -"x" 
													// The four highest bits serve no purpose. 		


	flag = arg_parse(argc, argv, filename_indices, filename_counter, archive_index);		// Arg_parse either exits the program due to an invalid invocation, 
													// or returns a valid flag which will be used to specify the behaviour of this program.	
	
	if(flag & 8)											// If '-x' was passed, (flag & 8) and extraction function is called 
	{												// with verbosity if '-v' was also passed
		if(extracting(argv, filename_indices, *filename_counter, *archive_index, flag & 2) == 0)	 
		{
			return 0;									
		}
		else											// If extracting doesn't exit with 0, extraction wasn't succesful -> exit main with 2.
		{
			exit(2);
		}
	}
	else if(flag & 1)
	{
		if(name_listing(argv, filename_indices, *filename_counter, *archive_index) == 0)	// If the flag didn't have the '-x' bit set, it must have the '-t' bit set, therefore listing is called
		{									
			return 0; 									
		}	
		else
		{
			exit(2);
		}
	}
	else												// For safety reasons, as if arg_parse somehow returned a flag with neither '-x' or '-t' set, 
	{												// the ouput could be mistaken for non-verbose correct execution (if main just terminated here). 
		printf("Error: arg_parse returned a flag with no opmode selected\n");
	}

	return 0;
}



void error_invalid_invocation(char *error_statement)
{
	printf("Error: Invalid invocation: %s\n", error_statement);
	exit(2);
}

void error_archive_notfound(char *archive_name_err, char *err_ex_name)
{	
	fprintf(stderr, "%s: %s: Cannot open: No such file or directory\n", err_ex_name, archive_name_err);
	fprintf(stderr, "%s: Error is not recoverable: exiting now\n", err_ex_name);
	exit(2);
}

void error_unexpected_EOF(char *err_ex_name)
{
	fprintf(stdout, "%s: Unexpected EOF in archive\n", err_ex_name);
	fprintf(stdout, "%s: Error is not recoverable: exiting now\n", err_ex_name);
	exit(2);
}


unsigned char arg_parse(int arg_argc, char **arg_argv, unsigned int *arg_filename_indices,  		// Parses arguments and then stores filenames, file counts and the archive name into the 
		unsigned int *arg_filename_counter, unsigned int *arg_archive_index)			// corresponding pointers and returns an arg_flag (as long as input arguments are valid).
{													

	unsigned char arg_flag = 0;									
	
	for(int i = 1; i < arg_argc; ++i)
	{	
		if(arg_argv[i][0] == '-')			  
		{
			switch(arg_argv[i][1])								// A case for every valid option and a default for invalid ones. 
			{
				case('f'):		
					if(arg_flag & 4)
					{
						error_invalid_invocation("'-f' has been passed twice");
					}
					else
					{
						arg_flag |= 1 << 2;
						if(arg_argv[i+1] == 0)					// Checks that there is another argument.
						{
							error_invalid_invocation("No value specified for option '-f'");
						}
						*arg_archive_index = i+1;				// Take the archive name index as the index of the next argument and increment i to skip over it. 
						++i;							// (validity of the next argument as an archive name will be checked later in this function)
					}			
					break;
				
				case('t'):
					arg_flag |= 1;
					if(arg_argv[i][2] != 0)						// Checks whether there is another letter behind "-t", if so, terminate, as we don't support collation.
					{
						error_invalid_invocation("'-t' isn't properly terminated");
					}						
					break;

				case('v'):
					arg_flag |= 1 << 1;
					if(arg_argv[i][2] != 0)						// Again check for proper termination 
					{
						error_invalid_invocation("'-v' isn't properly terminated");
					}						
					break;

				case('x'):
					arg_flag |= 1 << 3;
					if(arg_argv[i][2] != 0)						// Again check for proper termination
					{
						error_invalid_invocation("'-x' isn't properly terminated");
					}						
					break;

				default:
					error_invalid_invocation("Invalid option, only options -tvxf are supported");
			}	
		} 
		else 
		{
			if((arg_flag & 1) || (arg_flag & 8))						// '-t' or '-x' was specified prior, therefore we add this argument to our filename array.
			{
				arg_filename_indices[(*arg_filename_counter)++] = i;
			}

			else 										// In this case neither -t nor -x have been passed but a non-option argument is being passed.
			{										// Therefore this is an invalid invocation.	
				error_invalid_invocation("A non-option argument has been passed without a valid corresponding option");
			}
		}
	}	
	
													// Arguments are now parsed and its time to check whether they make sense semantically.


	//if(!(((arg_flag & 1) || (arg_flag & 8)) && !((arg_flag & 1) && (arg_flag & 8))))		// Checks whether exactly one of -tx options have been passed. 
	if(((arg_flag & 1) ^ ((arg_flag & 8)/8)) != 1)
	{												// Assuming !((a || b) && !(a && b)) is the best way to do NOR
		error_invalid_invocation("You must specify exactly one of the -tx operation modes");
	}
	if(!(arg_flag & 4))										// Checks whether '-f' was passed.
	{
		error_invalid_invocation("'-f' option was not passed");
	}
	if(arg_argv[*arg_archive_index][0] == '-')							// Checks that the passed archive name does not start with '-'. 
	{												
		error_invalid_invocation("Another option passed right after '-f'");
	}

	return arg_flag;
}


int print_filename(char **list_argv, char *prnt_filename, unsigned int *prnt_array, unsigned int prnt_counter, unsigned int *prnt_filename_copy, unsigned int *prnt_filenames_found_counter)
{
	if(prnt_counter == 0)										// If prnt_counter == 0, no filenames were specified in the arguments, print the input string.
	{
		printf("%s\n", prnt_filename);
		return 0;
	}
	else
	{
		for(unsigned int i=0; i<prnt_counter; ++i)						// If prnt_counter > 0, iterate through prnt_array. If a match is found, print its name and return 0,
		{											// otherwise return 1 after the loop is finished.
			if(strcmp(prnt_filename, list_argv[prnt_array[i]]) == 0)
			{
				printf("%s\n", prnt_filename);
				prnt_filename_copy[i] = 0;						// Delete the found filename index from our filename indices array copy. (We know that argv[0] isn't a filename) 
				(*prnt_filenames_found_counter)++;					
				return 0;
			}
		}	
		return 1;
	}
}


int name_listing(char **list_argv, unsigned int *list_filename_indices, unsigned int list_filename_counter, unsigned int list_archive_index)
{
	FILE *archive_fp;

	if((archive_fp = fopen(list_argv[list_archive_index], "r")) == NULL)				// Checks if the specified archive file is "open-able".
	{
		error_archive_notfound(list_argv[list_archive_index], list_argv[0]);		
	}

	fseek(archive_fp, 0L, SEEK_END);
	const long archive_size = ftell(archive_fp);							// Store the archive size by fseeking to the end, then rewind. 
	fseek(archive_fp, 0L, SEEK_SET);								 
	
	posix_header_t current_header;									// posix_header_t struct for storing information from the current header during parsing of the archive.	
	posix_header_t *current_header_ptr = &current_header;
	
	long current_header_size;									// Integer to store the size of a file entry listed in the header
	int header_read_count;										// 	
	int zero_block_read_count;									// Two integers for storing how many bytes have been fread()-ed when parsing a header/when looking for null byte blocks. 

	unsigned int list_filename_indices_copy[list_filename_counter];					// Copy the list of indices of filenames so that later on we can determine which filenames have (/not) been found.
	memcpy(&list_filename_indices_copy, list_filename_indices, list_filename_counter*sizeof(unsigned int));
	unsigned int list_filenames_found_counter = 0;							// An integer to count how many filenames have been found so far.
	
	unsigned int current_pos = 0;									// Two integers used to count how many bytes there are in a given file entry of the archive file. 
	unsigned int file_block_end = 0;

	const char null_block[1024] = {0};								// Two 1024 byte blocks for comparisons later on. 
	char test_block[1024];										//
	unsigned char typeflag_check;									// Char for storing the current header's typeflag so that we can print it in an error if needed.
	
	while(ftell(archive_fp) < archive_size)
	{
		if((header_read_count = fread(current_header_ptr, 1, 512, archive_fp)) < 512)		// Attempt to read the next 512 bytes as a standard tar header, send an error to stdout if less than 512 bytes are read.
		{
			error_unexpected_EOF(list_argv[0]);
		}
		
		if(strcmp(current_header_ptr->magic, "ustar  ") != 0)					// Refuse anything that doesn't look like a tar archive. 
		{
			printf("%s: This does not look like a tar archive\n", list_argv[0]);
			printf("%s: Exiting with failure status due to previous errors\n", list_argv[0]);
			exit(2);
		}	

		if((typeflag_check = current_header_ptr->typeflag) != '0') 				// Check the typeflag, if its anything else than '0', exit(2).
		{
			printf("%s: Unsupported header type: %d\n", "mytar", typeflag_check);
			exit(2);
		}

		current_header_size = strtol(current_header_ptr->size, NULL, 8);					
		current_header_size = current_header_size - (current_header_size % 512) + (((current_header_size % 512) > 0)*512);
													// Pad the size listed in the header to 512 bytes. 				
		current_pos = ftell(archive_fp);
		file_block_end = fseek(archive_fp, current_header_size, SEEK_CUR);			// fseek through the expected number of file entry bytes.

		if((file_block_end - current_pos) < current_header_size)				// Check that we actually "fseeked" (sorry for the incorrect grammar) through the specified
		{											// amount of bytes.
			error_unexpected_EOF(list_argv[0]);
		}
													// As we have just passed through a valid header + file-entry pair, call our name printing function.
		print_filename(list_argv, current_header_ptr->name, list_filename_indices, list_filename_counter, list_filename_indices_copy, &list_filenames_found_counter); 
		
		if((fgetc(archive_fp) == EOF) && (ftell(archive_fp) == archive_size))
		{
			goto list_check_and_exit;							// If there is an EOF char right after the last file block silently exit with 0.
		}
		fseek(archive_fp, -1L, SEEK_CUR);
		

		if((zero_block_read_count = fread(test_block, 1, 1024L, archive_fp)) < 1024)		// Check if the next 1024 bytes are null, exit succesfully if so. Move the file pointer back if not.	
		{											 			
			if(!memcmp(test_block, null_block, 512)) 					// If less than 1024 bytes are left, first check if this is a lone zero block.	
			{
				printf("%s: A lone zero block at %ld\n", list_argv[0], ftell(archive_fp)/512);	// If it is, print a warning and exit the loop.
				goto list_check_and_exit;	
			}
			fseek(archive_fp, -zero_block_read_count, SEEK_CUR);				// If this is not a lone zero block, treat it as a header and try to list its name 			
			if((header_read_count = fread(current_header_ptr, 1, 512, archive_fp)) > 99)	// and then exit with unexpected EOF error.
			{
				printf("%s\n", current_header_ptr->name);
			}
			error_unexpected_EOF(list_argv[0]);
		}		
		if(!memcmp(test_block, null_block, 1024))
		{
			goto list_check_and_exit;							// The next two blocks are all zeroes, exit succesfully.
		}
		fseek(archive_fp, -1024L, SEEK_CUR);							// The last two blocks are not zero-blocks, move the file pointer back  
	}												// and carry on iterating through the archive.			
	
list_check_and_exit:											// We are at the end, just check if all passed filenames have been found and exit(0). 
	fclose(archive_fp);										// Otherwise print filenames that haven't been found and exit(2).
	if(list_filenames_found_counter < list_filename_counter)
	{
		for(unsigned int i = 0; i<list_filename_counter; ++i)
		       if(list_filename_indices_copy[i] > 0)
		       {
				printf("%s: %s: Not found in archive\n", list_argv[0], list_argv[list_filename_indices_copy[i]]);
		       }	       
		printf("%s: Exiting with failure status due to previous errors\n", list_argv[0]);
		return 2;
	}
	else
	{
	return 0;
	}
}


int extracting(char **ext_argv, unsigned int *ext_filename_indices, unsigned int ext_filename_counter, unsigned int ext_archive_index, int ext_verbosity)
{
	FILE *archive_fp;
	FILE *new_file;

	if((archive_fp = fopen(ext_argv[ext_archive_index], "r")) == NULL)				// Checks if the specified archive file is "open-able".	
	{
		error_archive_notfound(ext_argv[ext_archive_index], ext_argv[0]);		
	}

	fseek(archive_fp, 0L, SEEK_END);				
	const long archive_size = ftell(archive_fp);							// Store the archive size by fseeking to the end, then rewind. 
	fseek(archive_fp, 0L, SEEK_SET);		

	posix_header_t current_header;									// posix_header_t struct for storing information from the current header during parsing of the archive.	
	posix_header_t *current_header_ptr = &current_header;
	char file_content[512];										// Char array for storing a single file entry block, 
													// will be used to extract file entries into new files.
	
	unsigned int current_pos = 0;									// Two integers used to check whether there the specified amount of bytes is present in a given file entry. 
	unsigned int file_block_end = 0;								// 
	
	long current_header_size;									// Two integers used to calculate how many byte blocks we are expecting in an entry
	unsigned int file_block_overhang = 0;								// 

	int header_read_count;										// Three integers used to verify whether we have read the correct amount of bytes from the archive file.
	int file_read_count;										//		
	int zero_block_read_count;									//

	unsigned int ext_filename_indices_copy[ext_filename_counter];					// We copy the list of indices of filenames. This is needed for determining which filenames have (/not) been found.
	memcpy(&ext_filename_indices_copy, ext_filename_indices, ext_filename_counter*sizeof(unsigned int));
	unsigned int ext_filenames_found_counter = 0;							// An integer to count how many filenames have been found.
	
	const char null_block[1024] = {0};
	char test_block[1024];
	unsigned char typeflag_check;									// Char for storing the current header's typeflag so that we can print it in an error if needed.

	while(ftell(archive_fp) < archive_size)
	{
		if((header_read_count = fread(current_header_ptr, 1, 512, archive_fp)) < 512)		// Attempt to read the next 512 bytes as a standard tar header, exit with EOF error if less than 512 bytes are read.
		{
			error_unexpected_EOF(ext_argv[0]);
		}

		if(strcmp(current_header_ptr->magic, "ustar  ") != 0)					// Refuse anything that doesn't look like a tar archive. 
		{
			printf("%s: This does not look like a tar archive\n", ext_argv[0]);
			printf("%s: Exiting with failure status due to previous errors\n", ext_argv[0]);
			exit(2);
		}	
		
		if((typeflag_check = current_header_ptr->typeflag) != '0') 				// Check the typeflag, if its anything else than '0', exit(2).
		{
			printf("%s: Unsupported header type: %c\n", ext_argv[0], typeflag_check);
			exit(2);
		}
		current_header_size = strtol(current_header_ptr->size, NULL, 8);			
		
		file_block_overhang = current_header_size % 512;
		current_header_size = current_header_size 
			- file_block_overhang + 512*((current_header_size % 512) > 0);			// Get the expected number of bytes in the next file entry.

		if(ext_filename_counter > 0)								// If there are any filenames in the argument list, check if our current header's name gets a match.
		{
			for(unsigned int i=0; i<ext_filename_counter; ++i)				
			{										
				if(strcmp(current_header_ptr->name, ext_argv[ext_filename_indices[i]]) == 0)
				{
					ext_filename_indices_copy[i] = 0;				// Delete the found filename index from our filename indices array copy and jump over to extraction. 
					++ext_filenames_found_counter;
					goto extract;							
				}
			}
			current_pos = ftell(archive_fp);						// If your current header's name wasn't passed as an argument, just skip over the next file entry
			file_block_end = fseek(archive_fp, current_header_size, SEEK_CUR);	

			if((file_block_end - current_pos) < current_header_size)			// Check if the file entry is too short.
			{
				error_unexpected_EOF(ext_argv[0]);
			}
		}
		else											// If no filenames have been passed go directly to extraction. 
		{
extract:
			if(ext_verbosity > 0)								// If verbosity option was passed, now its time to print the name from the current header.
			{
				printf("%s\n", current_header_ptr->name);
			}
			new_file = fopen(current_header_ptr->name, "w");	
			if(current_header_size == 0)							// If the file entry is empty, skip over to the next iteration.
			{
				continue;
			}
			for(unsigned int i = 0; i<(current_header_size/512)-1; ++i)			// Unless the archive file suddenly ends with EOF, keep extracting blocks from the current file entry
			{										// into the new file.
				if((file_read_count = fread(file_content, 1, 512, archive_fp)) < 512)		
				{
					error_unexpected_EOF(ext_argv[0]);
				}
				else
				{
					fwrite(file_content, 1, 512, new_file);
				}
			}					
			if((file_read_count = fread(file_content, 1, 512, archive_fp)) < 512)		// The for loop was stopped one iteration early, now we extract the last 'file_block_overhang'
			{										// bytes.
				error_unexpected_EOF(ext_argv[0]);
			}
			else
			{
				if(current_header_size % 512 == 0)					// If the current file's size is a multiple of 512, the last iteration needs to write a full 512b block and 
				{									// file_block_overhang is zero so we manually set it to 512.
					file_block_overhang = 512;					
				}
				fwrite(file_content, 1, file_block_overhang, new_file);			 
			}
			fclose(new_file);								// Now we can close new_file to prevent future problems.
			
		}

		if((fgetc(archive_fp) == EOF) && (ftell(archive_fp) == archive_size))
		{
			goto ext_check_and_exit;							// If there is an EOF char right after the last file block silently exit with 0.
		}
		fseek(archive_fp, -1L, SEEK_CUR);

		if((zero_block_read_count = fread(test_block, 1, 1024L, archive_fp)) < 1024)		// Check if the next 1024 bytes are null, exit succesfully if so. Move the file pointer back if not.	
		{											 			
			if(!memcmp(test_block, null_block, 512)) 					// If less than 1024 bytes are left, first check if this is a lone zero block.	
			{
				printf("%s: A lone zero block at %ld\n", ext_argv[0], ftell(archive_fp)/512);	// If it is, print a warning and exit the loop.
				goto ext_check_and_exit;	
			}
			fseek(archive_fp, -zero_block_read_count, SEEK_CUR);				// If this is not a lone zero block, "rewind" the pointer back and continue in the next iteration 		
		}		
		if(!memcmp(test_block, null_block, 1024))
		{
			goto ext_check_and_exit;							// The next two blocks are all zeroes, exit succesfully.
		}
		fseek(archive_fp, -1024L, SEEK_CUR);							// The last two blocks are not zero-blocks, move the file pointer back  
	}
ext_check_and_exit:									
	fclose(archive_fp);										
	if(ext_filenames_found_counter < ext_filename_counter)						// If this condition is true there must be some filenames passed in the arguments that were 
	{												// not found, print them out and exit with failure status. 
		for(unsigned int i = 0; i<ext_filename_counter; ++i)
		{
		       if(ext_filename_indices_copy[i] > 0)
		       {
				printf("%s: %s: Not found in archive\n", ext_argv[0], ext_argv[ext_filename_indices_copy[i]]);
		       }
		}	       
		printf("%s: Exiting with failure status due to previous errors\n", ext_argv[0]);
		return 2;
	}
	else												// If both integers from the previous condition are 0 or both are the same non-zero value, we 
	{												// return 0 to main, as either no filenames were passed as arguments or all of them were found.
	return 0;											
	}
}	
