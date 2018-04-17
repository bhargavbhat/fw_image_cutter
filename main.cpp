#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <chrono>

#include "crc32.h"

//#define DEBUG_CRC

const uint32_t METADATA_SIZE = 64;                  // in bytes
const uint32_t PRECOMPUTED_TEST_CRC = 0xCBF43926;   // precomputed CRC for test bytes from http://www.tahapaksu.com/crc/

int main(int argc, char** argv)
{
    if(argc != 9)
    {
        std::cout<<"Usage: fw_cutter input.bin start_offset_hex size_in_bytes_hex signature_in_hex major_ver minor_ver flag_metadata_write output.bin"<<std::endl;
        std::cout<<"Example: fw_cutter ti_stack.bin 0x203000 0x28000 0xABCD 2 0 1 fota_lower.bin"<<std::endl;
        return 0;
    }

#ifdef DEBUG_CRC
    // crc test
    unsigned char crc_test_input[] = "123456789";
    uint32_t crc_test_input_size = (sizeof(crc_test_input)/sizeof(crc_test_input[0])) - 1; // don't count NULL terminator
    uint32_t crc_test_value = crc32_gen(crc_test_input, crc_test_input_size);
    std::cout<<"CRC Test I/P: "<<crc_test_input<<std::showbase<<std::hex<<" Calc CRC="<<crc_test_value<<" Precomp CRC="<<PRECOMPUTED_TEST_CRC<<std::endl;
    if(PRECOMPUTED_TEST_CRC != crc_test_value)
    {
        std::cout<<"Precomputed & Calculated CRC don't match!"<<std::endl;
        return 0;
    }
#endif
  
    // parse command-line args
    std::string input_file = argv[1];
    uint32_t start_offset = std::strtoul(argv[2], NULL, 16);
    uint32_t section_size = std::strtoul(argv[3], NULL, 16);
    uint16_t image_sig = std::strtoul(argv[4], NULL, 16);
    uint16_t major_ver = std::strtoul(argv[5], NULL, 16);
    uint16_t minor_ver = std::strtoul(argv[6], NULL, 16);
    uint32_t flag_metadata_write = std::strtoul(argv[7], NULL, 16);
    std::string output_file = argv[8];

    auto millisecondsSinceEpoch = std::chrono::system_clock::now().time_since_epoch() / std::chrono::milliseconds(1);
    uint32_t metadata_ts = millisecondsSinceEpoch / 1000;
    uint32_t metadata_crc = 0x00;
    uint32_t metadata_sig = (image_sig << 16) | (major_ver << 8) | minor_ver;

    // print parameters
    std::cout<<"Input File  : "<<input_file<<std::endl;
    std::cout<<"Major Ver   : "<<std::dec<<major_ver<<std::endl;
    std::cout<<"Minor Ver   : "<<std::dec<<minor_ver<<std::endl;
    std::cout<<std::showbase<<std::internal<<std::setfill('0');
    std::cout<<"Start Offset: "<<std::hex<<start_offset<<std::endl;
    std::cout<<"Section Size: "<<std::hex<<section_size<<std::endl;
    std::cout<<"End Offset  : "<<std::hex<<start_offset + section_size<<std::endl;

    // open input file
    std::fstream in(input_file, std::ios::in | std::ios::binary);
    if(in.good())
    {
        // don't skip whitespace chars (we want 0xD and 0x0A to be retained)
        in >> std::noskipws;

        // check if sufficient bytes available
        // in input file from specified offset
        in.seekg(0, std::ios::end);
        auto end_pos = in.tellg();
        in.seekg(start_offset, std::ios::beg);
        auto start_pos = in.tellg();
        
        auto file_size = end_pos - start_pos;
        if(file_size >= section_size)
        {
            // file size is good
            std::vector<uint8_t> fw_metadata;   // holds metadata (signature, timestamp, crc)
            std::vector<uint8_t> fw_code;       // holds the actual code
            fw_code.reserve(section_size);      // reserve bytes to prevent repeated realloc
            try
            {
                // attempt to copy specified num of bytes from given offset
                std::copy_n(std::istream_iterator<unsigned char>(in), section_size, std::back_inserter(fw_code));
                std::cout<<"Read in "<<std::dec<<fw_code.size()<<" bytes"<<std::endl;
            }
            catch(...)
            {
                // some error occurred during file read
                std::cout<<"File read error, check filesize and section offset value"<<std::endl;
            }
                
            // code read complete, start metadata computation if requested
            if(flag_metadata_write)
            {   
                // calcuate CRC
                metadata_crc = crc32_ti_cc2538(fw_code.data(), fw_code.size());

                // print metadata
                std::cout<<"Signature   : "<<std::hex<<metadata_sig<<std::endl;
                std::cout<<"Timestamp   : "<<metadata_ts<<"("<<std::dec<<metadata_ts<<")"<<std::endl;
                std::cout<<"Image CRC   : "<<std::hex<<metadata_crc<<std::endl;
                std::cout<<"Section Size: "<<std::hex<<section_size<<std::endl;

                // fill in the metadata, in the order expected by FOTA manager and bootloader
                // current order is signature, timestamp, crc
                
                fw_metadata.resize(METADATA_SIZE);  // alloc to size of METADATA, actual data will be memcpy()'d
                uint32_t metadata_offset = 0;   // keep track of where to write within the vector
                std::memcpy(fw_metadata.data() + metadata_offset, &metadata_sig, sizeof(metadata_sig));
                metadata_offset += sizeof(metadata_sig);
                std::memcpy(fw_metadata.data() + metadata_offset, &metadata_ts, sizeof(metadata_ts));
                metadata_offset += sizeof(metadata_ts);
                std::memcpy(fw_metadata.data() + metadata_offset, &metadata_crc, sizeof(metadata_crc));
                metadata_offset += sizeof(metadata_crc);
                std::memcpy(fw_metadata.data() + metadata_offset, &section_size, sizeof(section_size));
                metadata_offset += sizeof(section_size);
#ifdef DEBUG
                std::cout<<"MetadataSize: "<<std::hex<<metadata_offset<<std::dec<<"("<<metadata_offset<<")"<<"bytes"<<std::endl;
#endif
            }
            else
            {
                std::cout<<"METADATA NOT CREATED (check flag in command-line)"<<std::endl;
            }
            std::fstream out(output_file, std::ios_base::out | std::ios_base::binary);
            if(out.good())
            {
                std::cout<<"Output File : "<<output_file<<std::endl;

                // write to file
                
                // metadata written only of flag is true
                if(flag_metadata_write)
                    out.write(reinterpret_cast<const char*>(fw_metadata.data()), fw_metadata.size()*sizeof(uint8_t));
                
                // code always written to file as-is
                out.write(reinterpret_cast<const char*>(fw_code.data()), fw_code.size()*sizeof(uint8_t));
               
                // done close file to flush contents to disk
                out.close();
            }
            else
            {
                std::cout<<"Can't open output file"<<std::endl;
            }
        }    
        else
        {
            std::cout<<"Can't jump to offset, check filesize"<<std::endl;
        }
        in.close();
    }
    else
    {
        std::cout<<"Can't open input file: "<<input_file<<std::endl;
        return 0;
    }
}
