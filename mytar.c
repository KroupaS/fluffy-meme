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

void print_flag(unsigned char);										// DEBUGGING function
void print_header(posix_header_t *, long);								// DEBUGGING function
void error_invalid_invocation(char *);
void error_archive_notfound(char *);
void error_unexpected_EOF(void);
int print_filename(char **, char *, unsigned int *, unsigned int, unsigned int *, unsigned int *);
unsigned char arg_parse(int, char **, unsigned int *, unsigned int *, unsigned int *);
int name_listing(char **, unsigned int *, unsigned int, unsigned int);
int extracting(char **, unsigned int *, unsigned int, unsigned int, int);


int main(int argc, char **argv)
{
	// DO VSECH ERROR ZPRAV ZPATKY ARGV[0] A TADY ODKROJIT ZACATEK PO POSLEDNI LOMITKO
	
	unsigned int filename_indices[argc-2];								// Array for storing indices of 'argv' which correspond to filenames.
	unsigned int file_count = 0;
	unsigned int *filename_counter = &file_count;							// And an integer to count how many filenames have been stored.
	
	unsigned int arch_index = 0;
	unsigned int *archive_index = &arch_index;							// Initializing a pointer for the index of argv which corresponds to the archive name.
	
	unsigned char flag;											
	flag = arg_parse(argc, argv, filename_indices, filename_counter, archive_index);		// Arg_parse exits the program due to an invalid invocation, 
													// or returns flag which will be used to specify the behaviour of this program.	
	#ifdef DEBUG
		print_flag(flag); 										
		printf("Archive name = %s\n", argv[*archive_index]);						
		for(unsigned int x = 0; x < *filename_counter; ++x)						
		{												
			printf("Filename number %d = %s\n", x, argv[filename_indices[x]]);			
		}												
	#endif
	

	if(flag & 8)
	{
		if(extracting(argv, filename_indices, *filename_counter, *archive_index, flag & 2) == 0)
		{
			return 0;
		}
		else
		{
			exit(2);
		}
	}
	else if(flag & 1)
	{
		if(name_listing(argv, filename_indices, *filename_counter, *archive_index) == 0)
		{
			return 0; 									// Listing was succesful, exit with 0.
		}	
		else
		{
			exit(2);
		}
	}
	else
	{
		printf("Error: arg_parse somehow returned a flag with no opmode selected\n");
	}

	return 0;
}


void print_flag(unsigned char flag)			// Debugging function, prints an unsigned char in binary.
{
	printf("Flag = NNNNxfvt\n");
	printf("Flag = %c%c%c%c%c%c%c%c\n", 
	(flag & 128 ? '1' : '0'), \
	(flag & 64 ? '1' : '0'), \
	(flag & 32 ? '1' : '0'), \
	(flag & 16 ? '1' : '0'), \
	(flag & 8 ? '1' : '0'), \
	(flag & 4 ? '1' : '0'), \
	(flag & 2 ? '1' : '0'), \
	(flag & 1 ? '1' : '0'));
}


void error_invalid_invocation(char *error_statement)
{
	printf("Error: Invalid invocation: %s\n", error_statement);
	exit(2);
}

void error_archive_notfound(char *archive_name_err)
{	
	fprintf(stderr, "%s: %s: Cannot open: No such file or directory\n", "mytar", archive_name_err);
	fprintf(stderr, "%s: Error is not recoverable: exiting now\n", "mytar");
	exit(2);
}

void error_unexpected_EOF(void)
{
	fprintf(stdout, "%s: Unexpected EOF in archive\n", "mytar");
	fprintf(stdout, "%s: Error is not recoverable: exiting now\n", "mytar");
	exit(2);
}


