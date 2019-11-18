#include "stdafx.h"

#define BUFFER_LENGTH 20

void showStreamDetails(FILE* fp, stream_header sh)
{
	int nb;
	uint16_t message_tag;
	module_skipped_message msm;
	int total_skipped = 0;
	int path_length;
	
	wchar_t local_path[BUFFER_LENGTH + 1];
	wchar_t local_exception[BUFFER_LENGTH + 1];
	msm.path = local_path;
	msm.exception = local_exception;

	if (sh.stream_type == modules_skipped_stream_type)
	{
		// Read list of module_skipped_message till sh.stream_size
		uint32_t rem = sh.stream_size;
		uint32_t minsize = sizeof(uint32_t) + 3*sizeof(uint16_t);
		while (rem > minsize)
		{
			message_tag = 0;
			msm.skip_reason = 0;
			msm.path_length = 0;
			msm.exception_length = 0;

			fread(&message_tag, sizeof(uint16_t), 1, fp);
			fread(&msm.skip_reason, sizeof(uint32_t), 1, fp);
			fread(&msm.path_length, sizeof(uint16_t), 1, fp);
			if (msm.path_length > BUFFER_LENGTH)
			{
				nb = fread(msm.path, sizeof(wchar_t), BUFFER_LENGTH, fp);
				msm.path[BUFFER_LENGTH] = 0;
				fseek(fp, (msm.path_length-nb)*sizeof(wchar_t), SEEK_CUR);
			}
			else
			{
				nb = fread(msm.path, sizeof(wchar_t), msm.path_length, fp);
				msm.path[msm.path_length] = 0;
			}

			fread(&msm.exception_length, sizeof(uint16_t), 1, fp);
			fseek(fp, msm.exception_length, SEEK_CUR);

			nb = (sizeof(wchar_t)*(msm.path_length + msm.exception_length)) + minsize;
			printf("Skipped Module Message: { module: %d, reason: %d, size: %d }\n", total_skipped, msm.skip_reason, nb);
			total_skipped++;
			rem = rem - nb;
		}

		printf("Total Skipped Modules: { count: %d }\n", total_skipped);
	}
}

void usage(char* progname)
{
	fprintf(stderr, "\nUsage: %s [df] <coverage file>\n\td: show details, such as stream stats\n\tf: fix corrupt coverage file", progname);
	exit(-1);
}

/*
 * This "quickfix" utility handles the "corrupt coverage" file cases observed in the Cloud Build scenario.
 * It happens when the coverage file is not properly finalized, due to race condition during shutdown sequence, which causes CodeCoverage.EXE to crash.
 */
int main(int argc, char** argv)
{
	char* optspec = "dfq";
	char optchar;

	// Flags which are controlled through cmd options
	int details = 0; // show details, such as stream stats from the coverage file.
	int fixcov = 0; // fix the coverage file, if found corrupt.
	int quite = 0; // be quite, and not print anything, except errors. Overrides 'details' option.

	while ((optchar = getopt(argc, argv, optspec)) != EOF)
	{
		switch (optchar)
		{
			case 'd': details = 1; break;
			case 'f': fixcov = 1; break;
			case 'q': quite = 1; break;
			case '?':
			default:
				usage(argv[0]);
		}
	}

	if (argc <= optind)
	{
		usage(argv[0]);
	}

	char* covfile = argv[optind];

	if (!quite) printf("Checking coverage file: %s\n", covfile);

	file_header fh;
	stream_header sh;
	int nb;
	int ns = 0;
	int corrupt = 0;

	FILE* fp = fopen(covfile, "r+b");
	if (fp == nullptr)
	{
		perror("Cannot open file in read/write mode. Error");
		return -3;
	}

	nb = fread(&fh, sizeof(file_header), 1, fp);
	if (nb != 1)
	{
		fprintf(stderr, "Cannot read the file header. Either the file is not a coverage file or it is corrupt beyond recovery.");
		return -1;
	}

	if (!quite) printf("File Header: { magic: 0x%x, schema: %d, nstreams: %d }\n", fh.magic, fh.message_schema_version, fh.streams_count);

	while ((nb = fread(&sh, sizeof(stream_header), 1, fp)) > 0)
	{
		if (!quite)
		{
			printf("Stream Header: { magic: 0x%x, size: %d, type: %s }\n", sh.magic, sh.stream_size,
				((sh.stream_type == modules_skipped_stream_type) ? "modules_skipped" :
				((sh.stream_type == module_data_stream_type) ? "modules_data" : "coverage_data"))
			);

			if (details)
			{
				long oldpos = ftell(fp);
				showStreamDetails(fp, sh);
				fseek(fp, oldpos, SEEK_SET);
			}
		}

		if (sh.magic != stream_header_magic)
		{
			fprintf(stderr, "Cannot fix the corrupt coverage file, as the stream offsets are not matching.");
			fprintf(stderr, "Either the file is not a coverage file or it is corrupt beyond recovery.");
			return -2;
		}
		ns++;
		fseek(fp, sh.stream_size, SEEK_CUR);
	}

	if (fh.magic == file_format_magic)
	{
		if (!quite) printf("The coverage file '%s' appears to be sound, and does not require any fix.\n", covfile);
		return 0;
	}

	if (!fixcov)
	{
		if (quite) return -5;
		printf("File found corrupt. Skipping the fix as -f option was not specified.");
		return 0;
	}

	fh.magic = file_format_magic;
	fh.streams_count = ns;
	if (!quite) printf("File found corrupt. Fixing it with { magic: 0x%x, schema: %d, nstreams: %d }\n", fh.magic, fh.message_schema_version, fh.streams_count);

	fseek(fp, 0, SEEK_SET);
	nb = fwrite(&fh, sizeof(fh), 1, fp);
	if (nb != 1)
	{
		perror("Could not fix the corrupt coverage file. Error");
		return -4;
	}

    return 0;
}
