#pragma once

#include <stdint.h>
#include <Windows.h>

const uint32_t file_format_magic = 0xFF484350;
const uint32_t stream_header_magic = 0x504348FF;

GUID const modules_skipped_stream_type = { 0xb157a3a4, 0x9542, 0x4546,{ 0x85, 0x9b, 0xab, 0xd1, 0x36, 0x7a, 0x8, 0xed } }; // {B157A3A4-9542-4546-859B-ABD1367A08ED}
GUID const module_data_stream_type = { 0x2a4f0f81, 0x6809, 0x4480,{ 0x87, 0xce, 0xd, 0x77, 0x25, 0x68, 0xcf, 0xb6 } }; // {2A4F0F81-6809-4480-87CE-0D772568CFB6}
GUID const coverage_data_stream_type = { 0xcb689549, 0xd63e, 0x4a55,{ 0xb8, 0xfa, 0x2a, 0x21, 0x47, 0x25, 0xcf, 0x39 } }; // {CB689549-D63E-4A55-B8FA-2A214725CF39}

#pragma pack(push)
#pragma pack(1)

//! The coverage file header.
struct file_header
{
	uint32_t magic;             // Must be file_format_magic
	uint16_t message_schema_version;
	uint32_t streams_count;
};

//! The stream header.
struct stream_header
{
	uint32_t magic;             // Must be stream_header_magic
	GUID stream_type;           // The type of the stream
	GUID identifier;            // An identifier for the stream; may be GUID_NULL
	uint32_t stream_size;       // Size of the stream, not including the stream header
};

struct module_skipped_message
{
	uint32_t skip_reason;
	uint32_t path_length;
	wchar_t* path;
	uint32_t exception_length;
	wchar_t* exception;
};

#pragma pack(pop)