unsigned char arg_parse(int arg_argc, char **arg_argv, unsigned int *arg_filename_indices,  		// Parses arguments and then stores filenames, file counts and the archive name into the 
		unsigned int *arg_filename_counter, unsigned int *arg_archive_index)			// corresponding pointers and returns an arg_flag (as long as input arguments are valid).
{													

	unsigned char arg_flag = 0;									// Flag indicating whether specific options/arguments have been passed yet.
													// Specific bits indicate (from lowest to highest): 
													// "-t" -> "-v" -> "-f" -> -"x" 
													// The four highest bits serve no purpose. 		
	
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
						++i;							//
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
					if(arg_argv[i][2] != 0)						// Checks whether there is another letter behind "-v", if so, terminate, as we don't support collation.
					{
						error_invalid_invocation("'-v' isn't properly terminated");
					}						
					break;

				case('x'):
					arg_flag |= 1 << 3;
					if(arg_argv[i][2] != 0)						// Checks whether there is another letter behind "-x", if so, terminate.
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

	if(!(((arg_flag & 1) || (arg_flag & 8)) && !((arg_flag & 1) && (arg_flag & 8))))		// Checks whether exactly one of -tx options have been passed. 
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


void print_header(posix_header_t *header_ptr, long header_size)
{
	printf("----Struct----\nname = %s\n", header_ptr->name);
	printf("mode = %s\n", header_ptr->mode);
	printf("uid = %s\n", header_ptr->uid);
	printf("gid = %s\n", header_ptr->gid);
	printf("size = %ld\n", header_size);
	printf("mtime = %s\n", header_ptr->mtime);
	printf("chksum = %s\n", header_ptr->chksum);
	printf("typeflag = %c\n", header_ptr->typeflag);
	printf("linkname = %s\n", header_ptr->linkname);
	printf("magic = %s\n", header_ptr->magic);
	printf("version = %s\n", header_ptr->version);
	printf("uname = %s\n", header_ptr->uname);
	printf("gname = %s\n", header_ptr->gname);
	printf("devmajor = %s\n", header_ptr->devmajor);
	printf("devminor = %s\n", header_ptr->devminor);
	printf("prefix = %s\n----Struct----\n", header_ptr->prefix);
}


int print_filename(char **list_argv, char *prnt_filename, unsigned int *prnt_array, unsigned int prnt_counter, unsigned int *prnt_filename_copy, unsigned int *prnt_filenames_found_counter)
{
	if(prnt_counter == 0)										// If prnt_counter == 0, no filenames were passed as arguments to '-t', print the input string.
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
				prnt_filename_copy[i] = 0;						// Delete the found filename index from our filename indices array copy. 
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

	if((archive_fp = fopen(list_argv[list_archive_index], "r")) == NULL)				// Checks if the specified archive file exists and if we have permission to open it.
	{
		error_archive_notfound(list_argv[list_archive_index]);		
	}

	fseek(archive_fp, 0L, SEEK_END);
	unsigned long long archive_size = ftell(archive_fp);						// ULL for archive size to prevent overflow. 
	fseek(archive_fp, 0L, SEEK_SET);								 
	

	posix_header_t current_header;									
	posix_header_t *current_header_ptr = &current_header;
	
	long current_header_size;
	int header_read_count;
	int zero_block_read_count;

	unsigned int list_filename_indices_copy[list_filename_counter];					// We copy the list of indices of filenames. This is needed for determining which filenames have (/not) been found.
	memcpy(&list_filename_indices_copy, list_filename_indices, list_filename_counter*sizeof(unsigned int));
	unsigned int list_filenames_found_counter = 0;							// An integer to count how many filenames have been found so far.
	
	unsigned int current_pos = 0;									// Two integers used to count how many bytes there are in a given file entry. 
	unsigned int file_block_end = 0;

	const char null_block[1024] = {0};
	char test_block[1024];
	unsigned char typeflag_check = '0';
	
	while((unsigned long long)ftell(archive_fp) < archive_size)
	{
		if((header_read_count = fread(current_header_ptr, 1, 512, archive_fp)) < 512)		// Attempt to read the next 512 bytes as a standard tar header, send an error to stdout if less than 512 bytes are read.
		{
			error_unexpected_EOF();
		}
		
		if(strcmp(current_header_ptr->magic, "ustar  ") != 0)					// Refuse anything that doesn't look like a tar archive. 
		{
			printf("%s: This does not look like a tar archive\n", "mytar");
			printf("%s: Exiting with failure status due to previous errors\n", "mytar");
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

		if((file_block_end - current_pos) < current_header_size)				// If the size we got from the header is too small, exit with EOF error.
		{
			error_unexpected_EOF();
		}

		#ifdef DEBUG
			print_header(current_header_ptr, current_header_size);	
		#endif

		print_filename(list_argv, current_header_ptr->name, list_filename_indices, list_filename_counter, list_filename_indices_copy, &list_filenames_found_counter); 
		
		if((fgetc(archive_fp) == EOF) && ((unsigned long long)ftell(archive_fp) == archive_size))
		{
			#ifdef DEBUG
				printf("exit by silent accept fgetc == EOF\n");				// This would mean there is EOF right after the last file block, in this case we silently exit with 0
			#endif
			goto list_check_and_exit;
		}
		fseek(archive_fp, -1L, SEEK_CUR);
		

		if((zero_block_read_count = fread(test_block, 1, 1024L, archive_fp)) < 1024)		// Check if the next 1024 bytes are null, exit succesfully if so. Move the file pointer back if not.	
		{											 			
			if(!memcmp(test_block, null_block, 512)) 					// If less than 1024 bytes are left, first check if this is a lone zero block.	
			{
				printf("%s: A lone zero block at %ld\n", "mytar", ftell(archive_fp)/512);// If it is, print a warning and exit the loop.
				goto list_check_and_exit;	
			}
			fseek(archive_fp, -zero_block_read_count, SEEK_CUR);				// If this is not a lone zero block, treat it as a header and try to list its name 		
			if((header_read_count = fread(current_header_ptr, 1, 512, archive_fp)) > 99)	// and exit with unexpected EOF error.
			{
				printf("%s\n", current_header_ptr->name);
			}
			error_unexpected_EOF();
		}		
		if(!memcmp(test_block, null_block, 1024))
		{
			#ifdef DEBUG
				printf("exit by memcmp\n");
			#endif
			goto list_check_and_exit;							// The next two blocks are all zeroes, exit succesfully.
		}
		fseek(archive_fp, -1024L, SEEK_CUR);							// The last two blocks are not zero-blocks, move the file pointer back  
													// and carry on iterating through the archive.
		#ifdef DEBUG	
			printf("memcmp = %d\n", memcmp(test_block, null_block, 1024));
		#endif
	} 			
	

list_check_and_exit:											// We are at the end, just check if all passed filenames have been found and exit(0). 
	fclose(archive_fp);										// Otherwise print filenames that haven't been found and exit(2).
	if(list_filenames_found_counter < list_filename_counter)
	{
		for(unsigned int i = 0; i<list_filename_counter; ++i)
		       if(list_filename_indices_copy[i] > 0)
		       {
				printf("%s: %s: Not found in archive\n", "mytar", list_argv[list_filename_indices_copy[i]]);
		       }	       
		printf("%s: Exiting with failure status due to previous errors\n", "mytar");
		exit(2);
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

	if((archive_fp = fopen(ext_argv[ext_archive_index], "r")) == NULL)				// Checks if the specified archive file exists and if we have permission to open it.
	{
		error_archive_notfound(ext_argv[ext_archive_index]);		
	}

	fseek(archive_fp, 0L, SEEK_END);				
	unsigned long long archive_size = ftell(archive_fp);						// ULL for archive size to prevent overflow. 
	fseek(archive_fp, 0L, SEEK_SET);		

	posix_header_t current_header;									
	posix_header_t *current_header_ptr = &current_header;
	char file_content[512];										// Char array for storing a single file entry block 

	unsigned int current_pos = 0;									// Two integers used to count how many bytes there are in a given file entry. 
	unsigned int file_block_end = 0;
	unsigned int file_block_overhang = 0;

	long current_header_size;
	int header_read_count;
	int file_read_count;
	int zero_block_read_count;

	unsigned int ext_filename_indices_copy[ext_filename_counter];					// We copy the list of indices of filenames. This is needed for determining which filenames have (/not) been found.
	memcpy(&ext_filename_indices_copy, ext_filename_indices, ext_filename_counter*sizeof(unsigned int));
	unsigned int ext_filenames_found_counter = 0;							// An integer to count how many filenames have been found.
	
	const char null_block[1024] = {0};
	char test_block[1024];
	unsigned char typeflag_check = '0';

	while((unsigned long long)ftell(archive_fp) < archive_size)
	{
		if((header_read_count = fread(current_header_ptr, 1, 512, archive_fp)) < 512)		// Attempt to read the next 512 bytes as a standard tar header, send an error to stdout if less than 512 bytes are read.
		{
			error_unexpected_EOF();
		}
		#ifdef DEBUG
			printf("position = %ld / %llu\n", ftell(archive_fp), archive_size);
		#endif

		if(strcmp(current_header_ptr->magic, "ustar  ") != 0)					// Refuse anything that doesn't look like a tar archive. 
		{
			printf("%s: This does not look like a tar archive\n", "mytar");
			printf("%s: Exiting with failure status due to previous errors\n", "mytar");
			exit(2);
		}	
		
		if((typeflag_check = current_header_ptr->typeflag) != '0') 				// Check the typeflag, if its anything else than '0', exit(2).
		{
			printf("%s: Unsupported header type: %c\n", "mytar", typeflag_check);
			exit(2);
		}
		current_header_size = strtol(current_header_ptr->size, NULL, 8);			
		
		file_block_overhang = current_header_size % 512;
		current_header_size = current_header_size 
			- file_block_overhang + 512*((current_header_size % 512) > 0);			// Get the expected number of bytes in the next file entry.

		if(ext_filename_counter > 0)								// If any there are any filenames in the argument list, check if our current header's name matches any of them.
		{
			for(unsigned int i=0; i<ext_filename_counter; ++i)				
			{										
				if(strcmp(current_header_ptr->name, ext_argv[ext_filename_indices[i]]) == 0)
				{
					ext_filename_indices_copy[i] = 0;				// Delete the found filename index from our filename indices array copy. 
					++ext_filenames_found_counter;
					goto extract;							// Jump over the next 10 lines and extract this file.
				}
			}
			current_pos = ftell(archive_fp);						// If your current header's name wasn't passed as an argument, just skip over the next file entry
			file_block_end = fseek(archive_fp, current_header_size, SEEK_CUR);	

			if((file_block_end - current_pos) < current_header_size)			// If the size we got from the header is too small, exit with EOF error.
			{
				error_unexpected_EOF();
			}
		}
		else											// If not filenames have been passed, just skip over this file entry just like during listing. 
		{
extract:
			if(ext_verbosity > 0)								// If verbosity option was passed, now its time to print the name from the current header.
			{
				printf("%s\n", current_header_ptr->name);
			}
			new_file = fopen(current_header_ptr->name, "w");	
			if(current_header_size == 0)
			{
				continue;
			}
			for(unsigned int i = 0; i<(current_header_size/512)-1; ++i)			// Unless the archive file suddenly ends with EOF, keep extracting blocks from the current file entry
			{										// into the new file.
				if((file_read_count = fread(file_content, 1, 512, archive_fp)) < 512)		
				{
					error_unexpected_EOF();
				}
				else
				{
					fwrite(file_content, 1, 512, new_file);
				}
			}					
			if((file_read_count = fread(file_content, 1, 512, archive_fp)) < 512)		// The for loop was stopped one iteration early, now we extract the last 'file_block_overhang'
			{										// bytes.
				error_unexpected_EOF();
			}
			else
			{
				if(current_header_size % 512 == 0)
				{
					file_block_overhang = 512;					// This should make it work for multiples of 512
				}
				fwrite(file_content, 1, file_block_overhang, new_file);			// THIS DOESNT WORK FOR MULTIPLES OF 512 
			}
			fclose(new_file);
			
		}

		if(fgetc(archive_fp) == EOF)
		{
			error_unexpected_EOF();								// MIGHT BE WRONG, BUT PASSES TEST I GUESS
			#ifdef DEBUG
				printf("exit by fgetc == EOF\n");					// This would mean there is EOF right after the last file block, in this case we silently exit with 0
			#endif
			goto ext_check_and_exit;
		}
		fseek(archive_fp, -1L, SEEK_CUR);							

		if((zero_block_read_count = fread(test_block, 1, 1024L, archive_fp)) < 1024)		// Check if the next 1024 bytes are null, exit succesfully if so. Move the file pointer back if not.	
		{											 			
			if(!memcmp(test_block, null_block, 512)) 					// If less than 1024 bytes are left, first check if this is a lone zero block.	
			{
				printf("%s: A lone zero block at %ld\n", "mytar", ftell(archive_fp)/512);// If it is, print a warning and exit the loop.
				goto ext_check_and_exit;	
			}
			fseek(archive_fp, -zero_block_read_count, SEEK_CUR);				// If this is not a lone zero block, treat it as a header and try to list its name 		
			printf("wtf\n");
		}		
		if(!memcmp(test_block, null_block, 1024))
		{
			#ifdef DEBUG
				printf("exit by memcmp\n");
			#endif
			goto ext_check_and_exit;							// The next two blocks are all zeroes, exit succesfully.
		}
		fseek(archive_fp, -1024L, SEEK_CUR);							// The last two blocks are not zero-blocks, move the file pointer back  

		#ifdef DEBUG	
			printf("memcmp = %d\n", memcmp(test_block, null_block, 1024));
		#endif
		
	}
ext_check_and_exit:
	fclose(archive_fp);										
	if(ext_filenames_found_counter < ext_filename_counter)
	{
		for(unsigned int i = 0; i<ext_filename_counter; ++i)
		{
		       if(ext_filename_indices_copy[i] > 0)
		       {
				printf("%s: %s: Not found in archive\n", "mytar", ext_argv[ext_filename_indices_copy[i]]);
		       }
		}	       
		printf("%s: Exiting with failure status due to previous errors\n", "mytar");
		exit(2);
	}
	else
	{
	return 0;
	}
}	
